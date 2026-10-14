#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QTimer>

class EntryItem;

// The Sorter grid as an absolutely-positioned gallery. Every node (card, category header, product header)
// has a stable key and a computed x / y / w / h. Only the nodes near the viewport are exposed as rows, so the
// view stays virtualized, and a node that survives a re-layout keeps its delegate: QML animates it to the new
// position, which is what makes a column change glide instead of jump.
class GalleryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qreal totalHeight READ totalHeight NOTIFY totalHeightChanged)
    Q_PROPERTY(bool reflowing READ reflowing NOTIFY reflowingChanged)
    Q_PROPERTY(int columns READ columns NOTIFY columnsChanged)
    Q_PROPERTY(qreal cardWidth READ cardWidth NOTIFY layoutChanged)
    Q_PROPERTY(qreal cardHeight READ cardHeight NOTIFY layoutChanged)
public:
    enum Kind { Card = 0, GroupHeader = 1, SubHeader = 2 };
    enum Roles { KindRole = Qt::UserRole + 1, EntryRole, XRole, YRole, WRole, HRole, TitleRole, CountRole,
                 SubCountRole, GroupIdRole, CollapsedRole, StaggerRole };

    struct Node {
        Kind kind = Card;
        QString key;
        EntryItem *entry = nullptr;
        QString title, groupId;
        int count = 0, subCount = 0;
        bool collapsed = false;
        // layout (filled by relayout)
        qreal x = 0, y = 0, w = 0, h = 0;
        int stagger = 0;      // position inside its card row: drives a small per-column delay in QML
    };

    explicit GalleryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(m_visible.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setNodes(const QList<Node> &nodes);                 // structure changed (filter, sort, new entry...)
    Q_INVOKABLE void setViewport(qreal width, qreal gridSize);   // geometry inputs; columns derive from them
    Q_INVOKABLE void setScroll(qreal contentY, qreal viewHeight);
    Q_INVOKABLE qreal yOf(const QString &path) const;        // top of the card for this path, -1 if absent
    int columns() const { return m_cols; }
    qreal cardWidth() const { return m_cardW; }
    qreal cardHeight() const { return m_cardH; }
    qreal totalHeight() const { return m_total; }
    bool reflowing() const { return m_reflowing; }

signals:
    void totalHeightChanged();
    void reflowingChanged();
    void columnsChanged();
    void layoutChanged();
    void scrollRequest(qreal y);       // keep the same content under the viewport after a re-layout

private:
    void relayout();
    void refreshWindow(bool dataDirty);
    void markReflow();
    int firstAt(qreal y) const;

    QList<Node> m_nodes;
    QHash<QString, int> m_index;       // key -> node index
    QStringList m_visible;             // keys currently exposed as rows, in row order
    qreal m_width = 0, m_gridSize = 160;
    int m_cols = 1;
    qreal m_cardW = 120, m_cardH = 150, m_total = 0;
    qreal m_scroll = 0, m_viewH = 0;
    QString m_anchorKey;               // node at the top of the viewport and its offset: scroll anchoring
    qreal m_anchorOffset = 0;
    bool m_reflowing = false;
    QTimer m_reflowTimer;
    static constexpr qreal kPad = 12, kGap = 8, kTop = 4, kBottom = 28, kBuffer = 700;
    static constexpr qreal kGroupH = 54, kSubH = 34;
};
