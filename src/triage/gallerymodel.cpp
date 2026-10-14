#include "gallerymodel.h"

#include "entryitem.h"

#include <QSet>
#include <algorithm>
#include <cmath>

GalleryModel::GalleryModel(QObject *parent) : QAbstractListModel(parent) {
    m_reflowTimer.setSingleShot(true);
    m_reflowTimer.setInterval(700);
    connect(&m_reflowTimer, &QTimer::timeout, this, [this] { m_reflowing = false; emit reflowingChanged(); });
}

QHash<int, QByteArray> GalleryModel::roleNames() const {
    return {{KindRole, "kind"}, {EntryRole, "entry"}, {XRole, "nx"}, {YRole, "ny"}, {WRole, "nw"}, {HRole, "nh"},
            {TitleRole, "title"}, {CountRole, "count"}, {SubCountRole, "subCount"}, {GroupIdRole, "groupId"},
            {CollapsedRole, "collapsed"}, {StaggerRole, "stagger"}};
}

QVariant GalleryModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_visible.size()) return {};
    const int ni = m_index.value(m_visible.at(index.row()), -1);
    if (ni < 0) return {};
    const Node &n = m_nodes.at(ni);
    switch (role) {
    case KindRole: return int(n.kind);
    case EntryRole: return QVariant::fromValue(static_cast<QObject *>(n.entry));
    case XRole: return n.x;
    case YRole: return n.y;
    case WRole: return n.w;
    case HRole: return n.h;
    case TitleRole: return n.title;
    case CountRole: return n.count;
    case SubCountRole: return n.subCount;
    case GroupIdRole: return n.groupId;
    case CollapsedRole: return n.collapsed;
    case StaggerRole: return n.stagger;
    }
    return {};
}

void GalleryModel::markReflow() {
    if (!m_reflowing) { m_reflowing = true; emit reflowingChanged(); }
    m_reflowTimer.start();
}

// ── Layout ───────────────────────────────────────────────────────────────
void GalleryModel::relayout() {
    qreal y = kTop;
    int col = 0;
    for (Node &n : m_nodes) {
        if (n.kind == Card) {
            if (col == m_cols) { y += m_cardH + kGap; col = 0; }
            n.x = kPad + col * (m_cardW + kGap);
            n.y = y;
            n.w = m_cardW;
            n.h = m_cardH;
            n.stagger = col;
            ++col;
        } else {
            if (col > 0) { y += m_cardH + kGap; col = 0; }
            n.x = 0;
            n.y = y;
            n.w = m_width;
            n.h = n.kind == GroupHeader ? kGroupH : kSubH;
            n.stagger = 0;
            y += n.h;
        }
    }
    if (col > 0) y += m_cardH + kGap;
    const qreal total = y + kBottom;
    if (!qFuzzyCompare(total, m_total)) { m_total = total; emit totalHeightChanged(); }
}

int GalleryModel::firstAt(qreal y) const {
    // first node whose bottom edge is below y (nodes are ordered by y)
    int lo = 0, hi = int(m_nodes.size());
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (m_nodes[mid].y + m_nodes[mid].h < y) lo = mid + 1; else hi = mid;
    }
    return lo;
}

void GalleryModel::setViewport(qreal width, qreal gridSize) {
    if (width <= 0) return;
    const bool sameInputs = qFuzzyCompare(width, m_width) && qFuzzyCompare(gridSize, m_gridSize);
    if (sameInputs) return;

    // scroll anchoring: remember which node is at the top so the content under the viewport stays put
    const Node *anchor = m_anchorKey.isEmpty() || !m_index.contains(m_anchorKey) ? nullptr : &m_nodes[m_index[m_anchorKey]];
    const qreal anchorOffset = anchor ? anchor->y - m_scroll : 0;

    const int oldCols = m_cols;
    const bool sizeChanged = !qFuzzyCompare(gridSize, m_gridSize);
    m_width = width;
    m_gridSize = gridSize;
    m_cols = qMax(1, int(std::floor((width - kPad * 2 + kGap) / (gridSize + kGap))));
    m_cardW = (width - kPad * 2 - (m_cols - 1) * kGap) / m_cols;
    m_cardH = m_cardW * 1.25;

    const bool colsChanged = m_cols != oldCols;
    // A column change or a grid-size change animates; a plain width change (the pane or window is being
    // resized continuously) follows instantly so cards never lag behind the edge.
    if (colsChanged || sizeChanged) markReflow();
    relayout();
    if (colsChanged) emit columnsChanged();
    emit layoutChanged();
    refreshWindow(true);

    if (anchor && m_scroll > 1 && colsChanged && m_index.contains(m_anchorKey)) {
        const qreal ny = m_nodes[m_index[m_anchorKey]].y - anchorOffset;
        emit scrollRequest(qMax(0.0, ny));
    }
}

void GalleryModel::setScroll(qreal contentY, qreal viewHeight) {
    m_scroll = contentY;
    m_viewH = viewHeight;
    const int a = firstAt(contentY);
    if (a < m_nodes.size()) { m_anchorKey = m_nodes[a].key; m_anchorOffset = m_nodes[a].y - contentY; }
    refreshWindow(false);
}

void GalleryModel::setNodes(const QList<Node> &nodes) {
    const bool hadNodes = !m_nodes.isEmpty();
    m_nodes = nodes;
    m_index.clear();
    for (int i = 0; i < m_nodes.size(); ++i) m_index.insert(m_nodes[i].key, i);
    if (hadNodes) markReflow();
    relayout();
    emit layoutChanged();
    refreshWindow(true);
}

// ── Window: which nodes exist as rows right now ─────────────────────────
void GalleryModel::refreshWindow(bool dataDirty) {
    QStringList target;
    if (!m_nodes.isEmpty() && m_viewH > 0) {
        for (int i = firstAt(m_scroll - kBuffer); i < m_nodes.size() && m_nodes[i].y <= m_scroll + m_viewH + kBuffer; ++i)
            target << m_nodes[i].key;
    }
    const QSet<QString> want(target.begin(), target.end());

    // 1) drop rows that left the window (or no longer exist), back to front in contiguous runs
    for (int i = int(m_visible.size()) - 1; i >= 0; --i) {
        if (want.contains(m_visible[i])) continue;
        int first = i;
        while (first > 0 && !want.contains(m_visible[first - 1])) --first;
        beginRemoveRows({}, first, i);
        m_visible.remove(first, i - first + 1);
        endRemoveRows();
        i = first;
    }
    // 2) move survivors / insert newcomers so the rows follow the target order
    for (int i = 0; i < target.size(); ++i) {
        if (i < m_visible.size() && m_visible[i] == target[i]) continue;
        const int j = int(m_visible.indexOf(target[i], i));
        if (j > i) {
            beginMoveRows({}, j, j, {}, i);
            m_visible.move(j, i);
            endMoveRows();
        } else {
            beginInsertRows({}, i, i);
            m_visible.insert(i, target[i]);
            endInsertRows();
        }
    }
    // 3) survivors take their new geometry / content
    if (dataDirty && !m_visible.isEmpty()) emit dataChanged(index(0), index(int(m_visible.size()) - 1));
}

qreal GalleryModel::yOf(const QString &path) const {
    const auto it = m_index.constFind("c:" + path);
    return it == m_index.constEnd() ? -1 : m_nodes[it.value()].y;
}
