#include "ZoomSupport.h"

#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>
#include <qmath.h>


ZoomSupport::ZoomSupport(QGraphicsView* view)
    : QObject(view)
    , view_(view)
{
    view_->viewport()->installEventFilter(this);
    view_->setMouseTracking(true);
    modifiers_ = Qt::ControlModifier;
    zoomFactorBase_ = 1.0015;
}

void ZoomSupport::gentleZoom(double factor)
{
    view_->scale(factor, factor);
    view_->centerOn(targetScenePos_);
    QPointF deltaViewportPos = targetViewportPos_ - QPointF(view_->viewport()->width() / 2.0,
                                                           view_->viewport()->height() / 2.0);
    QPointF viewportCenter = view_->mapFromScene(targetScenePos_) - deltaViewportPos;
    view_->centerOn(view_->mapToScene(viewportCenter.toPoint()));

    emit zoomed();
}

void ZoomSupport::setModifiers(Qt::KeyboardModifiers modifiers)
{
    modifiers_ = modifiers;
}

void ZoomSupport::setZoomFactorBase(double value)
{
    zoomFactorBase_ = value;
}

bool ZoomSupport::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        QPointF delta = targetViewportPos_ - mouseEvent->pos();
        if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
            targetViewportPos_ = mouseEvent->pos();
            targetScenePos_ = view_->mapToScene(mouseEvent->pos());
        }
    } else if (event->type() == QEvent::Wheel) {
        auto wheelEvent = static_cast<QWheelEvent*>(event);
        if (QApplication::keyboardModifiers() == modifiers_) {
            double angle = wheelEvent->angleDelta().y();
            double factor = qPow(zoomFactorBase_, angle);
            gentleZoom(factor);
            return true;
        }
    }

    Q_UNUSED(object)

    return false;
}
