#include "ecomcontroller.h"

#include "../core/err.h"
#include "../core/prefs.h"
#include "../platform/mac.h"
#include "../prompt/claude.h"
#include "../prompt/downloader.h"
#include "../prompt/higgsfield.h"
#include "../prompt/providers.h"
#include "../prompt/staging.h"
#include "ecomprompts.h"
#include "ecomstyle.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTimer>
#include <QtConcurrent>
#include <QtMath>

namespace {

const QSet<QString> &imageExts() {
    static const QSet<QString> s = {"jpg", "jpeg", "png", "webp", "heic", "heif", "tif", "tiff", "gif", "bmp", "avif"};
    return s;
}

QStringList imageFiles(const QList<QUrl> &urls) {
    QStringList out;
    for (const QUrl &u : urls) {
        if (!u.isLocalFile()) continue;
        const QString p = u.toLocalFile();
        if (imageExts().contains(QFileInfo(p).suffix().toLower()) && !out.contains(p)) out << p;
    }
    return out;
}

// Cuts a top-level key out of a JSON object text WITHOUT re-serialising it (QJson would sort the keys, and the
// order matters: task first, final_instruction last). `value` gets the key's value text; the rest is returned.
QString cutKey(const QString &json, const QString &key, QString *value) {
    value->clear();
    const QString needle = '"' + key + '"';
    int depth = 0;
    bool inStr = false;
    for (int i = 0; i < json.size(); ++i) {
        const QChar c = json[i];
        if (inStr) { if (c == '\\') ++i; else if (c == '"') inStr = false; continue; }
        if (c == '"') {
            if (depth == 1 && json.mid(i, needle.size()) == needle) {
                int j = i + needle.size();
                while (j < json.size() && json[j].isSpace()) ++j;
                if (j < json.size() && json[j] == ':') {
                    ++j;
                    while (j < json.size() && json[j].isSpace()) ++j;
                    int k = j, d = 0;
                    bool s2 = false;
                    for (; k < json.size(); ++k) {
                        const QChar ch = json[k];
                        if (s2) { if (ch == '\\') ++k; else if (ch == '"') s2 = false; continue; }
                        if (ch == '"') s2 = true;
                        else if (ch == '{' || ch == '[') ++d;
                        else if (ch == '}' || ch == ']') { if (--d == 0) { ++k; break; } }
                    }
                    *value = json.mid(j, k - j);
                    int from = i, b = i - 1;
                    while (b >= 0 && json[b].isSpace()) --b;
                    if (b >= 0 && json[b] == ',') from = b;
                    else { int a = k; while (a < json.size() && json[a].isSpace()) ++a; if (a < json.size() && json[a] == ',') k = a + 1; }
                    return json.left(from) + json.mid(k);
                }
            }
            inStr = true;
            continue;
        }
        if (c == '{' || c == '[') ++depth;
        else if (c == '}' || c == ']') --depth;
    }
    return json;
}

// Caps, hats and the like have no back shot
bool isAccessory(const QString &type) {
    static const QSet<QString> s = {"cap", "bucket-hat", "beanie", "bag", "socks"};
    return s.contains(type.toLower());
}

QString firstImage(const QList<QUrl> &urls) {
    for (const QUrl &u : urls) {
        if (!u.isLocalFile()) continue;
        const QString p = u.toLocalFile();
        if (imageExts().contains(QFileInfo(p).suffix().toLower())) return p;
    }
    return {};
}

const QString kRatio = QStringLiteral("1:1");

}  // namespace

EcomController::EcomController(QObject *parent) : QObject(parent) {
    for (int i = 0; i < providers().size(); ++i)
        if (providers().at(i).jobType == QLatin1String("gpt_image_2_5")) m_provider = i;
    m_pool.setMaxThreadCount(12);
    connect(&m_log, &QAbstractItemModel::rowsInserted, this, [this] { emit logCountChanged(); });
    connect(&m_log, &QAbstractItemModel::rowsRemoved, this, [this] { emit logCountChanged(); });
    connect(&m_log, &QAbstractItemModel::modelReset, this, [this] { emit logCountChanged(); });
    m_pool.start([this] { EcomStyle::seed(); const int n = EcomStyle::count(); post([this, n] { m_styleCount = n; emit styleChanged(); }); });
}

void EcomController::refreshStyle() { m_styleCount = EcomStyle::count(); emit styleChanged(); }
QString EcomController::stylePath() const { return EcomStyle::dir(); }

void EcomController::post(std::function<void()> fn) { QMetaObject::invokeMethod(this, std::move(fn), Qt::QueuedConnection); }
void EcomController::push(const QString &line) { m_log.append(line); }

QString EcomController::resultsPath() const { return Prefs::outputPath() + "/Ecommerce"; }

// ── Engine settings ──────────────────────────────────────────────────────
QStringList EcomController::providerNames() const { return {providers().at(m_provider).shortLabel}; }
QString EcomController::providerLabel() const { return providers().at(m_provider).label; }
QStringList EcomController::ratios() const { return {kRatio}; }
QStringList EcomController::resolutions() const { return providers().at(m_provider).resolutions; }
QStringList EcomController::qualities() const { return providers().at(m_provider).qualities; }

// Ecommerce is fixed to one engine (GPT Image 2.5 Sunburst) and one canvas (1:1): the house catalog photo on a square
QString EcomController::effectiveRatio() const { return kRatio; }

void EcomController::scheduleCost() {
    m_costKnown = false; m_cost = 0;
    emit costChanged();
    if (!m_costTimer) {
        m_costTimer = new QTimer(this);
        m_costTimer->setSingleShot(true);
        m_costTimer->setInterval(300);
        connect(m_costTimer, &QTimer::timeout, this, &EcomController::estimateCost);
    }
    m_costTimer->start();
}

void EcomController::estimateCost() {
    if (!Higgsfield::isInstalled()) return;
    const int provider = m_provider;
    const QString ratio = effectiveRatio(), res = m_resolution, quality = m_quality;
    const int ticket = ++m_costTicket;
    m_pool.start([=, this] {
        const Provider &p = providers().at(provider);
        const double c = Higgsfield::cost(p.jobType, priceParams(p, ratio, res, quality, true));
        post([=, this] {
            if (ticket != m_costTicket) return;
            m_costKnown = c >= 0; m_cost = c > 0 ? c : 0;
            emit costChanged();
        });
    });
}

void EcomController::setProviderIndex(int) {}   // fixed: GPT Sunburst only
void EcomController::setAspectRatio(const QString &) {}   // fixed: 1:1 only
void EcomController::setResolution(const QString &r) { if (m_resolution != r) { m_resolution = r; emit fireSettingsChanged(); scheduleCost(); } }
void EcomController::setQuality(const QString &q) { if (m_quality != q && providers().at(m_provider).qualities.contains(q)) { m_quality = q; emit fireSettingsChanged(); scheduleCost(); } }
void EcomController::setVariations(int n) { n = qBound(1, n, 4); if (m_variations != n) { m_variations = n; emit fireSettingsChanged(); emit costChanged(); } }
void EcomController::setNotes(const QString &n) { if (m_notes != n) { m_notes = n; emit notesChanged(); } }

// ── JSON ─────────────────────────────────────────────────────────────────
bool EcomController::jsonValid() const { return QJsonDocument::fromJson(m_json.toUtf8()).isObject(); }

void EcomController::setJson(const QString &j) {
    if (m_json == j) return;
    m_json = j;
    const auto doc = QJsonDocument::fromJson(j.toUtf8());
    m_backSource.clear();
    if (doc.isObject()) {
        m_productType = doc.object().value("product").toObject().value("type").toString();
        m_backSource = doc.object().value("back_spec").toObject().value("back_source").toString();
    }
    emit jsonChanged();
    emit backChanged();
}

QString EcomController::photoInfo() const {
    if (m_photos.size() > 1) return QStringLiteral("%1 photos · main %2 × %3 px").arg(m_photos.size()).arg(m_photoW).arg(m_photoH);
    return m_photoW > 0 ? QStringLiteral("%1 × %2 px").arg(m_photoW).arg(m_photoH) : QString();
}

bool EcomController::backAllowed() const { return !isAccessory(m_productType); }
bool EcomController::backStale() const {
    return m_shootBack && backAllowed() && !m_json.trimmed().isEmpty() && !QJsonDocument::fromJson(m_json.toUtf8()).object().contains("back_spec");
}
void EcomController::setShootBack(bool on) {
    if (m_shootBack == on) return;
    m_shootBack = on;
    emit backChanged(); emit fireSettingsChanged(); emit costChanged();
}

// ── Inputs ───────────────────────────────────────────────────────────────
// Several garment photos (angles and close-ups) and one layout reference: a new layout drop replaces the old one.
// Changing the garment photos makes the JSON stale.
void EcomController::stageInto(const QList<QUrl> &urls, bool layout) {
    QStringList files = imageFiles(urls);
    if (files.isEmpty()) return;
    if (layout) files = files.mid(0, 1);
    else {
        const int room = kMaxPhotos - int(m_photos.size());
        if (room <= 0) { emit toast(QStringLiteral("Up to %1 product photos").arg(kMaxPhotos), 2); return; }
        if (files.size() > room) { files = files.mid(0, room); emit toast(QStringLiteral("Up to %1 product photos: the rest was left out").arg(kMaxPhotos), 0); }
    }
    m_pool.start([this, files, layout] {
        try {
            const QStringList staged = Staging::stage(files);
            const auto props = Mac::imageProps(staged.first());
            post([this, staged, props, layout] {
                if (layout) { m_layoutRefs = staged; emit layoutRefsChanged(); }
                else {
                    if (m_photos.isEmpty()) { m_photoW = props.ok ? props.w : 0; m_photoH = props.ok ? props.h : 0; }
                    m_photos += staged;
                    m_json.clear(); m_productType.clear(); m_backSource.clear(); m_analyzeStatus = 0; m_analyzeError.clear();
                    emit photosChanged(); emit jsonChanged(); emit analyzeChanged(); emit fireSettingsChanged(); emit backChanged();
                    scheduleCost();
                }
                emit inputsChanged();
            });
        } catch (const Err &e) {
            post([this, msg = e.msg] { push("Error: " + msg); });
        }
    });
}

void EcomController::addPhoto(const QList<QUrl> &urls) { stageInto(urls, false); }
void EcomController::addLayoutRef(const QList<QUrl> &urls) { stageInto(urls, true); }

void EcomController::removePhoto(int index) {
    if (index < 0 || index >= m_photos.size()) return;
    m_photos.removeAt(index);
    if (m_photos.isEmpty()) m_photoW = m_photoH = 0;
    else if (index == 0) { const auto p = Mac::imageProps(m_photos.first()); m_photoW = p.ok ? p.w : 0; m_photoH = p.ok ? p.h : 0; }
    m_json.clear(); m_productType.clear(); m_backSource.clear(); m_analyzeStatus = 0; m_analyzeError.clear();
    emit photosChanged(); emit jsonChanged(); emit analyzeChanged(); emit inputsChanged(); emit fireSettingsChanged(); emit backChanged();
    scheduleCost();
}

void EcomController::removeLayoutRef() {
    if (m_layoutRefs.isEmpty()) return;
    m_layoutRefs.clear();
    emit layoutRefsChanged(); emit inputsChanged();
}

// ── Analysis (Opus) ──────────────────────────────────────────────────────
void EcomController::analyze() {
    if (!canAnalyze()) return;
    m_analyzeStatus = 1; m_analyzeError.clear();
    emit analyzeChanged(); emit inputsChanged();
    push(QStringLiteral("▶ Opus · reading %1 and writing the JSON%2...").arg(m_photos.size() > 1 ? QStringLiteral("%1 photos").arg(m_photos.size()) : QStringLiteral("the photo"), shots() > 1 ? QStringLiteral(" (front and back)") : QString()));
    const QStringList photos = m_photos;
    const QString layout = m_layoutRefs.value(0), notes = m_notes;
    const int w = m_photoW, h = m_photoH;
    const bool wantBack = m_shootBack && backAllowed();
    m_pool.start([=, this] {
        try {
            const QString spec = ClaudeCli::generateEcomSpec(photos, layout, notes, w, h, wantBack);
            post([=, this] {
                m_analyzeStatus = 2;
                setJson(spec);
                emit analyzeChanged(); emit inputsChanged();
                push(QStringLiteral("JSON ready ✓ · %1").arg(m_productType.isEmpty() ? QStringLiteral("product type unknown") : m_productType));
                if (!m_backSource.isEmpty()) push(m_backSource == QLatin1String("photo") ? QStringLiteral("Back view: measured from your photos")
                                                                                          : QStringLiteral("Back view: inferred from the front (no photo shows the back)"));
            });
        } catch (const Err &e) {
            post([=, this, msg = e.msg] {
                m_analyzeStatus = 3; m_analyzeError = msg;
                emit analyzeChanged(); emit inputsChanged();
                push("JSON failed: " + msg);
            });
        }
    });
}

// ── Fire ─────────────────────────────────────────────────────────────────
void EcomController::fire() {
    if (!canFire()) return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastFire < 600) return;
    m_lastFire = now;
    if (!jsonValid()) { emit toast(QStringLiteral("The JSON is not valid. Fix it or run Analyze again"), 2); return; }

    const QString whole = m_json.trimmed();
    const auto root = QJsonDocument::fromJson(whole.toUtf8()).object();
    const bool wantBack = m_shootBack && backAllowed();
    QString backText;
    const QString frontText = cutKey(whole, QStringLiteral("back_spec"), &backText);
    if (wantBack && backText.isEmpty()) { emit toast(QStringLiteral("The JSON has no back view yet. Run Analyze again"), 2); return; }

    // Which photo shows what (Opus lists one entry per photo in product.photo_views): the primary reference of each shot goes first
    QStringList views;
    for (const auto &v : root.value("product").toObject().value("photo_views").toArray()) views << v.toString().trimmed().toLower();
    auto viewIndex = [&](const QString &w) { for (int i = 0; i < qMin(views.size(), int(m_photos.size())); ++i) if (views[i].startsWith(w)) return i; return -1; };
    const int frontIdx = qMax(0, viewIndex(QStringLiteral("front")));
    const int backIdx = viewIndex(QStringLiteral("back"));
    auto ordered = [&](int primary) { QStringList r{m_photos.at(primary)}; for (int i = 0; i < m_photos.size(); ++i) if (i != primary) r << m_photos.at(i); return r; };

    // Layout reference: the one the user chose, else the library photo Opus matched (checked to live inside the library)
    const QString userLayout = m_layoutRefs.value(0);
    auto layoutOf = [&](const QJsonObject &o) { return userLayout.isEmpty() ? EcomStyle::resolve(o.value("catalog_layout").toObject().value("layout_reference_file").toString()) : userLayout; };
    const QString layoutFront = layoutOf(root);
    QString layoutBack = wantBack ? layoutOf(QJsonDocument::fromJson(backText.toUtf8()).object()) : QString();
    if (layoutBack.isEmpty()) layoutBack = layoutFront;

    struct Shot { QString view, prompt; QStringList refs; };
    QList<Shot> shotList;
    shotList << Shot{QStringLiteral("front"), EcomPrompts::preamble(!layoutFront.isEmpty(), m_photos.size(), false, true) + frontText,
                     ordered(frontIdx) + (layoutFront.isEmpty() ? QStringList() : QStringList{layoutFront})};
    if (wantBack)
        shotList << Shot{QStringLiteral("back"), EcomPrompts::preamble(!layoutBack.isEmpty(), m_photos.size(), true, backIdx >= 0) + backText,
                         ordered(backIdx >= 0 ? backIdx : frontIdx) + (layoutBack.isEmpty() ? QStringList() : QStringList{layoutBack})};
    const bool hasLayout = !layoutFront.isEmpty();
    const QString layout = layoutFront;
    const QString ratio = effectiveRatio(), res = m_resolution, quality = m_quality;
    const int provider = m_provider, n = m_variations;
    const int total = n * int(shotList.size());
    const Provider &pv = providers().at(provider);

    m_tasks += 1;
    emit tasksChanged();
    push(QStringLiteral("▶ %1 ×%2%3 · %4 · %5%6").arg(pv.label).arg(n).arg(wantBack ? QStringLiteral(" (front + back)") : QString()).arg(ratio, res.toUpper(),
         pv.qualities.isEmpty() ? QString() : QStringLiteral(" · %1").arg(quality)));
    if (hasLayout) push(QStringLiteral("Layout reference: %1").arg(QFileInfo(layout).completeBaseName()));

    m_pool.start([=, this] {
        try {
            QHash<QString, QString> idOf;                     // upload each file once, share across the batch
            if (total > 1) {
                QStringList unique;
                for (const Shot &sh : shotList) for (const QString &f : sh.refs) if (!unique.contains(f)) unique << f;
                post([=, this] { push(QStringLiteral("Uploading %1 image%2...").arg(unique.size()).arg(unique.size() > 1 ? "s" : "")); });
                QList<QFuture<QString>> ups;
                for (const QString &f : unique) ups << QtConcurrent::run(&m_pool, [f] { return Higgsfield::upload(f); });
                for (int i = 0; i < unique.size(); ++i) idOf.insert(unique[i], ups[i].result());
            }
            QList<QFuture<bool>> jobs;
            for (int i = 0; i < n; ++i)
                for (const Shot &sh : shotList) {
                    QStringList refs = sh.refs;
                    if (!idOf.isEmpty()) for (QString &r : refs) r = idOf.value(r, r);
                    jobs << QtConcurrent::run(&m_pool, [=, this] { return runJob(sh.prompt, refs, sh.view, ratio, res, quality, provider); });
                }
            int succeeded = 0;
            for (auto &j : jobs) if (j.result()) ++succeeded;

            post([=, this] {
                if (total > 1) push(succeeded == total ? QStringLiteral("All %1 images generated.").arg(total)
                              : succeeded == 0 ? QStringLiteral("All %1 images failed.").arg(total)
                              : QStringLiteral("%1/%2 generated, %3 failed.").arg(succeeded).arg(total).arg(total - succeeded));
                m_fireResult = succeeded > 0 ? 2 : 3;
                m_tasks -= 1;
                emit tasksChanged();
                if (succeeded > 0) emit toast(total > 1 ? QStringLiteral("%1 images saved").arg(succeeded) : QStringLiteral("Image saved"), 1);
                else emit toast(QStringLiteral("Generation failed"), 2);
            });
        } catch (const Err &e) {
            post([=, this, msg = e.msg] {
                m_fireResult = 3; m_tasks -= 1;
                emit tasksChanged();
                push("Error: " + msg);
                emit toast(msg, 2);
            });
        }
    });
}

bool EcomController::runJob(const QString &prompt, const QStringList &refsIn, const QString &view, const QString &ratio, const QString &res,
                            const QString &quality, int providerIndex) {
    const Provider &p = providers().at(providerIndex);
    const QString safeRatio = p.allRatios.contains(ratio) ? ratio : p.allRatios.first();
    const QString safeRes = p.resolutions.contains(res) ? res : p.resolutions.last();
    const QString safeQuality = p.qualities.contains(quality) ? quality : p.defaultQuality;
    const QStringList refs = refsIn.mid(0, p.maxRefs);

    const QString detail = p.qualities.isEmpty() ? QString() : QStringLiteral(" · %1").arg(safeQuality);
    post([=, this] { push(QStringLiteral("Submitting %1 %2 (%3 · %4%5)...").arg(p.jobType, view, safeRatio, safeRes.toUpper(), detail)); });
    try {
        Higgsfield::Params params = {{"prompt", prompt}, {"aspect_ratio", safeRatio}, {"resolution", safeRes},
                                     {"image_references", refs.isEmpty() ? QVariant() : QVariant(refs)}};
        if (!p.qualities.isEmpty()) params.append(qMakePair(QStringLiteral("quality"), QVariant(safeQuality)));
        for (const auto &f : p.fixed) params.append(qMakePair(f.first, QVariant(f.second)));
        post([this] { emit charged(); QTimer::singleShot(3000, this, [this] { emit charged(); }); });
        struct Done { EcomController *c; ~Done() { QMetaObject::invokeMethod(c, [c = c] { emit c->charged(); }, Qt::QueuedConnection); } } done{this};
        const auto job = Higgsfield::generate(p.jobType, params, [this](const QString &line) { post([this, line] { push(line); }); });
        if (job.resultUrl.isEmpty()) { post([this] { push(QStringLiteral("No image in response")); }); return false; }

        const QString ext = Downloader::extensionOf(job.resultUrl, "jpg");
        const QString dir = resultsPath();
        QDir().mkpath(dir);
        qint64 stamp = QDateTime::currentMSecsSinceEpoch();
        QString name, dest;
        do { name = QStringLiteral("ecom_%1%2.%3").arg(stamp++).arg(view == QLatin1String("back") ? QStringLiteral("_back") : QString(), ext); dest = dir + "/" + name; } while (QFileInfo::exists(dest));

        post([this] { push(QStringLiteral("Downloading image...")); });
        Downloader::download(job.resultUrl, dest);

        // the JSON that made the image, saved beside it (same base name) so the shot can be repeated
        const int dot = prompt.indexOf('{');
        QFile side(dest.left(dest.lastIndexOf('.')) + ".json");
        if (dot >= 0 && side.open(QIODevice::WriteOnly)) { side.write(prompt.mid(dot).toUtf8()); side.close(); }

        const auto props = Mac::imageProps(dest);
        const QString savedLine = props.ok ? QStringLiteral("Saved: %1 · %2×%3px").arg(name).arg(props.w).arg(props.h) : QStringLiteral("Saved: %1").arg(name);
        post([=, this] { push(savedLine); m_results.prepend(dest); emit resultsChanged(); emit saved(dest, p.label); });
        return true;
    } catch (const Err &e) {
        post([this, msg = e.msg] { push("Error: " + msg); });
        return false;
    }
}

// ── Reset / log ──────────────────────────────────────────────────────────
void EcomController::reset() {
    m_photos.clear(); m_layoutRefs.clear(); m_photoW = m_photoH = 0;
    m_notes.clear(); m_json.clear(); m_productType.clear(); m_backSource.clear(); m_analyzeStatus = 0; m_analyzeError.clear(); m_variations = 1;
    emit photosChanged(); emit layoutRefsChanged(); emit notesChanged(); emit jsonChanged(); emit analyzeChanged();
    emit fireSettingsChanged(); emit inputsChanged(); emit backChanged();
    if (m_tasks == 0) { m_fireResult = 0; m_log.clear(); emit tasksChanged(); }
    scheduleCost();
}

void EcomController::demo() {
    setJson(QStringLiteral("{\n  \"task\": \"ecommerce_catalog_retouch\",\n  \"product\": {\n    \"type\": \"hoodie\",\n    \"detail\": \"heavyweight oversized pullover\",\n    \"view\": \"front\",\n    \"presentation\": \"invisible mannequin\"\n  },\n  \"preserve_from_source\": {\n    \"subject_frame\": {\"left\": 0.12, \"top\": 0.06, \"width\": 0.76, \"height\": 0.88}\n  }\n}"));
    m_analyzeStatus = 2; emit analyzeChanged();
    if (m_photos.isEmpty()) {            // sample photos from the style library, only to see the pane filled
        QDirIterator it(EcomStyle::dir(), {"*.jpg"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && m_photos.size() < 3) m_photos << it.next();
        m_shootBack = true;
        emit photosChanged(); emit backChanged(); emit inputsChanged();
    }
    for (const char *l : {"▶ Opus · reading the photo and writing the JSON...", "JSON ready ✓ · hoodie", "▶ GPT Image 2.5 Sunburst ×1 · 1:1 · 2K · high", "Saved: ecom_1760541234567.jpg · 2048×2048px"})
        push(QString::fromUtf8(l));
}

void EcomController::clearLog() {
    m_log.clear();
    m_fireResult = 0;
    emit tasksChanged();
}
