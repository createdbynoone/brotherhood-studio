#pragma once
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

// Window-level state: layout, active pane, access key, update notice and small OS helpers.
// The two panes keep their own controllers so a log line in BMP never touches the Sorter grid.
class StudioController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString layout READ layout WRITE setLayout NOTIFY layoutChanged)       // "prompt" | "ecom" | "split" | "triage"
    Q_PROPERTY(double splitFraction READ splitFraction WRITE setSplitFraction NOTIFY splitFractionChanged)
    Q_PROPERTY(QString activePane READ activePane WRITE setActivePane NOTIFY activePaneChanged)
    Q_PROPERTY(bool unlocked READ unlocked NOTIFY unlockedChanged)
    Q_PROPERTY(double lockUntil READ lockUntil NOTIFY lockChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(bool reduceMotion READ reduceMotion CONSTANT)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY updateChanged)
    Q_PROPERTY(QUrl updateUrl READ updateUrl NOTIFY updateChanged)
    Q_PROPERTY(bool checkingUpdate READ checkingUpdate NOTIFY updateChanged)
    Q_PROPERTY(bool updating READ updating NOTIFY updatingChanged)
    Q_PROPERTY(double updateProgress READ updateProgress NOTIFY updatingChanged)       // 0..1
    Q_PROPERTY(int uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)        // chosen, percent; applies on relaunch
    Q_PROPERTY(int activeUiScale READ activeUiScale CONSTANT)                         // the scale this run started with
    Q_PROPERTY(QString updateError READ updateError NOTIFY updatingChanged)

public:
    explicit StudioController(QObject *parent = nullptr);

    QString layout() const { return m_layout; }
    void setLayout(const QString &l);
    double splitFraction() const { return m_split; }
    void setSplitFraction(double f);
    QString activePane() const { return m_active; }
    void setActivePane(const QString &p);
    bool unlocked() const { return m_unlocked; }
    double lockUntil() const;
    QString version() const;
    bool reduceMotion() const { return m_reduceMotion; }
    bool updateAvailable() const { return !m_updateVersion.isEmpty(); }
    QString updateVersion() const { return m_updateVersion; }
    QUrl updateUrl() const { return m_updateUrl; }
    bool checkingUpdate() const { return m_checking; }
    bool updating() const { return m_updating; }
    double updateProgress() const { return m_progress; }
    int uiScale() const { return m_uiScale; }
    void setUiScale(int percent);
    int activeUiScale() const { return m_activeUiScale; }
    QString updateError() const { return m_updateError; }

    Q_INVOKABLE void verifyKey(const QString &key);      // async; emits keyResult
    Q_INVOKABLE void forgetKey();
    Q_INVOKABLE void saveSplit();
    Q_INVOKABLE void cyclePane();
    Q_INVOKABLE void dismissUpdate();
    Q_INVOKABLE void checkForUpdate();
    Q_INVOKABLE void restart();             // quit and open the app again (applies the interface scale)
    Q_INVOKABLE void installUpdate();       // download the release zip, replace this app, relaunch
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void openUrl(const QUrl &url);
    Q_INVOKABLE void openFolder(const QString &path);
    Q_INVOKABLE void zoomWindow();
    Q_INVOKABLE QString abbreviate(const QString &path) const;       // /Users/me/Desktop -> ~/Desktop
    Q_INVOKABLE QString toolPath(const QString &bin) const;          // resolved CLI path or empty
    Q_INVOKABLE QString dbPath() const;
    Q_INVOKABLE QString memoryPath() const;
    Q_INVOKABLE QString libraryPath() const;
    Q_INVOKABLE void revealFile(const QString &path);
    Q_INVOKABLE void clearThumbnails();

signals:
    void layoutChanged();
    void magnified(double x, double y, double magnification, int phase);   // trackpad pinch, window coordinates
    void splitFractionChanged();
    void activePaneChanged();
    void unlockedChanged();
    void lockChanged();
    void updateChanged();
    void updatingChanged();
    void uiScaleChanged();
    void keyResult(bool ok, double lockUntil);
    void toast(const QString &message, int kind);

private:
    QString m_layout, m_active = "prompt";
    double m_split;
    bool m_unlocked = false;
    bool m_reduceMotion = false;
    int m_uiScale = 100, m_activeUiScale = 100;
    bool m_checking = false;
    QString m_updateVersion;
    QUrl m_updateUrl;
    QUrl m_updateAsset;
    bool m_updating = false;
    double m_progress = 0;
    QString m_updateError;
    QNetworkAccessManager *m_nam = nullptr;
};
