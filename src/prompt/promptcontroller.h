#pragma once
#include <QObject>
#include <QStringList>
#include <QThreadPool>
#include <QUrl>
#include <atomic>

class QTimer;
#include <QVariantList>

#include <QVariantMap>

#include "logmodel.h"

// The BMP pane's state and actions: inputs -> two-stage prompt (Sonnet brief, Opus prompt) -> fire
// to Higgsfield. Long work runs on a private thread pool; results hop back to the GUI thread.
class PromptController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList refs READ refs NOTIFY refsChanged)
    Q_PROPERTY(QStringList products READ products NOTIFY productsChanged)
    Q_PROPERTY(QString lookPath READ lookPath CONSTANT)                              // folder of reference photos to pick backgrounds from
    Q_PROPERTY(QStringList backgrounds READ backgrounds NOTIFY backgroundsChanged)   // optional location photo (at most one)
    Q_PROPERTY(QVariantList cameraSettings READ cameraSettings CONSTANT)             // [{id, title, options: [...]}] for the Camera card
    Q_PROPERTY(QVariantList cameraPresets READ cameraPresets CONSTANT)               // [{id, label}]
    Q_PROPERTY(QVariantMap camera READ camera NOTIFY cameraChanged)                  // chosen values by setting id (missing = auto)
    Q_PROPERTY(QString cameraSummary READ cameraSummary NOTIFY cameraChanged)
    Q_PROPERTY(QString cameraPreset READ cameraPreset NOTIFY cameraChanged)          // id of the preset the values match, else empty
    Q_PROPERTY(QString brief READ brief WRITE setBrief NOTIFY briefChanged)
    Q_PROPERTY(QString refSpecs READ refSpecs WRITE setRefSpecs NOTIFY refSpecsChanged)
    Q_PROPERTY(QString prompt READ prompt NOTIFY promptChanged)
    Q_PROPERTY(int generateStatus READ generateStatus NOTIFY generateChanged)     // 0 idle 1 loading 2 done 3 error
    Q_PROPERTY(QString generateError READ generateError NOTIFY generateChanged)
    Q_PROPERTY(QString generateStage READ generateStage NOTIFY generateChanged)
    Q_PROPERTY(bool briefLoading READ briefLoading NOTIFY briefLoadingChanged)
    Q_PROPERTY(QStringList providerNames READ providerNames CONSTANT)
    Q_PROPERTY(int providerIndex READ providerIndex WRITE setProviderIndex NOTIFY providerChanged)
    Q_PROPERTY(QString providerLabel READ providerLabel NOTIFY providerChanged)
    Q_PROPERTY(QStringList ratios READ ratios NOTIFY providerChanged)
    Q_PROPERTY(QStringList resolutions READ resolutions NOTIFY providerChanged)
    Q_PROPERTY(QStringList qualities READ qualities NOTIFY providerChanged)
    Q_PROPERTY(QString quality READ quality WRITE setQuality NOTIFY fireSettingsChanged)
    Q_PROPERTY(bool costKnown READ costKnown NOTIFY costChanged)
    Q_PROPERTY(double costPerImage READ costPerImage NOTIFY costChanged)
    Q_PROPERTY(double fireCost READ fireCost NOTIFY costChanged)          // per image x variations
    Q_PROPERTY(QString aspectRatio READ aspectRatio WRITE setAspectRatio NOTIFY fireSettingsChanged)
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY fireSettingsChanged)
    Q_PROPERTY(int variations READ variations WRITE setVariations NOTIFY fireSettingsChanged)
    Q_PROPERTY(int tasks READ tasks NOTIFY tasksChanged)
    Q_PROPERTY(int fireStatus READ fireStatus NOTIFY tasksChanged)                // 0 idle 1 loading 2 done 3 error
    Q_PROPERTY(LogModel *log READ log CONSTANT)
    Q_PROPERTY(int logCount READ logCount NOTIFY logCountChanged)
    Q_PROPERTY(int memoryTotal READ memoryTotal NOTIFY memoryChanged)
    Q_PROPERTY(int memoryFired READ memoryFired NOTIFY memoryChanged)
    Q_PROPERTY(bool hasCredits READ hasCredits NOTIFY creditsChanged)
    Q_PROPERTY(double credits READ credits NOTIFY creditsChanged)
    Q_PROPERTY(QString plan READ plan NOTIFY creditsChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(QStringList missingTools READ missingTools NOTIFY missingToolsChanged)
    Q_PROPERTY(bool showLogin READ showLogin NOTIFY showLoginChanged)
    Q_PROPERTY(bool canGenerate READ canGenerate NOTIFY inputsChanged)
    Q_PROPERTY(bool canAutoBrief READ canAutoBrief NOTIFY inputsChanged)
    Q_PROPERTY(bool canFire READ canFire NOTIFY promptChanged)
    Q_PROPERTY(bool hasOutput READ hasOutput NOTIFY outputVisibilityChanged)

public:
    explicit PromptController(QObject *parent = nullptr);

    QStringList refs() const { return m_refs; }
    QStringList products() const { return m_products; }
    QStringList backgrounds() const { return m_backgrounds; }
    QString lookPath() const;
    QVariantList cameraSettings() const;
    QVariantList cameraPresets() const;
    QVariantMap camera() const { return m_camera; }
    QString cameraSummary() const;
    QString cameraPreset() const;
    QString brief() const { return m_brief; }
    void setBrief(const QString &b);
    QString refSpecs() const { return m_refSpecs; }
    void setRefSpecs(const QString &s);
    QString prompt() const { return m_prompt; }
    int generateStatus() const { return m_generateStatus; }
    QString generateError() const { return m_generateError; }
    QString generateStage() const { return m_generateStage; }
    bool briefLoading() const { return m_briefLoading; }
    QStringList providerNames() const;
    int providerIndex() const { return m_provider; }
    void setProviderIndex(int i);
    QString providerLabel() const;
    QStringList ratios() const;
    QStringList resolutions() const;
    QStringList qualities() const;
    QString quality() const { return m_quality; }
    void setQuality(const QString &q);
    bool costKnown() const { return m_costKnown; }
    double costPerImage() const { return m_cost; }
    double fireCost() const { return m_cost * m_variations; }
    QString aspectRatio() const { return m_ratio; }
    void setAspectRatio(const QString &r);
    QString resolution() const { return m_resolution; }
    void setResolution(const QString &r);
    int variations() const { return m_variations; }
    void setVariations(int n);
    int tasks() const { return m_tasks; }
    int fireStatus() const { return m_tasks > 0 ? 1 : m_fireResult; }
    LogModel *log() { return &m_log; }
    int logCount() const { return m_log.count(); }
    int memoryTotal() const { return m_memTotal; }
    int memoryFired() const { return m_memFired; }
    bool hasCredits() const { return m_hasCredits; }
    double credits() const { return m_credits; }
    QString plan() const { return m_plan; }
    QString outputPath() const { return m_outputPath; }
    QStringList missingTools() const { return m_missing; }
    bool showLogin() const { return m_showLogin; }
    bool canGenerate() const { return !m_refs.isEmpty() && !m_products.isEmpty(); }
    bool canAutoBrief() const { return canGenerate() && m_generateStatus != 1 && !m_briefLoading; }
    bool canFire() const { return !m_prompt.isEmpty(); }
    bool hasOutput() const { return !m_prompt.isEmpty() || showLog(); }
    bool showLog() const { return m_log.count() > 0 || m_tasks > 0; }

    Q_INVOKABLE void boot();
    Q_INVOKABLE void addRefs(const QList<QUrl> &urls);
    Q_INVOKABLE void addProducts(const QList<QUrl> &urls);
    Q_INVOKABLE void removeRef(int index);
    Q_INVOKABLE void removeProduct(int index);
    Q_INVOKABLE void addBackground(const QList<QUrl> &urls);
    Q_INVOKABLE void removeBackground();
    Q_INVOKABLE void setCameraValue(const QString &id, const QString &value);   // picking the chosen value again clears it (auto)
    Q_INVOKABLE void applyCameraPreset(const QString &id);                      // picking the active preset clears everything
    Q_INVOKABLE void clearCamera();
    Q_INVOKABLE void autoWriteBrief();
    Q_INVOKABLE void generate();
    Q_INVOKABLE void fire();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE void demoLog();            // sample lines for design review (--open log)
    Q_INVOKABLE void usePrompt(const QString &p);
    Q_INVOKABLE QVariantList history() const;
    Q_INVOKABLE void setOutputPath(const QString &path);
    Q_INVOKABLE void refreshCredits();
    Q_INVOKABLE void login();
    Q_INVOKABLE QString toolPath(const QString &bin) const;

signals:
    void refsChanged();
    void productsChanged();
    void backgroundsChanged();
    void briefChanged();
    void cameraChanged();
    void refSpecsChanged();
    void promptChanged();
    void generateChanged();
    void briefLoadingChanged();
    void providerChanged();
    void fireSettingsChanged();
    void tasksChanged();
    void logCountChanged();
    void memoryChanged();
    void creditsChanged();
    void costChanged();
    void outputPathChanged();
    void missingToolsChanged();
    void showLoginChanged();
    void inputsChanged();
    void outputVisibilityChanged();
    void saved(const QString &path, const QString &model);       // a generated file landed in the output folder (and which engine made it)
    void toast(const QString &message, int kind);   // 0 info, 1 ok, 2 error

private:
    void push(const QString &line);
    void post(std::function<void()> fn);
    QString inputSignature() const;
    bool fireGate();
    bool runJob(const QString &prompt, const QStringList &products, int nBackground, const QString &ratio, const QString &res,
                const QString &quality, int provider, const QStringList &refIds);
    void scheduleCost();
    void estimateCost();
    void startCreditsWatch();
    void refreshMemoryStats();
    void emitOutputVisibility();

    QThreadPool m_pool;
    LogModel m_log;

    QStringList m_refs, m_products, m_backgrounds;
    QString m_brief, m_refSpecs, m_prompt;
    QVariantMap m_camera;
    int m_generateStatus = 0;
    QString m_generateError, m_generateStage;
    bool m_briefLoading = false;
    QString m_autoBriefText, m_autoBriefSig;
    bool m_hasAutoBrief = false;
    QString m_memoryId;

    int m_provider = 1;     // Nano Banana Pro
    QString m_ratio, m_resolution = "2k", m_quality = "high";
    int m_variations = 1;

    int m_tasks = 0;
    int m_fireResult = 0;
    qint64 m_lastFire = 0;

    int m_memTotal = 0, m_memFired = 0;
    QTimer *m_costTimer = nullptr, *m_creditsTimer = nullptr;
    int m_costTicket = 0;
    bool m_costKnown = false;
    double m_cost = 0;
    std::atomic<bool> m_creditsBusy{false};
    bool m_hasCredits = false;
    double m_credits = 0;
    QString m_plan;
    QString m_outputPath;
    QStringList m_missing;
    bool m_showLogin = false;
};
