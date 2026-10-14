#include "triagecontroller.h"

#include "../core/paths.h"
#include "../core/prefs.h"
#include "../platform/mac.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>
#include <QtConcurrent>
#include <algorithm>

using namespace Triage;

namespace {
const QList<double> kGridSizes = {120, 160, 220, 300, 400};

QString watchedFolder() {
    const QString p = Prefs::outputPath();
    return QFileInfo(p).isDir() ? p : Paths::defaultOutput();
}
}  // namespace

TriageController::TriageController(QObject *parent) : QObject(parent), m_store(this), m_gallery(this) {
    connect(&m_store, &Store::changed, this, &TriageController::onStoreChanged);
    m_watchPath = watchedFolder();
}

// ── Boot / scan ──────────────────────────────────────────────────────────
void TriageController::boot() {
    if (m_booted) return;
    m_booted = true;
    const double g = Prefs::sorterGridSize(160);
    if (kGridSizes.contains(g)) m_gridSize = g;
    m_inspectorOpen = Prefs::sorterInspectorOpen(true);
    emit gridSizeChanged();
    emit inspectorOpenChanged();
    m_store.load();
    rescan();
}

void TriageController::rescan() {
    m_scanning = true; emit scanningChanged();
    m_watchPath = watchedFolder();
    emit watchPathChanged();
    QStringList files;
    for (const QString &n : QDir(m_watchPath).entryList(QDir::Files)) if (isBmpOutput(n)) files << QDir(m_watchPath).filePath(n);
    m_store.reconcile(files, Desktop);
    startWatcher();
    m_scanning = false; emit scanningChanged();
}

void TriageController::startWatcher() {
    if (m_watcher) { m_watcher->stop(); m_watcher->deleteLater(); }
    m_watcher = new FolderWatcher(m_watchPath, this);
    connect(m_watcher, &FolderWatcher::added, this, [this](const QStringList &paths) { markNew(m_store.reconcile(paths, Desktop)); });
    connect(m_watcher, &FolderWatcher::removed, this, [this](const QStringList &paths) {
        for (const auto &p : paths) if (auto *e = m_store.entry(p)) e->setMissing(true);
        m_store.touch();
    });
}

void TriageController::markNew(const QList<EntryItem *> &added) {
    for (auto *e : added) {
        e->setIsNew(true);
        QPointer<EntryItem> guard(e);
        QTimer::singleShot(1400, this, [guard] { if (guard) guard->setIsNew(false); });
    }
}

void TriageController::ingest(const QString &path) { markNew(m_store.reconcile({path}, Desktop)); }

void TriageController::importFolder(const QUrl &folder) {
    if (!folder.isLocalFile()) return;
    markNew(m_store.reconcile(Store::expand({folder.toLocalFile()}), Folder));
}

void TriageController::importPaths(const QList<QUrl> &urls) {
    QStringList raw;
    for (const auto &u : urls) if (u.isLocalFile()) raw << u.toLocalFile();
    raw = Store::expand(raw);
    const QString lib = Paths::sorterLibrary();
    QStringList files;
    for (const auto &p : raw) files << ((p.startsWith(m_watchPath) || p.startsWith(lib)) ? p : Store::copyToLibrary(p));
    markNew(m_store.reconcile(files, Drop));
}

// ── Derived lists ────────────────────────────────────────────────────────
void TriageController::onStoreChanged() {
    // drop selection / focus of entries that no longer exist
    QSet<QString> alive;
    for (const auto &p : std::as_const(m_selected)) if (m_store.entry(p)) alive.insert(p);
    if (alive != m_selected) { m_selected = alive; emit selectionChanged(); }
    if (!m_anchor.isEmpty() && !m_store.entry(m_anchor)) { m_anchor.clear(); emit selectionChanged(); }
    recompute();
    emit revisionChanged();
}

void TriageController::recompute() {
    QList<EntryItem *> list;
    int status[5] = {0, 0, 0, 0, 0};
    for (auto *e : m_store.entries()) {
        status[e->status()]++;
        if (m_filter > 0) { if (e->status() != m_filter - 1) continue; }
        else if (e->status() == Archived) continue;     // 'All' hides archived on purpose
        list << e;
    }
    const QString q = m_search.trimmed().toLower();
    if (!q.isEmpty()) {
        list.erase(std::remove_if(list.begin(), list.end(), [&](EntryItem *e) {
            return !e->path().toLower().contains(q) && !e->note().toLower().contains(q);
        }), list.end());
    }
    switch (m_sort) {
    case Newest: std::sort(list.begin(), list.end(), [](EntryItem *a, EntryItem *b) { return a->addedAt() > b->addedAt(); }); break;
    case Oldest: std::sort(list.begin(), list.end(), [](EntryItem *a, EntryItem *b) { return a->addedAt() < b->addedAt(); }); break;
    case ByStatus: std::sort(list.begin(), list.end(), [](EntryItem *a, EntryItem *b) {
            const int sa = statusSortOrder(a->status()), sb = statusSortOrder(b->status());
            return sa != sb ? sa < sb : a->addedAt() > b->addedAt();
        }); break;
    case ByName: std::sort(list.begin(), list.end(), [](EntryItem *a, EntryItem *b) {
            return QString::localeAwareCompare(a->path(), b->path()) < 0; }); break;
    }
    m_filtered = list;

    int total = 0;
    for (int i = 0; i < 5; ++i) { m_statusCounts[i] = status[i]; total += status[i]; }
    m_filterCounts[0] = total - status[Archived];
    for (int i = 0; i < 5; ++i) m_filterCounts[i + 1] = status[i];

    m_grouped = m_filter == All && q.isEmpty();
    m_groups = m_grouped ? computeGroups() : QList<Group>();
    if (m_groups.isEmpty()) m_grouped = false;

    m_orderAll.clear();
    if (m_grouped) { for (const auto &g : m_groups) for (const auto &s : g.subs) m_orderAll += s.entries; }
    else m_orderAll = m_filtered;

    if (m_focusIndex >= m_orderAll.size()) { m_focusIndex = m_orderAll.isEmpty() ? -1 : int(m_orderAll.size()) - 1; emit focusChanged(); }

    rebuildRows();
    emit countsChanged();
}

QList<TriageController::Group> TriageController::computeGroups() const {
    const auto &cats = m_store.categories();
    QList<Category> parents, children;
    for (const auto &c : cats) (c.isParent() ? parents : children).append(c);
    std::sort(parents.begin(), parents.end(), [](const Category &a, const Category &b) { return a.createdAt < b.createdAt; });

    QList<Group> groups;
    QSet<QString> assigned;
    for (const auto &p : parents) {
        QList<EntryItem *> pe;
        for (auto *e : m_filtered) if (e->categories().contains(p.id)) pe << e;
        if (pe.isEmpty()) continue;

        QList<Category> kids;
        for (const auto &c : children) if (c.parentId == p.id) kids << c;
        std::sort(kids.begin(), kids.end(), [](const Category &a, const Category &b) { return a.createdAt < b.createdAt; });

        QList<SubGroup> subs;
        QSet<QString> subAssigned;
        for (const auto &c : kids) {
            QList<EntryItem *> ce;
            for (auto *e : pe) if (e->categories().contains(c.id)) ce << e;
            if (ce.isEmpty()) continue;
            for (auto *e : ce) subAssigned.insert(e->path());
            subs.append({c.id, c.name, true, ce});
        }
        QList<EntryItem *> rest;
        for (auto *e : pe) if (!subAssigned.contains(e->path())) rest << e;
        if (!rest.isEmpty()) subs.append({p.id + "__nosub", p.name, false, rest});
        for (auto *e : pe) assigned.insert(e->path());
        groups.append({p.id, p.name, int(pe.size()), subs});
    }
    QList<EntryItem *> uncat;
    for (auto *e : m_filtered) if (!assigned.contains(e->path())) uncat << e;
    if (!uncat.isEmpty()) groups.append({"__uncat", QStringLiteral("Sin categoría"), int(uncat.size()), {{"__uncat_sub", QStringLiteral("Sin categoría"), false, uncat}}});
    return groups;
}

void TriageController::rebuildRows() {
    // chips (first two category names) for the hover strip on each card
    const auto &cats = m_store.categories();
    for (auto *e : m_store.entries()) {
        QStringList names;
        const auto ids = e->categories();
        for (int i = 0; i < qMin<qsizetype>(2, ids.size()); ++i) if (cats.contains(ids[i])) names << cats[ids[i]].name;
        e->setChips(names, ids.size() > 2 ? int(ids.size()) - 2 : 0);
    }

    QList<GalleryModel::Node> nodes;
    m_orderVisible.clear();

    auto addCards = [&](const QList<EntryItem *> &entries) {
        for (auto *e : entries) {
            GalleryModel::Node n;
            n.kind = GalleryModel::Card;
            n.key = "c:" + e->path();
            n.entry = e;
            nodes << n;
            m_orderVisible << e;
        }
    };

    if (m_grouped) {
        for (const auto &g : m_groups) {
            GalleryModel::Node h;
            h.kind = GalleryModel::GroupHeader;
            h.key = "g:" + g.id;
            h.title = g.name; h.groupId = g.id; h.count = g.count;
            h.collapsed = m_collapsed.contains(g.id);
            for (const auto &s : g.subs) if (s.isProduct) ++h.subCount;
            nodes << h;
            if (h.collapsed) continue;
            for (const auto &s : g.subs) {
                if (s.isProduct) {
                    GalleryModel::Node sh;
                    sh.kind = GalleryModel::SubHeader;
                    sh.key = "s:" + s.id;
                    sh.title = s.name; sh.groupId = s.id; sh.count = int(s.entries.size());
                    nodes << sh;
                }
                addCards(s.entries);
            }
        }
    } else {
        addCards(m_filtered);
    }
    m_gallery.setNodes(nodes);
}

QVariantList TriageController::filterCounts() const { QVariantList l; for (int c : m_filterCounts) l << c; return l; }
QVariantList TriageController::statusCounts() const { QVariantList l; for (int c : m_statusCounts) l << c; return l; }
int TriageController::missingCount() const {
    int n = 0;
    for (auto *e : m_store.entries()) if (e->missing()) ++n;
    return n;
}

void TriageController::setFilter(int f) { if (f != m_filter) { m_filter = f; recompute(); emit filterChanged(); } }
void TriageController::setSort(int s) { if (s != m_sort) { m_sort = s; recompute(); emit sortChanged(); } }
void TriageController::setSearch(const QString &s) { if (s != m_search) { m_search = s; recompute(); emit searchChanged(); } }
void TriageController::setGridSize(double v) { if (v != m_gridSize) { m_gridSize = v; Prefs::setSorterGridSize(v); emit gridSizeChanged(); } }
void TriageController::setInspectorOpen(bool o) { if (o != m_inspectorOpen) { m_inspectorOpen = o; Prefs::setSorterInspectorOpen(o); emit inspectorOpenChanged(); } }
void TriageController::setFocusNote(bool f) { if (f != m_focusNote) { m_focusNote = f; emit focusNoteChanged(); } }

void TriageController::stepGrid(int dir) {
    int i = int(kGridSizes.indexOf(m_gridSize));
    if (i < 0) { setGridSize(160); return; }
    setGridSize(kGridSizes[qBound(0, i + dir, int(kGridSizes.size()) - 1)]);
}

// ── Selection ────────────────────────────────────────────────────────────
void TriageController::setAnchor(const QString &path) {
    if (path == m_anchor) return;
    if (auto *old = m_store.entry(m_anchor)) old->setPrimary(false);
    m_anchor = path;
    if (auto *e = m_store.entry(m_anchor)) e->setPrimary(true);
}

void TriageController::applySelectionFlags(const QSet<QString> &before) {
    for (const auto &p : before) if (!m_selected.contains(p)) if (auto *e = m_store.entry(p)) e->setSelected(false);
    for (const auto &p : std::as_const(m_selected)) if (!before.contains(p)) if (auto *e = m_store.entry(p)) e->setSelected(true);
}

void TriageController::click(const QString &path, bool command, bool shift) {
    const QSet<QString> before = m_selected;
    if (command) {
        if (m_selected.contains(path)) m_selected.remove(path); else m_selected.insert(path);
        setAnchor(path);
    } else if (shift && !m_anchor.isEmpty()) {
        int i = -1, j = -1;
        for (int k = 0; k < m_orderVisible.size(); ++k) {
            if (m_orderVisible[k]->path() == m_anchor) i = k;
            if (m_orderVisible[k]->path() == path) j = k;
        }
        if (i >= 0 && j >= 0) {
            m_selected.clear();
            for (int k = qMin(i, j); k <= qMax(i, j); ++k) m_selected.insert(m_orderVisible[k]->path());
        } else { m_selected = {path}; setAnchor(path); }
    } else {
        m_selected = {path};
        setAnchor(path);
    }
    applySelectionFlags(before);
    emit selectionChanged();
}

void TriageController::selectAll() {
    const QSet<QString> before = m_selected;
    m_selected.clear();
    for (auto *e : std::as_const(m_filtered)) m_selected.insert(e->path());
    applySelectionFlags(before);
    emit selectionChanged();
}

void TriageController::clearSelection() {
    const QSet<QString> before = m_selected;
    m_selected.clear();
    setAnchor({});
    applySelectionFlags(before);
    emit selectionChanged();
}

void TriageController::setSelection(const QStringList &paths, bool) {
    const QSet<QString> before = m_selected;
    m_selected = QSet<QString>(paths.begin(), paths.end());
    if (m_anchor.isEmpty() || !m_selected.contains(m_anchor)) {
        QString first;
        for (auto *e : std::as_const(m_orderVisible)) if (m_selected.contains(e->path())) { first = e->path(); break; }
        setAnchor(first);
    }
    applySelectionFlags(before);
    emit selectionChanged();
}

QStringList TriageController::contextTarget(const QString &path) const {
    if (m_selected.size() > 1 && m_selected.contains(path)) return QStringList(m_selected.begin(), m_selected.end());
    return {path};
}

void TriageController::moveAnchor(int dx, int dy) {
    if (m_orderVisible.isEmpty()) return;
    int i = -1;
    for (int k = 0; k < m_orderVisible.size(); ++k) if (m_orderVisible[k]->path() == m_anchor) { i = k; break; }
    const int next = qBound(0, i + dx + dy * qMax(1, m_gallery.columns()), int(m_orderVisible.size()) - 1);
    const QString p = m_orderVisible[next]->path();
    const QSet<QString> before = m_selected;
    m_selected = {p};
    setAnchor(p);
    applySelectionFlags(before);
    emit selectionChanged();
    emit scrollToPath(p);
    if (m_focusIndex >= 0) {
        for (int k = 0; k < m_orderAll.size(); ++k) if (m_orderAll[k]->path() == p) { m_focusIndex = k; emit focusChanged(); break; }
    }
}

// ── Mutations ────────────────────────────────────────────────────────────
void TriageController::setStatus(const QStringList &paths, int s) {
    if (s < 0 || s > 4) return;
    for (const auto &p : paths) if (auto *e = m_store.entry(p)) { e->setStatus(s); e->touchUpdated(); }
    m_store.touch();
}

void TriageController::setStatusSelection(int s) {
    if (m_selected.size() > 1) setStatus(QStringList(m_selected.begin(), m_selected.end()), s);
    else if (!m_anchor.isEmpty()) setStatus({m_anchor}, s);
}

// Bulk-safe toggle: everything -> target unless the whole selection already is, then back to unsorted.
void TriageController::toggleStatus(const QStringList &paths, int target) {
    bool all = true;
    for (const auto &p : paths) { auto *e = m_store.entry(p); if (!e || e->status() != target) { all = false; break; } }
    setStatus(paths, all ? int(Unsorted) : target);
}

void TriageController::setNote(const QString &path, const QString &note) {
    if (auto *e = m_store.entry(path)) { e->setNote(note.left(4000)); e->touchUpdated(); m_store.touch(); }
}

void TriageController::toggleCategory(const QString &path, const QString &id) {
    auto *e = m_store.entry(path);
    const auto &cats = m_store.categories();
    if (!e || !cats.contains(id)) return;
    const Category cat = cats[id];
    QString currentParent;
    for (const auto &c : e->categories()) if (cats.contains(c) && cats[c].isParent()) { currentParent = c; break; }

    QStringList next;
    if (cat.isParent()) {
        if (currentParent != id) next = {id};
    } else {
        const QString parent = currentParent.isEmpty() ? cat.parentId : currentParent;
        QString currentSub;
        for (const auto &c : e->categories()) if (cats.contains(c) && cats[c].parentId == cat.parentId) { currentSub = c; break; }
        next = currentSub == id ? QStringList{parent} : QStringList{parent, id};
    }
    QStringList valid;
    for (const auto &c : next) if (cats.contains(c)) valid << c;
    e->setCategories(valid);
    e->touchUpdated();
    m_store.touch();
}

QString TriageController::addCategory(const QString &name, const QString &parentId) { return m_store.addCategory(name, parentId); }
void TriageController::renameCategory(const QString &id, const QString &name) { m_store.renameCategory(id, name); }
void TriageController::deleteCategory(const QString &id) { m_store.deleteCategory(id); }

QVariantList TriageController::parentCategories() const {
    QList<Category> ps;
    for (const auto &c : m_store.categories()) if (c.isParent()) ps << c;
    std::sort(ps.begin(), ps.end(), [](const Category &a, const Category &b) { return a.createdAt < b.createdAt; });
    QVariantList out;
    for (const auto &c : ps) out << QVariantMap{{"id", c.id}, {"name", c.name}};
    return out;
}

QVariantList TriageController::subCategories(const QString &parentId) const {
    QList<Category> ps;
    for (const auto &c : m_store.categories()) if (c.parentId == parentId && !parentId.isEmpty()) ps << c;
    std::sort(ps.begin(), ps.end(), [](const Category &a, const Category &b) { return a.createdAt < b.createdAt; });
    QVariantList out;
    for (const auto &c : ps) out << QVariantMap{{"id", c.id}, {"name", c.name}};
    return out;
}

QString TriageController::categoryOf(const QString &path) const {
    auto *e = m_store.entry(path);
    if (!e) return {};
    const auto &cats = m_store.categories();
    for (const auto &c : e->categories()) if (cats.contains(c) && cats[c].isParent()) return c;
    return {};
}

void TriageController::toggleGroup(const QString &groupId) {
    if (m_collapsed.contains(groupId)) m_collapsed.remove(groupId); else m_collapsed.insert(groupId);
    rebuildRows();
}

void TriageController::purgeMissing() { m_store.purgeMissing(); }

void TriageController::trashDiscarded() {
    QStringList discarded;
    for (auto *e : m_store.entries()) if (e->status() == Discard) discarded << e->path();
    int moved = 0;
    for (const auto &p : discarded) {
        if (!QFileInfo::exists(p) || Mac::trashItem(p)) { m_store.remove(p); ++moved; }
    }
    emit toast(QStringLiteral("%1 file%2 moved to the Trash").arg(moved).arg(moved == 1 ? "" : "s"), 1);
}

// ── Focus ────────────────────────────────────────────────────────────────
EntryItem *TriageController::focusEntry() const {
    return m_focusIndex >= 0 && m_focusIndex < m_orderAll.size() ? m_orderAll[m_focusIndex] : nullptr;
}

void TriageController::applyFocusSelection() {
    if (auto *e = focusEntry()) {
        const QSet<QString> before = m_selected;
        m_selected = {e->path()};
        setAnchor(e->path());
        applySelectionFlags(before);
        emit selectionChanged();
    }
}

void TriageController::openFocus(const QString &path) {
    const QString p = path.isEmpty() ? m_anchor : path;
    for (int i = 0; i < m_orderAll.size(); ++i) {
        if (m_orderAll[i]->path() == p) {
            m_focusIndex = i;
            m_focusNote = false;
            applyFocusSelection();
            emit focusChanged();
            return;
        }
    }
}

void TriageController::closeFocus() {
    if (m_focusIndex < 0) return;
    const QString p = m_anchor;
    m_focusIndex = -1;
    emit focusChanged();
    if (!p.isEmpty()) emit scrollToPath(p);
}

void TriageController::focusNavigate(int delta) {
    const int n = m_focusIndex + delta;
    if (m_focusIndex < 0 || n < 0 || n >= m_orderAll.size()) return;
    m_focusIndex = n;
    m_focusNote = false;
    applyFocusSelection();
    emit focusChanged();
}

void TriageController::focusStatus(int s) {
    auto *cur = focusEntry();
    if (!cur) return;
    const int i = m_focusIndex;
    const QString path = cur->path();
    setStatus({path}, s);                // store.touch() recomputes the lists synchronously
    // Auto-advance; when the filter drops the entry the same index already points at the next one
    const bool stillThere = i < m_orderAll.size() && m_orderAll[i]->path() == path;
    if (stillThere) focusNavigate(1);
    else if (i >= m_orderAll.size()) { m_focusIndex = m_orderAll.isEmpty() ? -1 : int(m_orderAll.size()) - 1; emit focusChanged(); }
    applyFocusSelection();
}

// ── Files ────────────────────────────────────────────────────────────────
void TriageController::reveal(const QString &path) { Mac::revealInFinder(path); }
void TriageController::open(const QString &path) { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); }
QString TriageController::mediaInfo(const QString &path) const { return Mac::mediaInfo(path); }

void TriageController::exportEntry(const QString &path, const QStringList &sizeKeys, const QUrl &folder) {
    if (!folder.isLocalFile() || sizeKeys.isEmpty()) return;
    const QString dir = folder.toLocalFile();
    struct Size { QString key; int w, h; };
    static const QList<Size> all = {{"box", 1080, 1080}, {"vertical", 1080, 1920}};
    QList<Size> chosen;
    for (const auto &s : all) if (sizeKeys.contains(s.key)) chosen << s;
    const QString base = QFileInfo(path).completeBaseName();
    (void)QtConcurrent::run([=, this] {
        QStringList saved;
        QString error;
        for (const auto &s : chosen) {
            const QByteArray data = Mac::composeCover(path, s.w, s.h, 0.95);
            if (data.isEmpty()) { error = QStringLiteral("No se pudo componer %1×%2").arg(s.w).arg(s.h); break; }
            const QString out = QStringLiteral("%1/%2_%3×%4.jpg").arg(dir, base).arg(s.w).arg(s.h);
            QFile f(out);
            if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size()) { error = QStringLiteral("No se pudo guardar %1").arg(QFileInfo(out).fileName()); break; }
            saved << out;
        }
        QMetaObject::invokeMethod(this, [=, this] {
            if (!error.isEmpty()) { emit toast(error, 2); return; }
            emit toast(QStringLiteral("%1 file%2 exported").arg(saved.size()).arg(saved.size() == 1 ? "" : "s"), 1);
            if (!saved.isEmpty()) Mac::revealInFinder(saved.first());
        }, Qt::QueuedConnection);
    });
}
