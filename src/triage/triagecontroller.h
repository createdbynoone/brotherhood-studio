#pragma once
#include <QObject>
#include <QSet>
#include <QUrl>
#include <QVariantList>

#include "entryitem.h"
#include "gallerymodel.h"
#include "store.h"
#include "watcher.h"

// The Sorter pane's state and actions. Entries are live EntryItem objects; the grid is a GalleryModel
// rebuilt (and diffed) only when the library, the filters or the column count actually change.
class TriageController : public QObject {
    Q_OBJECT
    Q_PROPERTY(GalleryModel *gallery READ gallery CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(int filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(double gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(bool inspectorOpen READ inspectorOpen WRITE setInspectorOpen NOTIFY inspectorOpenChanged)
    Q_PROPERTY(QVariantList filterCounts READ filterCounts NOTIFY countsChanged)     // All, Unsorted, Keep, Maybe, Discard, Archived
    Q_PROPERTY(QVariantList statusCounts READ statusCounts NOTIFY countsChanged)     // Unsorted, Keep, Maybe, Discard, Archived
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int visibleCount READ visibleCount NOTIFY countsChanged)
    Q_PROPERTY(int missingCount READ missingCount NOTIFY countsChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(EntryItem *anchorEntry READ anchorEntry NOTIFY selectionChanged)
    Q_PROPERTY(int focusIndex READ focusIndex NOTIFY focusChanged)
    Q_PROPERTY(int focusCount READ focusCount NOTIFY focusChanged)
    Q_PROPERTY(EntryItem *focusEntry READ focusEntry NOTIFY focusChanged)
    Q_PROPERTY(bool focusNote READ focusNote WRITE setFocusNote NOTIFY focusNoteChanged)
    Q_PROPERTY(bool grouped READ grouped NOTIFY countsChanged)
    Q_PROPERTY(QString watchPath READ watchPath NOTIFY watchPathChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    explicit TriageController(QObject *parent = nullptr);

    Store *store() { return &m_store; }
    GalleryModel *gallery() { return &m_gallery; }
    int revision() const { return m_store.revision(); }
    int filter() const { return m_filter; }
    void setFilter(int f);
    int sort() const { return m_sort; }
    void setSort(int s);
    QString search() const { return m_search; }
    void setSearch(const QString &s);
    double gridSize() const { return m_gridSize; }
    void setGridSize(double v);
    bool inspectorOpen() const { return m_inspectorOpen; }
    void setInspectorOpen(bool o);
    int columns() const { return m_gallery.columns(); }
    QVariantList filterCounts() const;
    QVariantList statusCounts() const;
    int totalCount() const { return int(m_store.entries().size()); }
    int visibleCount() const { return int(m_filtered.size()); }
    int missingCount() const;
    int selectedCount() const { return int(m_selected.size()); }
    EntryItem *anchorEntry() const { return m_store.entry(m_anchor); }
    int focusIndex() const { return m_focusIndex; }
    int focusCount() const { return int(m_orderAll.size()); }
    EntryItem *focusEntry() const;
    bool focusNote() const { return m_focusNote; }
    void setFocusNote(bool f);
    bool grouped() const { return m_grouped; }
    QString watchPath() const { return m_watchPath; }
    bool scanning() const { return m_scanning; }

    Q_INVOKABLE void boot();
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void importPaths(const QList<QUrl> &urls);
    Q_INVOKABLE void importFolder(const QUrl &folder);
    Q_INVOKABLE void ingest(const QString &path);
    void flushNow() { m_store.flushSync(); }

    // selection
    Q_INVOKABLE void click(const QString &path, bool command, bool shift);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void setSelection(const QStringList &paths, bool additive);   // marquee
    Q_INVOKABLE void moveAnchor(int dx, int dy);
    Q_INVOKABLE QStringList contextTarget(const QString &path) const;
    Q_INVOKABLE QStringList selectedPaths() const { return QStringList(m_selected.begin(), m_selected.end()); }

    // mutations
    Q_INVOKABLE void setStatus(const QStringList &paths, int status);
    Q_INVOKABLE void setStatusSelection(int status);
    Q_INVOKABLE void toggleStatus(const QStringList &paths, int status);
    Q_INVOKABLE void setNote(const QString &path, const QString &note);
    Q_INVOKABLE void toggleCategory(const QString &path, const QString &categoryId);
    Q_INVOKABLE QString addCategory(const QString &name, const QString &parentId);
    Q_INVOKABLE void renameCategory(const QString &id, const QString &name);
    Q_INVOKABLE void deleteCategory(const QString &id);
    Q_INVOKABLE QVariantList parentCategories() const;
    Q_INVOKABLE QVariantList subCategories(const QString &parentId) const;
    Q_INVOKABLE QString categoryOf(const QString &path) const;      // active parent id for an entry
    Q_INVOKABLE void toggleGroup(const QString &groupId);
    Q_INVOKABLE void purgeMissing();
    Q_INVOKABLE void trashDiscarded();

    // focus
    Q_INVOKABLE void openFocus(const QString &path = {});
    Q_INVOKABLE void closeFocus();
    Q_INVOKABLE void focusNavigate(int delta);
    Q_INVOKABLE void focusStatus(int status);

    // files
    Q_INVOKABLE void reveal(const QString &path);
    Q_INVOKABLE void open(const QString &path);
    Q_INVOKABLE QString mediaInfo(const QString &path) const;
    Q_INVOKABLE void exportEntry(const QString &path, const QStringList &sizeKeys, const QUrl &folder);
    Q_INVOKABLE void stepGrid(int dir);

signals:
    void revisionChanged();
    void filterChanged();
    void sortChanged();
    void searchChanged();
    void gridSizeChanged();
    void inspectorOpenChanged();
    void countsChanged();
    void selectionChanged();
    void focusChanged();
    void focusNoteChanged();
    void watchPathChanged();
    void scanningChanged();
    void toast(const QString &message, int kind);
    void scrollToPath(const QString &path);      // keep the anchor visible after keyboard navigation

private:
    void onStoreChanged();
    void recompute();                 // filtered list, groups, rows, counts
    void rebuildRows();
    void startWatcher();
    void markNew(const QList<EntryItem *> &added);
    void applySelectionFlags(const QSet<QString> &before);
    void setAnchor(const QString &path);
    void applyFocusSelection();

    struct SubGroup { QString id, name; bool isProduct; QList<EntryItem *> entries; };
    struct Group { QString id, name; int count; QList<SubGroup> subs; };

    QList<Group> computeGroups() const;

    Store m_store;
    GalleryModel m_gallery;
    FolderWatcher *m_watcher = nullptr;

    int m_filter = Triage::All, m_sort = Triage::Newest;
    QString m_search;
    double m_gridSize = 160;
    bool m_inspectorOpen = true;
    QString m_watchPath;
    bool m_scanning = false;
    bool m_booted = false;

    QList<EntryItem *> m_filtered;        // filtered + sorted
    QList<Group> m_groups;                // empty unless grouped
    bool m_grouped = false;
    QList<EntryItem *> m_orderAll;        // navigation order for Focus (grouped order when grouped)
    QList<EntryItem *> m_orderVisible;    // navigation order for the grid (collapsed groups skipped)
    QList<int> m_filterCounts = QList<int>(6, 0);
    QList<int> m_statusCounts = QList<int>(5, 0);
    QSet<QString> m_collapsed;
    int m_lastRevision = -1;

    QSet<QString> m_selected;
    QString m_anchor;
    int m_focusIndex = -1;
    bool m_focusNote = false;
};
