#include "promptcontroller.h"

#include "../core/err.h"
#include "../core/paths.h"
#include "../core/prefs.h"
#include "../platform/mac.h"
#include "claude.h"
#include "downloader.h"
#include "higgsfield.h"
#include "memorystore.h"
#include "shell.h"
#include "staging.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QSet>
#include <QtConcurrent>

namespace {

struct Provider {
    QString shortLabel, label, jobType;
    QStringList ratios, resolutions;
    int maxRefs;
    bool thinking;
};

// Seedream 5.0 Pro has no 4:5 in its aspect_ratio enum (3:4 is its closest portrait).
// First ratio is the provider default.
const QList<Provider> &providers() {
    static const QList<Provider> p = {
        {"Seedream", "Seedream 5.0 Pro", "seedream_v5_pro", {"3:4", "9:16"}, {"1k", "2k"}, 10, false},
        {"NB Pro", "Nano Banana Pro", "nano_banana_pro", {"4:5", "9:16"}, {"1k", "2k", "4k"}, 14, false},
        {"NB 2.1", "Nano Banana 2.1", "nano_banana_2_1", {"4:5", "9:16"}, {"1k", "2k", "4k"}, 14, true},
    };
    return p;
}

const QSet<QString> &imageExts() {
    static const QSet<QString> s = {"jpg", "jpeg", "png", "webp", "heic", "heif", "tif", "tiff", "gif", "bmp", "avif"};
    return s;
}

QStringList imagePaths(const QList<QUrl> &urls) {
    QStringList out;
    for (const QUrl &u : urls) {
        if (!u.isLocalFile()) continue;
        const QString p = u.toLocalFile();
        if (imageExts().contains(QFileInfo(p).suffix().toLower())) out << p;
    }
    return out;
}

}  // namespace

PromptController::PromptController(QObject *parent) : QObject(parent) {
    m_pool.setMaxThreadCount(24);
    m_ratio = providers().at(m_provider).ratios.first();
    m_outputPath = Prefs::outputPath();
    connect(&m_log, &QAbstractItemModel::rowsInserted, this, [this] { emit logCountChanged(); emitOutputVisibility(); });
    connect(&m_log, &QAbstractItemModel::rowsRemoved, this, [this] { emit logCountChanged(); });
    connect(&m_log, &QAbstractItemModel::modelReset, this, [this] { emit logCountChanged(); emitOutputVisibility(); });
}

void PromptController::post(std::function<void()> fn) { QMetaObject::invokeMethod(this, std::move(fn), Qt::QueuedConnection); }
void PromptController::emitOutputVisibility() { emit outputVisibilityChanged(); }
void PromptController::push(const QString &line) { m_log.append(line); }

QStringList PromptController::providerNames() const {
    QStringList n;
    for (const auto &p : providers()) n << p.shortLabel;
    return n;
}
QString PromptController::providerLabel() const { return providers().at(m_provider).label; }
QStringList PromptController::ratios() const { return providers().at(m_provider).ratios; }
QStringList PromptController::resolutions() const { return providers().at(m_provider).resolutions; }

void PromptController::setProviderIndex(int i) {
    if (i < 0 || i >= providers().size() || i == m_provider) return;
    m_provider = i;
    const auto &p = providers().at(i);
    if (!p.resolutions.contains(m_resolution)) m_resolution = "2k";
    // 3:4 vs 4:5 portrait differs per provider: keep 9:16, else snap to the provider default
    if (!p.ratios.contains(m_ratio)) m_ratio = p.ratios.first();
    emit providerChanged();
    emit fireSettingsChanged();
}

void PromptController::setAspectRatio(const QString &r) { if (m_ratio != r) { m_ratio = r; emit fireSettingsChanged(); } }
void PromptController::setResolution(const QString &r) { if (m_resolution != r) { m_resolution = r; emit fireSettingsChanged(); } }
void PromptController::setVariations(int n) { n = qBound(1, n, 4); if (m_variations != n) { m_variations = n; emit fireSettingsChanged(); } }
void PromptController::setBrief(const QString &b) { if (m_brief != b) { m_brief = b; emit briefChanged(); } }
void PromptController::setRefSpecs(const QString &s) { if (m_refSpecs != s) { m_refSpecs = s; emit refSpecsChanged(); } }

// ── Boot ─────────────────────────────────────────────────────────────────
void PromptController::boot() {
    Staging::reset();
    refreshMemoryStats();
    QStringList missing;
    if (!ClaudeCli::isInstalled()) missing << "claude";
    if (!Higgsfield::isInstalled()) missing << "higgsfield";
    m_missing = missing;
    emit missingToolsChanged();
    if (!Higgsfield::isInstalled()) return;
    m_pool.start([this] {
        const bool authed = Higgsfield::isAuthenticated();
        if (!authed) {
            post([this] { m_showLogin = true; emit showLoginChanged(); });
            try { Higgsfield::login(); } catch (const Err &) {}
            post([this] { m_showLogin = false; emit showLoginChanged(); });
        }
        const auto c = Higgsfield::credits();
        post([this, c] { m_hasCredits = c.ok; m_credits = c.credits; m_plan = c.plan; emit creditsChanged(); });
    });
}

void PromptController::refreshCredits() {
    m_pool.start([this] {
        const auto c = Higgsfield::credits();
        post([this, c] { m_hasCredits = c.ok; m_credits = c.credits; m_plan = c.plan; emit creditsChanged(); });
    });
}

void PromptController::login() {
    m_showLogin = true; emit showLoginChanged();
    m_pool.start([this] {
        try { Higgsfield::login(); } catch (const Err &) {}
        const auto c = Higgsfield::credits();
        post([this, c] {
            m_showLogin = false; emit showLoginChanged();
            m_hasCredits = c.ok; m_credits = c.credits; m_plan = c.plan; emit creditsChanged();
        });
    });
}

QString PromptController::toolPath(const QString &bin) const { return Shell::resolve(bin); }

void PromptController::refreshMemoryStats() {
    MemoryStore::stats(&m_memTotal, &m_memFired);
    emit memoryChanged();
}

void PromptController::setOutputPath(const QString &path) {
    Prefs::setOutputPath(path);
    m_outputPath = path;
    emit outputPathChanged();
}

// ── Drops ────────────────────────────────────────────────────────────────
// One reference only: a second drop replaces the first (several references pull the prompt in
// different directions). Staging decodes/re-encodes, so it runs off the GUI thread.
void PromptController::addRefs(const QList<QUrl> &urls) {
    const QStringList files = imagePaths(urls);
    if (files.isEmpty()) return;
    if (files.size() > 1) push(QStringLiteral("Warning: only 1 reference is used, took %1").arg(QFileInfo(files.first()).fileName()));
    const QStringList first{files.first()};
    m_pool.start([this, first] {
        try {
            const QStringList staged = Staging::stage(first);
            post([this, staged] { m_refs = staged; emit refsChanged(); emit inputsChanged(); });
        } catch (const Err &e) {
            post([this, msg = e.msg] { push("Error: " + msg); });
        }
    });
}

void PromptController::addProducts(const QList<QUrl> &urls) {
    const QStringList files = imagePaths(urls);
    if (files.isEmpty()) return;
    m_pool.start([this, files] {
        try {
            const QStringList staged = Staging::stage(files);
            post([this, staged] { m_products += staged; emit productsChanged(); emit inputsChanged(); });
        } catch (const Err &e) {
            post([this, msg = e.msg] { push("Error: " + msg); });
        }
    });
}

void PromptController::removeRef(int i) { if (i >= 0 && i < m_refs.size()) { m_refs.removeAt(i); emit refsChanged(); emit inputsChanged(); } }
void PromptController::removeProduct(int i) { if (i >= 0 && i < m_products.size()) { m_products.removeAt(i); emit productsChanged(); emit inputsChanged(); } }

QString PromptController::inputSignature() const { return (m_refs + m_products + QStringList{m_refSpecs}).join('|'); }

// ── Brief (stage 1, Sonnet) ──────────────────────────────────────────────
void PromptController::autoWriteBrief() {
    if (!canAutoBrief()) return;
    m_briefLoading = true; m_generateError.clear();
    emit briefLoadingChanged(); emit generateChanged(); emit inputsChanged();
    push(QStringLiteral("▶ Sonnet · writing brief..."));
    const auto refs = m_refs, products = m_products;
    const auto specs = m_refSpecs, sig = inputSignature();
    m_pool.start([=, this] {
        try {
            const QString text = ClaudeCli::generateBrief(refs, products, specs);
            post([=, this] {
                m_briefLoading = false; setBrief(text);
                m_autoBriefText = text; m_autoBriefSig = sig; m_hasAutoBrief = true;
                emit briefLoadingChanged(); emit inputsChanged();
                push(QStringLiteral("Brief ready ✓"));
            });
        } catch (const Err &e) {
            post([=, this, msg = e.msg] {
                m_briefLoading = false; m_generateError = msg;
                emit briefLoadingChanged(); emit generateChanged(); emit inputsChanged();
                push("Brief failed: " + msg);
            });
        }
    });
}

// ── Prompt (stage 2, Opus) ───────────────────────────────────────────────
void PromptController::generate() {
    if (!canGenerate() || m_generateStatus == 1 || m_briefLoading) return;
    m_generateStatus = 1; m_prompt.clear(); m_generateError.clear();
    emit generateChanged(); emit promptChanged(); emit inputsChanged(); emitOutputVisibility();

    const auto refs = m_refs, products = m_products;
    const auto specs = m_refSpecs, sig = inputSignature();
    const QString typed = m_brief.trimmed();
    const bool stale = m_hasAutoBrief && m_autoBriefText == m_brief && m_autoBriefSig != sig;

    m_pool.start([=, this] {
        try {
            QString description = typed;
            if (description.isEmpty() || stale) {
                post([this] { m_generateStage = "Brief · Sonnet"; emit generateChanged(); push(QStringLiteral("▶ Sonnet · writing brief...")); });
                description = ClaudeCli::generateBrief(refs, products, specs);
                post([=, this] {
                    setBrief(description);
                    m_autoBriefText = description; m_autoBriefSig = sig; m_hasAutoBrief = true;
                    push(QStringLiteral("Brief ready ✓"));
                });
            }
            post([this] { m_generateStage = "Prompt · Opus"; emit generateChanged(); push(QStringLiteral("▶ Opus · writing prompt...")); });
            const auto result = ClaudeCli::generatePrompt(refs, products, description, specs);
            post([=, this] {
                m_prompt = result.prompt; m_memoryId = result.memoryId; m_generateStatus = 2; m_generateStage.clear();
                emit promptChanged(); emit generateChanged(); emit inputsChanged(); emitOutputVisibility();
                push(QStringLiteral("Prompt ready ✓"));
                refreshMemoryStats();
            });
        } catch (const Err &e) {
            post([=, this, msg = e.msg] {
                m_generateError = msg; m_generateStatus = 3; m_generateStage.clear();
                emit generateChanged(); emit inputsChanged();
                push("Prompt generation failed: " + msg);
            });
        }
    });
}

void PromptController::usePrompt(const QString &p) {
    m_prompt = p; m_generateStatus = 2; m_memoryId.clear();
    emit promptChanged(); emit generateChanged(); emitOutputVisibility();
}

// ── Fire ─────────────────────────────────────────────────────────────────
// Guard against double-clicks now that fire stays enabled while batches run
bool PromptController::fireGate() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastFire < 600) return false;
    m_lastFire = now;
    return true;
}

void PromptController::fire() {
    if (!canFire() || !fireGate()) return;
    const QString prompt = m_prompt, ratio = m_ratio, res = m_resolution, memoryId = m_memoryId;
    const QStringList products = m_products;
    const int provider = m_provider, n = m_variations;

    m_tasks += 1;
    emit tasksChanged(); emitOutputVisibility();
    push(QStringLiteral("▶ %1 ×%2 · %3 · %4").arg(providers().at(provider).label).arg(n).arg(ratio, res.toUpper()));

    m_pool.start([=, this] {
        try {
            // Refs are uploaded ONCE and shared across the parallel batch
            QStringList refIds;
            if (!products.isEmpty() && n > 1) {
                const QStringList files = products.mid(0, 14);
                post([=, this] { push(QStringLiteral("Uploading %1 image%2...").arg(files.size()).arg(files.size() > 1 ? "s" : "")); });
                QList<QFuture<QString>> ups;
                for (const QString &f : files) ups << QtConcurrent::run(&m_pool, [f] { return Higgsfield::upload(f); });
                for (auto &u : ups) refIds << u.result();
                post([=, this] { push(QStringLiteral("%1 image%2 ready ✓").arg(files.size()).arg(files.size() > 1 ? "s" : "")); });
            }

            QList<QFuture<bool>> jobs;
            for (int i = 0; i < n; ++i)
                jobs << QtConcurrent::run(&m_pool, [=, this] { return runJob(prompt, products, ratio, res, provider, refIds); });
            int succeeded = 0;
            for (auto &j : jobs) if (j.result()) ++succeeded;

            const int failed = n - succeeded;
            post([=, this] {
                if (n > 1) {
                    push(failed == 0 ? QStringLiteral("All %1 variations generated.").arg(n)
                         : succeeded == 0 ? QStringLiteral("All %1 variations failed.").arg(n)
                         : QStringLiteral("%1/%2 generated, %3 failed.").arg(succeeded).arg(n).arg(failed));
                }
                m_fireResult = succeeded > 0 ? 2 : 3;
                if (succeeded > 0 && !memoryId.isEmpty()) { MemoryStore::markFired(memoryId, ratio); refreshMemoryStats(); }
                m_tasks -= 1;
                emit tasksChanged();
                if (succeeded > 0) emit toast(n > 1 ? QStringLiteral("%1 images saved").arg(succeeded) : QStringLiteral("Image saved"), 1);
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

bool PromptController::runJob(const QString &prompt, const QStringList &products, const QString &ratio, const QString &res,
                              int providerIndex, const QStringList &refIds) {
    const Provider &p = providers().at(providerIndex);
    const QString safeRatio = p.ratios.contains(ratio) ? ratio : p.ratios.first();
    const QString safeRes = p.resolutions.contains(res) ? res : p.resolutions.last();

    QStringList refs = refIds.mid(0, p.maxRefs);
    if (refs.isEmpty() && !products.isEmpty()) {
        if (products.size() > p.maxRefs)
            post([=, this] { push(QStringLiteral("%1 accepts max %2 reference images, using the first %2").arg(p.label).arg(p.maxRefs)); });
        refs = products.mid(0, p.maxRefs);
    }

    post([=, this] { push(QStringLiteral("Submitting %1 (%2 · %3)...").arg(p.jobType, safeRatio, safeRes.toUpper())); });
    try {
        Higgsfield::Params params = {{"prompt", prompt}, {"aspect_ratio", safeRatio}, {"resolution", safeRes},
                                     {"image_references", refs.isEmpty() ? QVariant() : QVariant(refs)}};
        if (p.thinking) params.append(qMakePair(QStringLiteral("thinking_level"), QVariant(QStringLiteral("high"))));
        const auto job = Higgsfield::generate(p.jobType, params, [this](const QString &line) { post([this, line] { push(line); }); });

        if (job.resultUrl.isEmpty()) { post([this] { push(QStringLiteral("No image in response")); }); return false; }

        // bmp_<ms>.<ext>: the prefix is what the Sorter pane matches. Parallel variations can finish in
        // the same millisecond, so bump until the name is free.
        const QString ext = Downloader::extensionOf(job.resultUrl, "jpg");
        qint64 stamp = QDateTime::currentMSecsSinceEpoch();
        QString name, dest;
        const QString dir = Prefs::outputPath();
        do { name = QStringLiteral("bmp_%1.%2").arg(stamp++).arg(ext); dest = dir + "/" + name; } while (QFileInfo::exists(dest));

        post([this] { push(QStringLiteral("Downloading image...")); });
        Downloader::download(job.resultUrl, dest);
        const auto props = Mac::imageProps(dest);
        const QString savedLine = props.ok ? QStringLiteral("Saved: %1 · %2×%3px").arg(name).arg(props.w).arg(props.h) : QStringLiteral("Saved: %1").arg(name);
        post([=, this] { push(savedLine); emit saved(dest); });
        return true;
    } catch (const Err &e) {
        post([this, msg = e.msg] { push("Error: " + msg); });
        return false;
    }
}

// ── Reset / log / history ────────────────────────────────────────────────
void PromptController::reset() {
    m_refs.clear(); m_products.clear(); m_brief.clear(); m_refSpecs.clear(); m_hasAutoBrief = false;
    m_prompt.clear(); m_generateStatus = 0; m_memoryId.clear(); m_variations = 1; m_generateError.clear();
    emit refsChanged(); emit productsChanged(); emit briefChanged(); emit refSpecsChanged(); emit promptChanged();
    emit generateChanged(); emit fireSettingsChanged(); emit inputsChanged();
    if (m_tasks == 0) { m_fireResult = 0; m_log.clear(); emit tasksChanged(); }
    emitOutputVisibility();
}

void PromptController::clearLog() {
    m_log.clear();
    m_fireResult = 0;
    emit tasksChanged();
    emitOutputVisibility();
}

QVariantList PromptController::history() const {
    QVariantList out;
    const QLocale es(QLocale::Spanish, QLocale::Colombia);
    for (const auto &e : MemoryStore::entries()) {
        out.append(QVariantMap{{"id", e.id}, {"description", e.description}, {"prompt", e.prompt}, {"fired", e.fired},
                               {"aspectRatio", e.aspectRatio},
                               {"date", es.toString(QDateTime::fromMSecsSinceEpoch(qint64(e.timestamp)), "d MMM · HH:mm")}});
    }
    return out;
}
