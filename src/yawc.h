/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#ifndef CURRENTAPPCONTROL_HEADER
#define CURRENTAPPCONTROL_HEADER

#include <Plasma/Applet>
#include <KActivities/Consumer>

#include "ui_yawc.h"

class TitleWidget;
class QGraphicsLinearLayout;

namespace Plasma
{
    class IconWidget;
    class ItemBackground;
    class Dialog;
}

class YetAnotherWindowControl : public Plasma::Applet
{
    Q_OBJECT
public:

    YetAnotherWindowControl(QObject *parent, const QVariantList &args);
    ~YetAnotherWindowControl();

    void init();
    void configChanged();
    void constraintsEvent(Plasma::Constraints constraints);

    void createConfigurationInterface(KConfigDialog *parent);

protected:
    int windowsCount() const;

protected Q_SLOTS:
    void activeWindowChanged(WId id);
    void windowChanged(WId id);
    void windowRemoved(WId id);
    void setSyncDelay(bool delay);
    void syncActiveWindow();
    void closeWindow();
    void toggleMaximizedWindow();
    void toggleMinimizedWindow();
    void toggleTop();
    void toggleAllDesktop();
    void listWindows();
    void configAccepted();

private:
    void updateSize();
    QStringList m_allItem;
    QStringList m_activeItem;
    QMap<QString, QString> m_itemName;
    QMap<QString, Plasma::IconWidget*> m_itemMap;

    TitleWidget *m_currentTask;
    TitleWidget* m_currentTaskTitle;
    Plasma::IconWidget* m_topTask;
    Plasma::IconWidget* m_alldesktopTask;
    Plasma::IconWidget* m_minimizeTask;
    Plasma::IconWidget *m_maximizeTask;
    Plasma::IconWidget *m_closeTask;

    QGraphicsLinearLayout *m_appletLayout;

    bool m_syncDelay;
    WId m_activeWindow;
    WId m_lastActiveWindow;
    WId m_pendingActiveWindow;

    Ui::GeneralConfig m_generalUi;
    QString m_toolTipText;
    bool m_enableDrag;
    bool m_borderlessMaximize;
    int m_titleWidth;

    KActivities::Consumer m_consumer;
};
#endif
