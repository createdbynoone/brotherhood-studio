#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QList>

// Activity log shown in the BMP pane. Capped; appends in place (never copies the list per line).
class LogModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Kind { Plain, Ok, Error, Warn, Action };
    enum Roles { TimeRole = Qt::UserRole + 1, LineRole, KindRole };

    explicit LogModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override {
        return {{TimeRole, "time"}, {LineRole, "line"}, {KindRole, "kind"}};
    }

    void append(const QString &line);
    void clear();
    int count() const { return int(m_rows.size()); }

    static Kind classify(const QString &line);

private:
    struct Row { QDateTime when; QString line; Kind kind; };
    QList<Row> m_rows;
    static constexpr int kCap = 400;
};
