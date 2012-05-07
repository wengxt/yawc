#include <QGraphicsSceneMouseEvent>
#include <QX11Info>

#include <KWindowSystem>
#include <NETWinInfo>

#include "titlewidget.h"

TitleWidget::TitleWidget(QGraphicsItem* parent): Plasma::IconWidget(parent)
    ,m_drag(false)
{

}
TitleWidget::~TitleWidget()
{

}

void TitleWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    Plasma::IconWidget::mouseMoveEvent(event);
    if (m_drag && (event->buttons() & Qt::LeftButton))
    {
        WId wid = KWindowSystem::activeWindow();
        if (wid) {
            XUngrabPointer(QX11Info::display(), QX11Info::appTime());
            NETRootInfo rootInfo(QX11Info::display(), NET::WMMoveResize);
            KWindowInfo info = KWindowSystem::windowInfo(wid, NET::WMGeometry);
            int x;
            int y;
            QPointF p = event->screenPos();
            if (p.x() > info.geometry().x() && p.x() < info.geometry().x() + info.geometry().width())
                x = p.x();
            else
                x = info.geometry().x();
            if (p.y() > info.geometry().y() && p.y() < info.geometry().y() + info.geometry().height())
                y = p.y();
            else
                y = info.geometry().y();
            rootInfo.moveResizeRequest( wid, x, y, NET::Move);
        }

    }
}

void TitleWidget::setEnableDragWindow(bool enable)
{
    m_drag = enable;
}