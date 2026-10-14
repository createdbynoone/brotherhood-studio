#include "logmodel.h"

QVariant LogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case TimeRole: return r.when.toString("HH:mm:ss");
    case LineRole: return r.line;
    case KindRole: return int(r.kind);
    }
    return {};
}

LogModel::Kind LogModel::classify(const QString &line) {
    if (line.contains(QChar(0x2713)) || line.startsWith("Saved") || (line.startsWith("All ") && line.contains("generated"))) return Ok;
    if (line.toLower().startsWith("error") || line.contains("failed") || line.startsWith("Rate limit") || line.contains("no llegó") || line.contains("No se pudo")) return Error;
    if (line.startsWith("Warning") || line.contains("accepts max") || (line.contains('/') && line.contains("generated,"))) return Warn;
    if (line.startsWith(QChar(0x25B6))) return Action;
    return Plain;
}

void LogModel::append(const QString &line) {
    beginInsertRows({}, int(m_rows.size()), int(m_rows.size()));
    m_rows.append({QDateTime::currentDateTime(), line, classify(line)});
    endInsertRows();
    if (m_rows.size() > kCap) {
        const int drop = int(m_rows.size()) - kCap;
        beginRemoveRows({}, 0, drop - 1);
        m_rows.remove(0, drop);
        endRemoveRows();
    }
}

void LogModel::clear() {
    if (m_rows.isEmpty()) return;
    beginResetModel();
    m_rows.clear();
    endResetModel();
}
