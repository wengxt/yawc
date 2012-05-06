#ifndef TITLEWIDGET_H
#define TITLEWIDGET_H

#include <Plasma/IconWidget>
class TitleWidget : public Plasma::IconWidget
{
    Q_OBJECT
public:
    explicit TitleWidget(QGraphicsItem* parent = 0);
    virtual ~TitleWidget();

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
    void setEnableDragWindow(bool enable = true);
private:
    bool m_drag;
};

#endif // TITLEWIDGET_H