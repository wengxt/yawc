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
            rootInfo.moveResizeRequest( wid, event->pos().x(), event->pos().y(), NET::Move);
        }

    }
}

void TitleWidget::setEnableDragWindow(bool enable)
{
    m_drag = enable;
}


#include "titlewidget.moc"