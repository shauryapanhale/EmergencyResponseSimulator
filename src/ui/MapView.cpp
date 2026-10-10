#include "ers/ui/MapView.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QToolButton>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <map>

namespace ers {

namespace {
constexpr double kScale = 70.0;

QPointF toScene(double x, double y) { return QPointF(x * kScale, y * kScale); }

void pinToScene(QGraphicsItem* item, const QPointF& scenePos, double z) {
    item->setPos(scenePos);
    item->setZValue(z);
    item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

// Text is a path so its pixel offset is part of the geometry; it keeps its size when zooming.
QGraphicsPathItem* addText(QGraphicsScene* scene, const QString& text, const QColor& color,
                           int pointSize, bool bold, const QPointF& anchor,
                           const QPointF& pixelOffset, double z) {
    QFont f;
    f.setPointSize(pointSize);
    f.setBold(bold);
    QPainterPath path;
    path.addText(0, 0, f, text);
    const QRectF br = path.boundingRect();
    path.translate(-br.center().x() + pixelOffset.x(), -br.center().y() + pixelOffset.y());
    auto* item = scene->addPath(path, QPen(Qt::NoPen), QBrush(color));
    pinToScene(item, anchor, z);
    return item;
}

QColor vehicleColor(const QString& type) {
    const QChar c = type.isEmpty() ? QChar('?') : type.at(0);
    if (c == 'A') return QColor(0, 120, 200);
    if (c == 'F') return QColor(210, 90, 0);
    return QColor(70, 50, 160);
}
} // namespace

MapView::MapView(ApplicationFacade& facade, QWidget* parent)
    : QGraphicsView(parent), facade_(facade), scene_(new QGraphicsScene(this)) {
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumHeight(260);

    auto makeButton = [this](const QString& text, int width) {
        auto* b = new QToolButton(viewport());
        b->setText(text);
        b->setFixedSize(width, 28);
        b->setStyleSheet(
            "QToolButton{background:white;color:#222;border:1px solid #888;font-weight:bold;}"
            "QToolButton:hover{background:#e3edff;}");
        return b;
    };
    zoomInBtn_ = makeButton("+", 28);
    zoomOutBtn_ = makeButton("-", 28);
    fitBtn_ = makeButton("Fit", 40);
    connect(zoomInBtn_, &QToolButton::clicked, this, &MapView::zoomIn);
    connect(zoomOutBtn_, &QToolButton::clicked, this, &MapView::zoomOut);
    connect(fitBtn_, &QToolButton::clicked, this, &MapView::fitAll);

    connect(&facade_, &ApplicationFacade::stateChanged, this, &MapView::redraw);
    redraw();
    layoutButtons();
}

void MapView::redraw() {
    scene_->clear();
    const CityMap& city = facade_.city();

    double minX = 1e12, minY = 1e12, maxX = -1e12, maxY = -1e12;
    for (const auto& entry : city.nodes()) {
        const QPointF p = toScene(entry.second.x(), entry.second.y());
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    if (city.nodes().empty()) {
        minX = minY = 0;
        maxX = maxY = kScale;
    }
    scene_->setSceneRect(QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).adjusted(-100, -90, 100, 90));

    for (const auto& entry : city.roads()) {
        const Road& road = entry.second;
        if (!city.hasNode(road.from()) || !city.hasNode(road.to())) continue;
        const CityNode& a = city.node(road.from());
        const CityNode& b = city.node(road.to());
        const QPointF p1 = toScene(a.x(), a.y());
        const QPointF p2 = toScene(b.x(), b.y());

        QPen pen;
        if (road.isBlocked()) {
            pen = QPen(QColor(200, 0, 0), 4, Qt::DashLine);
        } else {
            const double f = road.trafficFactor();
            QColor color(130, 130, 130);
            if (f > 1.5) color = QColor(190, 70, 0);
            else if (f > 1.0) color = QColor(230, 160, 0);
            pen = QPen(color, f > 1.0 ? 4 : 3);
        }
        pen.setCosmetic(true);
        auto* line = scene_->addLine(QLineF(p1, p2), pen);
        line->setZValue(0);
        line->setToolTip(QString("Road %1: %2 km, base %3 min, traffic x%4%5")
                             .arg(road.id())
                             .arg(road.distanceKm())
                             .arg(road.baseMinutes())
                             .arg(road.trafficFactor())
                             .arg(road.isBlocked() ? ", BLOCKED" : ""));

        addText(scene_, QString("R%1").arg(road.id()), QColor(100, 100, 100), 8, false,
                (p1 + p2) / 2.0, QPointF(0, -9), 1);
    }

    for (const auto& entry : city.nodes()) {
        const CityNode& n = entry.second;
        const QPointF p = toScene(n.x(), n.y());
        QGraphicsItem* shape = nullptr;
        if (n.kind() == NodeKind::Station) {
            shape = scene_->addRect(QRectF(-10, -10, 20, 20), QPen(Qt::black, 1),
                                    QBrush(QColor(60, 110, 200)));
        } else if (n.kind() == NodeKind::Landmark) {
            QPolygonF diamond;
            diamond << QPointF(0, -11) << QPointF(11, 0) << QPointF(0, 11) << QPointF(-11, 0);
            shape = scene_->addPolygon(diamond, QPen(Qt::black, 1), QBrush(QColor(70, 160, 90)));
        } else {
            shape = scene_->addEllipse(QRectF(-7, -7, 14, 14), QPen(Qt::black, 1),
                                       QBrush(QColor(170, 170, 170)));
        }
        pinToScene(shape, p, 2);
        shape->setToolTip(QString("Node %1: %2").arg(n.id()).arg(QString::fromStdString(n.name())));

        addText(scene_, QString::fromStdString(n.name()), QColor(30, 30, 30), 9, false, p,
                QPointF(0, 22), 3);

        if (n.id() == selectedNode_) {
            auto* sel = scene_->addEllipse(QRectF(-15, -15, 30, 30),
                                           QPen(QColor(0, 90, 255), 3, Qt::DashLine), Qt::NoBrush);
            pinToScene(sel, p, 3.5);
        }
    }

    std::map<int, int> perNode;
    for (const auto& i : facade_.incidents()) {
        if (i.status == "Resolved" || i.status == "Unserved") continue;
        if (!city.hasNode(i.nodeId)) continue;
        const CityNode& n = city.node(i.nodeId);
        const QPointF p = toScene(n.x(), n.y());
        const bool critical = i.priority.compare("Critical", Qt::CaseInsensitive) == 0;

        auto* ring = scene_->addEllipse(QRectF(-18, -18, 36, 36),
                                        QPen(QColor(210, 0, 0), critical ? 4 : 2), Qt::NoBrush);
        pinToScene(ring, p, 4);
        ring->setToolTip(QString("Incident %1 (%2, %3, %4)").arg(i.id).arg(i.type, i.priority, i.status));

        const int stack = perNode[i.nodeId]++;
        addText(scene_, QString("#%1 %2 [%3]").arg(i.id).arg(i.type, i.status), QColor(190, 0, 0), 9,
                true, p, QPointF(0, -34 - 14 * stack), 6);
    }

    for (const auto& v : facade_.vehicles()) {
        const QPointF base = toScene(v.x, v.y);
        const double off = ((v.id % 3) - 1) * 26.0;
        auto* box = scene_->addRect(QRectF(off - 12, 28, 24, 16), QPen(Qt::black, 1),
                                    QBrush(vehicleColor(v.type)));
        pinToScene(box, base, 5);
        box->setOpacity(v.status == "OutOfService" ? 0.4 : 1.0);
        box->setToolTip(QString("Vehicle %1: %2, %3, incident %4")
                            .arg(v.id)
                            .arg(v.type, v.status)
                            .arg(v.assignedIncident < 0 ? QString("none")
                                                        : QString::number(v.assignedIncident)));
        addText(scene_, QString("%1%2").arg(v.type.left(1)).arg(v.id), Qt::white, 8, true, base,
                QPointF(off, 36), 7);
    }

    if (autoFit_) fitAll();
}

void MapView::fitAll() {
    if (viewport()->width() <= 0 || viewport()->height() <= 0) return;
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    fitScale_ = transform().m11();
    minScale_ = fitScale_ * 0.7;
    maxScale_ = fitScale_ * 14.0;
    autoFit_ = true;
}

void MapView::zoomBy(double factor, bool underMouse) {
    const double current = transform().m11();
    const double target = std::clamp(current * factor, minScale_, maxScale_);
    if (std::abs(target - current) < 1e-9) return;
    setTransformationAnchor(underMouse ? QGraphicsView::AnchorUnderMouse
                                       : QGraphicsView::AnchorViewCenter);
    scale(target / current, target / current);
    autoFit_ = false;
}

void MapView::zoomIn() { zoomBy(1.4, false); }
void MapView::zoomOut() { zoomBy(1.0 / 1.4, false); }

void MapView::layoutButtons() {
    zoomInBtn_->move(10, 10);
    zoomOutBtn_->move(10, 42);
    fitBtn_->move(10, 74);
    zoomInBtn_->raise();
    zoomOutBtn_->raise();
    fitBtn_->raise();
}

void MapView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (autoFit_) fitAll();
    layoutButtons();
}

void MapView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (autoFit_) fitAll();
    layoutButtons();
}

void MapView::wheelEvent(QWheelEvent* event) {
    zoomBy(event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2, true);
    event->accept();
}

void MapView::mousePressEvent(QMouseEvent* event) {
    pressPos_ = event->position();
    QGraphicsView::mousePressEvent(event);
}

void MapView::mouseReleaseEvent(QMouseEvent* event) {
    QGraphicsView::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton) return;
    const QPointF d = event->position() - pressPos_;
    if (std::abs(d.x()) + std::abs(d.y()) < 5.0) pickNode(event->position());
    else autoFit_ = false;
}

void MapView::mouseDoubleClickEvent(QMouseEvent* event) {
    zoomBy(1.8, true);
    event->accept();
}

void MapView::pickNode(const QPointF& viewPos) {
    const CityMap& city = facade_.city();
    int best = -1;
    double bestDist = 18.0;
    for (const auto& entry : city.nodes()) {
        const CityNode& n = entry.second;
        const QPointF vp = mapFromScene(toScene(n.x(), n.y()));
        const QPointF d = vp - viewPos;
        const double dist = std::abs(d.x()) + std::abs(d.y());
        if (dist < bestDist) {
            bestDist = dist;
            best = n.id();
        }
    }
    selectedNode_ = best;
    redraw();
    if (best >= 0) emit nodeClicked(best);
}

void MapView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor(238, 241, 232));
    QPen grid(QColor(214, 220, 206));
    grid.setCosmetic(true);
    grid.setWidth(1);
    painter->setPen(grid);
    for (double x = std::floor(rect.left() / kScale) * kScale; x <= rect.right(); x += kScale)
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    for (double y = std::floor(rect.top() / kScale) * kScale; y <= rect.bottom(); y += kScale)
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
}

} // namespace ers