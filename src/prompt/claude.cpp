#include "claude.h"

#include "../core/err.h"
#include "../ecom/ecomprompts.h"
#include "../ecom/ecomstyle.h"
#include "memorystore.h"
#include "prompts.h"
#include "shell.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QSet>
#include <QtMath>

namespace ClaudeCli {

static const QString kBriefModel = QStringLiteral("claude-sonnet-5-5");
static const QString kPromptModel = QStringLiteral("claude-opus-5-5");
static const qint64 kCooldownMs = 4000;

bool isInstalled() { return !Shell::resolve("claude").isEmpty(); }

static QString block(const QString &label, const QStringList &paths) {
    if (paths.isEmpty()) return {};
    QStringList lines;
    for (int i = 0; i < paths.size(); ++i) lines << QStringLiteral("Image %1: %2").arg(i + 1).arg(paths[i]);
    return QStringLiteral("## %1:\n%2\n\n").arg(label, lines.join('\n'));
}

static QStringList uniqueOrdered(const QStringList &in) {
    QStringList out;
    QSet<QString> seen;
    for (const auto &s : in) if (!seen.contains(s)) { seen.insert(s); out << s; }
    return out;
}

static QString nfc(const QString &s) { return s.normalized(QString::NormalizationForm_C); }

// `extraDirs`: folders the model may Read from without being required to (the ecommerce style library)
static QString call(const QString &model, const QString &system, const QString &user, const QStringList &images, const QStringList &extraDirs = {}) {
    const QStringList unique = uniqueOrdered(images);
    QStringList missing;
    for (const auto &p : unique) if (!QFileInfo::exists(p)) missing << QFileInfo(p).fileName();
    if (!missing.isEmpty())
        throw Err(QStringLiteral("No se pudo acceder a estas imágenes (¿se movieron, se renombraron, o no están descargadas de iCloud?): %1").arg(missing.join(", ")));

    QStringList dirs;
    for (const auto &p : unique) dirs << QFileInfo(p).absolutePath();
    dirs += extraDirs;
    dirs = uniqueOrdered(dirs);

    QStringList args = {"-p", user, "--output-format", "stream-json", "--verbose", "--model", model,
                        "--system-prompt", system, "--tools", "Read", "--permission-mode", "bypassPermissions",
                        "--safe-mode", "--no-session-persistence"};
    for (const auto &d : dirs) args << "--add-dir" << d;

    const auto result = Shell::run("claude", args);

    // macOS can echo paths back in NFD even when we sent NFC: compare normalized.
    QSet<QString> readPaths;
    QJsonObject final;
    bool haveFinal = false;
    for (const QString &line : result.out.split('\n', Qt::SkipEmptyParts)) {
        const auto doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) continue;
        const auto obj = doc.object();
        const QString type = obj.value("type").toString();
        if (type == "assistant") {
            for (const auto &b : obj.value("message").toObject().value("content").toArray()) {
                const auto bo = b.toObject();
                if (bo.value("type").toString() == "tool_use" && bo.value("name").toString() == "Read") {
                    const QString p = bo.value("input").toObject().value("file_path").toString();
                    if (!p.isEmpty()) readPaths.insert(nfc(p));
                }
            }
        } else if (type == "result") {
            final = obj; haveFinal = true;
        }
    }
    if (!haveFinal) throw Err(QStringLiteral("Claude CLI no devolvió resultado"));
    if (final.value("is_error").toBool()) throw Err(final.value("result").toString("Claude CLI error"));

    QStringList unread;
    for (const auto &p : unique) if (!readPaths.contains(nfc(p))) unread << QFileInfo(p).fileName();
    if (!unread.isEmpty())
        throw Err(QStringLiteral("Claude no llegó a ver %1 imagen(es) antes de generar el prompt: %2. Vuelve a intentar.").arg(unread.size()).arg(unread.join(", ")));

    return final.value("result").toString().trimmed();
}

QString generateBrief(const QStringList &refs, const QStringList &products, const QStringList &backgrounds, const QString &specs) {
    const int unique = uniqueOrdered(refs + products + backgrounds).size();
    const QString user = block("REFERENCE IMAGE (scene, framing, light, mood)", refs)
        + block("PRODUCT PHOTOS (Brotherhood garment)", products)
        + block("BACKGROUND PLATE (the real location where the image takes place)", backgrounds)
        + QStringLiteral("## REFERENCE SPECS (what the user wants taken from the reference):\n%1\n\n")
              .arg(specs.isEmpty() ? QStringLiteral("(none given: infer the strongest, most repeatable traits of the reference)") : specs)
        + QStringLiteral("You MUST call the Read tool once for each of the %1 image path(s) listed above before writing. Then write the brief.").arg(unique);
    return call(kBriefModel, Prompts::briefSystem, user, refs + products + backgrounds);
}

PromptResult generatePrompt(const QStringList &refs, const QStringList &products, const QStringList &backgrounds, const QString &description, const QString &specs, const QString &camera) {
    static QMutex mtx;
    static qint64 lastGenerate = 0;
    {
        QMutexLocker l(&mtx);
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastGenerate < kCooldownMs)
            throw Err(QStringLiteral("Rate limit: wait %1s before generating again").arg(qCeil((kCooldownMs - (now - lastGenerate)) / 1000.0)));
        lastGenerate = now;
    }

    const QString system = Prompts::system + Prompts::engineNote + MemoryStore::context();
    const int unique = uniqueOrdered(refs + products + backgrounds).size();
    const QString user = block("REFERENCE IMAGES (composition/mood)", refs)
        + block("PRODUCT PHOTOS (Brotherhood garment)", products)
        + block("BACKGROUND PLATE (real location; it is sent as the LAST reference image)", backgrounds)
        + Prompts::referenceProtocol
        + (backgrounds.isEmpty() ? Prompts::realismProtocol : Prompts::backgroundProtocol)
        + (specs.isEmpty() ? QString() : QStringLiteral("## REFERENCE SPECS (priority: follow these over your own reading of the reference):\n%1\n\n").arg(specs))
        + camera
        + QStringLiteral("## USER BRIEF:\n%1\n\n").arg(description)
        + QStringLiteral("You MUST call the Read tool once for each of the %1 image path(s) listed above before writing anything — do not skip any, do not infer content from filenames alone. Only after viewing every image, generate the marketing image prompt.").arg(unique);

    const QString prompt = call(kPromptModel, system, user, refs + products + backgrounds);
    const auto entry = MemoryStore::add(description, prompt);
    return {prompt, entry.id};
}


// The model sometimes wraps JSON in a fence or adds a sentence: take the outermost object, validated but left untouched.
static QString extractJsonObject(const QString &text) {
    const int a = text.indexOf('{'), b = text.lastIndexOf('}');
    if (a < 0 || b <= a) return {};
    const QString object = text.mid(a, b - a + 1);
    return QJsonDocument::fromJson(object.toUtf8()).isObject() ? object : QString();      // as written: QJson would sort the keys
}

QString generateEcomSpec(const QStringList &photos, const QString &layoutRef, const QString &notes, int width, int height, bool wantBack) {
    QStringList images = photos;
    if (!layoutRef.isEmpty()) images << layoutRef;
    const QString facts = width > 0 && height > 0
        ? QStringLiteral("MAIN GARMENT PHOTO FACTS (Image 1): %1 x %2 px, width/height = %3.\n\n").arg(width).arg(height).arg(width / double(height), 0, 'f', 3)
        : QString();
    const QString library = EcomStyle::index();
    const QString user = block(QStringLiteral("GARMENT PHOTOS (the product, source of truth: %1 photo(s), angles and close-ups of the same product; write product.photo_views with one entry per photo, in this order)").arg(photos.size()), photos)
        + block("LAYOUT REFERENCE (the user's choice for this run: canvas, subject size and position, camera, light and background to reproduce)", layoutRef.isEmpty() ? QStringList() : QStringList{layoutRef})
        + (library.isEmpty() ? QStringLiteral("## HOUSE STYLE LIBRARY: empty. Use the house-style rules of the system prompt.\n\n")
                             : QStringLiteral("## HOUSE STYLE LIBRARY (finished Brotherhood catalog photos, one per line; Read only the 1 or 2 that match; folder: %1):\n%2\n\n").arg(EcomStyle::dir(), library))
        + facts
        + QStringLiteral("## BACK SHOT: %1\n\n").arg(wantBack ? QStringLiteral("YES. Add the back_spec key as the last key of the object.") : QStringLiteral("NO. Leave back_spec out."))
        + (notes.trimmed().isEmpty() ? QString() : QStringLiteral("## USER NOTES (these outrank your own reading):\n%1\n\n").arg(notes.trimmed()))
        + QStringLiteral("You MUST call the Read tool once for each of the %1 image path(s) listed above before writing. Then return the JSON object.").arg(images.size());

    const QStringList extra{EcomStyle::dir()};
    QString json = extractJsonObject(call(kPromptModel, EcomPrompts::system, user, images, extra));
    if (json.isEmpty()) {         // one retry: a malformed answer is usually a one-off
        json = extractJsonObject(call(kPromptModel, EcomPrompts::system, user + QStringLiteral("\n\nYour previous answer was not a single valid JSON object. Return only the JSON object."), images, extra));
        if (json.isEmpty()) throw Err(QStringLiteral("Claude did not return valid JSON. Try again."));
    }
    return json;
}

}  // namespace ClaudeCli
