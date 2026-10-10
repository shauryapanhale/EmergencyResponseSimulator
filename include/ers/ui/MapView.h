#pragma once
#include <QGraphicsView>
#include <QPointF>
#include "ers/ui/ApplicationFacade.h"

class QGraphicsScene;
class QMouseEvent;
class QPainter;
class QResizeEvent;
class QShowEvent;
class QToolButton;
class QWheelEvent;

namespace ers {

// Presentation only: an interactive 2D map (wheel zoom, drag pan, zoom buttons,
// click-to-select node). It draws ApplicationFacade state and holds no engine logic.
class MapView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapView(ApplicationFacade& facade, QWidget* parent = nullptr);
    int selectedNode() const { return selectedNode_; }

public slots:
    void redraw();
    void fitAll();
    void zoomIn();
    void zoomOut();

signals:
    void nodeClicked(int nodeId);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void zoomBy(double factor, bool underMouse);
    void layoutButtons();
    void pickNode(const QPointF& viewPos);

    ApplicationFacade& facade_;
    QGraphicsScene* scene_;
    bool autoFit_ = true;
    double fitScale_ = 1.0;
    double minScale_ = 0.05;
    double maxScale_ = 20.0;
    int selectedNode_ = -1;
    QPointF pressPos_;
    QToolButton* zoomInBtn_ = nullptr;
    QToolButton* zoomOutBtn_ = nullptr;
    QToolButton* fitBtn_ = nullptr;
};

} // namespace ers