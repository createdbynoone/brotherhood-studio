#pragma once
#include <QObject>
#include <QStringList>
#include <QThreadPool>
#include <QUrl>
#include <atomic>

#include "../prompt/logmodel.h"

class QTimer;

// The Ecommerce pane: a garment photo (plus an optional layout reference) -> Opus writes a JSON specification that
// locks size, position, light and background and sets the retouch targets (smooth fabric, clean seams, exact
// graphics) -> the JSON is fired at Higgsfield together with the photo, as an edit. Long work runs on a private
// thread pool; results hop back to the GUI thread.
inline constexpr int kMaxPhotos = 8;     // leaves room for the layout reference under the engine's 10-reference limit

class EcomController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList photos READ photos NOTIFY photosChanged)             // the garment photos: angles and close-ups (up to kMaxPhotos)
    Q_PROPERTY(QStringList layoutRefs READ layoutRefs NOTIFY layoutRefsChanged) // optional catalog photo whose framing/light to copy
    Q_PROPERTY(bool shootBack READ shootBack WRITE setShootBack NOTIFY backChanged)   // also fire the back view (garments, not accessories)
    Q_PROPERTY(bool backAllowed READ backAllowed NOTIFY backChanged)            // false for caps, hats, bags, socks
    Q_PROPERTY(bool backStale READ backStale NOTIFY backChanged)                // back requested but the JSON has no back_spec: Analyze again
    Q_PROPERTY(QString backSource READ backSource NOTIFY backChanged)           // "photo" or "inferred" (from the back_spec)
    Q_PROPERTY(int shots READ shots NOTIFY backChanged)                         // images per variation: 1, or 2 with the back
    Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY notesChanged)
    Q_PROPERTY(QString photoInfo READ photoInfo NOTIFY photosChanged)
    Q_PROPERTY(QString json READ json WRITE setJson NOTIFY jsonChanged)
    Q_PROPERTY(bool jsonValid READ jsonValid NOTIFY jsonChanged)
    Q_PROPERTY(QString productType READ productType NOTIFY jsonChanged)         // detected from the photo
    Q_PROPERTY(int analyzeStatus READ analyzeStatus NOTIFY analyzeChanged)      // 0 idle 1 loading 2 done 3 error
    Q_PROPERTY(QString analyzeError READ analyzeError NOTIFY analyzeChanged)
    Q_PROPERTY(bool canAnalyze READ canAnalyze NOTIFY inputsChanged)
    Q_PROPERTY(bool canFire READ canFire NOTIFY jsonChanged)
    Q_PROPERTY(QStringList providerNames READ providerNames CONSTANT)
    Q_PROPERTY(int providerIndex READ providerIndex WRITE setProviderIndex NOTIFY providerChanged)
    Q_PROPERTY(QString providerLabel READ providerLabel NOTIFY providerChanged)
    Q_PROPERTY(QStringList ratios READ ratios NOTIFY providerChanged)           // "auto" first, then what the engine accepts
    Q_PROPERTY(QStringList resolutions READ resolutions NOTIFY providerChanged)
    Q_PROPERTY(QStringList qualities READ qualities NOTIFY providerChanged)
    Q_PROPERTY(QString aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY fireSettingsChanged)
    Q_PROPERTY(QString effectiveRatio READ effectiveRatio NOTIFY fireSettingsChanged)   // what "auto" resolves to
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY fireSettingsChanged)
    Q_PROPERTY(QString quality READ quality WRITE setQuality NOTIFY fireSettingsChanged)
    Q_PROPERTY(int variations READ variations WRITE setVariations NOTIFY fireSettingsChanged)
    Q_PROPERTY(bool costKnown READ costKnown NOTIFY costChanged)
    Q_PROPERTY(double fireCost READ fireCost NOTIFY costChanged)
    Q_PROPERTY(int tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(int fireStatus READ fireStatus NOTIFY tasksChanged)              // 0 idle 1 loading 2 done 3 error
    Q_PROPERTY(LogModel *log READ log CONSTANT)
    Q_PROPERTY(int logCount READ logCount NOTIFY logCountChanged)
    Q_PROPERTY(QStringList results READ results NOTIFY resultsChanged)          // images saved in this session, newest first
    Q_PROPERTY(QString resultsPath READ resultsPath NOTIFY resultsChanged)
    Q_PROPERTY(QString stylePath READ stylePath CONSTANT)                       // the house-style library folder
    Q_PROPERTY(int styleCount READ styleCount NOTIFY styleChanged)              // photos in it

public:
    explicit EcomController(QObject *parent = nullptr);

    QStringList photos() const { return m_photos; }
    QStringList layoutRefs() const { return m_layoutRefs; }
    bool shootBack() const { return m_shootBack; }
    void setShootBack(bool on);
    bool backAllowed() const;
    bool backStale() const;
    QString backSource() const { return m_backSource; }
    int shots() const { return (m_shootBack && backAllowed()) ? 2 : 1; }
    QString notes() const { return m_notes; }
    void setNotes(const QString &n);
    QString photoInfo() const;
    QString json() const { return m_json; }
    void setJson(const QString &j);
    bool jsonValid() const;
    QString productType() const { return m_productType; }
    int analyzeStatus() const { return m_analyzeStatus; }
    QString analyzeError() const { return m_analyzeError; }
    bool canAnalyze() const { return !m_photos.isEmpty() && m_analyzeStatus != 1; }
    bool canFire() const { return !m_json.trimmed().isEmpty() && !m_photos.isEmpty(); }
    QStringList providerNames() const;
    int providerIndex() const { return m_provider; }
    void setProviderIndex(int i);
    QString providerLabel() const;
    QStringList ratios() const;
    QStringList resolutions() const;
    QStringList qualities() const;
    QString aspectRatio() const { return m_ratio; }
    void setAspectRatio(const QString &r);
    QString effectiveRatio() const;
    QString resolution() const { return m_resolution; }
    void setResolution(const QString &r);
    QString quality() const { return m_quality; }
    void setQuality(const QString &q);
    int variations() const { return m_variations; }
    void setVariations(int n);
    bool costKnown() const { return m_costKnown; }
    double fireCost() const { return m_cost * m_variations * shots(); }
    int tasks() const { return m_tasks; }
    int fireStatus() const { return m_tasks > 0 ? 1 : m_fireResult; }
    LogModel *log() { return &m_log; }
    int logCount() const { return m_log.count(); }
    QStringList results() const { return m_results; }
    QString resultsPath() const;
    QString stylePath() const;
    int styleCount() const { return m_styleCount; }

    Q_INVOKABLE void addPhoto(const QList<QUrl> &urls);
    Q_INVOKABLE void removePhoto(int index);
    Q_INVOKABLE void addLayoutRef(const QList<QUrl> &urls);
    Q_INVOKABLE void removeLayoutRef();
    Q_INVOKABLE void analyze();
    Q_INVOKABLE void fire();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE void demo();               // sample JSON and log lines for design review (--open ecom)
    Q_INVOKABLE void scheduleCost();
    Q_INVOKABLE void refreshStyle();       // re-count the library (after the user adds photos)

signals:
    void photosChanged();
    void layoutRefsChanged();
    void notesChanged();
    void jsonChanged();
    void analyzeChanged();
    void inputsChanged();
    void providerChanged();
    void fireSettingsChanged();
    void costChanged();
    void tasksChanged();
    void logCountChanged();
    void resultsChanged();
    void styleChanged();
    void backChanged();
    void charged();                                   // a job was created (credits moved): refresh the balance
    void saved(const QString &path, const QString &model);   // a generated file landed in the Ecommerce folder (and which engine made it)
    void toast(const QString &message, int kind);     // 0 info, 1 ok, 2 error

private:
    void push(const QString &line);
    void post(std::function<void()> fn);
    void stageInto(const QList<QUrl> &urls, bool layout);
    void estimateCost();
    bool runJob(const QString &prompt, const QStringList &refs, const QString &view, const QString &ratio, const QString &res, const QString &quality, int provider);

    QThreadPool m_pool;
    LogModel m_log;

    QStringList m_photos, m_layoutRefs;
    int m_photoW = 0, m_photoH = 0;
    QString m_notes, m_json, m_productType, m_backSource;
    bool m_shootBack = false;
    int m_analyzeStatus = 0;
    QString m_analyzeError;

    int m_provider = 0;     // GPT Image 2.5 Sunburst (set in the constructor); Ecommerce uses only this engine
    QString m_ratio = "1:1", m_resolution = "2k", m_quality = "high";
    int m_variations = 1;

    int m_tasks = 0, m_fireResult = 0;
    qint64 m_lastFire = 0;
    QTimer *m_costTimer = nullptr;
    int m_costTicket = 0;
    bool m_costKnown = false;
    double m_cost = 0;
    QStringList m_results;
    int m_styleCount = 0;
};
