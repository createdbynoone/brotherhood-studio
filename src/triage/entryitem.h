#pragma once
#include <QJsonObject>
#include <QObject>
#include <QStringList>

// One library entry, exposed to QML as a live object: delegates bind to its properties and only the
// bindings that depend on a changed property re-evaluate (no row rebuilds on selection / status).
class EntryItem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString filename READ filename CONSTANT)
    Q_PROPERTY(QString fingerprint READ fingerprint NOTIFY fingerprintChanged)
    Q_PROPERTY(int status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString note READ note NOTIFY noteChanged)
    Q_PROPERTY(bool isVideo READ isVideo CONSTANT)
    Q_PROPERTY(bool missing READ missing NOTIFY missingChanged)
    Q_PROPERTY(bool isNew READ isNew NOTIFY isNewChanged)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged)
    Q_PROPERTY(bool primary READ primary NOTIFY primaryChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(QStringList chips READ chips NOTIFY chipsChanged)
    Q_PROPERTY(int chipExtra READ chipExtra NOTIFY chipsChanged)
    Q_PROPERTY(QString addedLabel READ addedLabel CONSTANT)
    Q_PROPERTY(QString thumbSource READ thumbSource NOTIFY fingerprintChanged)

public:
    explicit EntryItem(const QJsonObject &raw, QObject *parent = nullptr);

    QJsonObject toJson() const;

    QString path() const { return m_path; }
    QString filename() const;
    QString fingerprint() const { return m_fingerprint; }
    int status() const { return m_status; }
    QString note() const { return m_note; }
    bool isVideo() const { return m_isVideo; }
    bool missing() const { return m_missing; }
    bool isNew() const { return m_isNew; }
    bool selected() const { return m_selected; }
    bool primary() const { return m_primary; }
    QStringList categories() const { return m_categories; }
    QStringList chips() const { return m_chips; }
    int chipExtra() const { return m_chipExtra; }
    QString addedLabel() const;
    QString thumbSource() const;
    int source() const { return m_source; }
    double addedAt() const { return m_addedAt; }
    double updatedAt() const { return m_updatedAt; }

    void setFingerprint(const QString &f);
    void setStatus(int s);
    void setNote(const QString &n);
    void setMissing(bool m);
    void setIsNew(bool n);
    void setSelected(bool s);
    void setPrimary(bool p);
    void setCategories(const QStringList &c);
    void setChips(const QStringList &names, int extra);
    void touchUpdated();
    void setPath(const QString &p);   // re-link after a move (path is the identity, so only the store calls this)

    static EntryItem *makeNew(const QString &path, const QString &fingerprint, int source);

signals:
    void fingerprintChanged();
    void statusChanged();
    void noteChanged();
    void missingChanged();
    void isNewChanged();
    void selectedChanged();
    void primaryChanged();
    void categoriesChanged();
    void chipsChanged();

private:
    QJsonObject m_raw;       // unknown keys from other builds survive a round trip
    QString m_path, m_fingerprint, m_note;
    int m_status = 0, m_source = 0, m_rating = 0;
    double m_addedAt = 0, m_updatedAt = 0;
    bool m_isVideo = false, m_missing = false, m_hasMissingKey = false;
    bool m_isNew = false, m_selected = false, m_primary = false;
    QStringList m_categories, m_chips;
    int m_chipExtra = 0;
};
