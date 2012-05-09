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

#include "yawc.h"
#include "titlewidget.h"

//Qt
#include <QGraphicsLinearLayout>
#include <QX11Info>
#include <QTimer>
#include <QFontMetrics>
#include <QListWidget>
#include <QVarLengthArray>
#include <QDBusInterface>

//KDE
#include <KConfigDialog>
#include <KWindowSystem>
#include <KIconLoader>
#include <KIcon>
#include <NETWinInfo>

//Plasma
#include <Plasma/IconWidget>
#include <Plasma/ItemBackground>
#include <Plasma/View>
#include <Plasma/Theme>
#include <Plasma/Dialog>
#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/WindowEffects>
#include <Plasma/ToolTipContent>
#include <Plasma/ToolTipManager>


YetAnotherWindowControl::YetAnotherWindowControl(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
    ,m_currentTask(new TitleWidget(this))
    ,m_currentTaskTitle(new TitleWidget(this))
    ,m_topTask(new Plasma::IconWidget(this))
    ,m_alldesktopTask(new Plasma::IconWidget(this))
    ,m_minimizeTask(new Plasma::IconWidget(this))
    ,m_maximizeTask(new Plasma::IconWidget(this))
    ,m_closeTask(new Plasma::IconWidget(this))
    ,m_appletLayout(new QGraphicsLinearLayout(Qt::Horizontal))
    ,m_syncDelay(false)
    ,m_activeWindow(0)
    ,m_lastActiveWindow(0)
    ,m_pendingActiveWindow(0)
    ,m_borderlessMaximize(false)
    ,m_titleWidth(15)
{
    m_allItem << "Icon" << "Title" << "Top" << "AllDesktop" << "Min" << "Max" << "Close";
    m_itemName["Icon"] = i18n("Icon");
    m_itemName["Title"] = i18n("Window Title");
    m_itemName["Top"] = i18n("Always on Top");
    m_itemName["AllDesktop"] = i18n("Show on all desktop");
    m_itemName["Min"] = i18n("Minimize window");
    m_itemName["Max"] = i18n("Maximize window");
    m_itemName["Close"] = i18n("Close window");
    m_itemMap["Icon"] = m_currentTask;
    m_itemMap["Title"] =m_currentTaskTitle;
    m_itemMap["Top"] = m_topTask;
    m_itemMap["AllDesktop"] = m_alldesktopTask;
    m_itemMap["Min"] = m_minimizeTask;
    m_itemMap["Max"] = m_maximizeTask;
    m_itemMap["Close"] = m_closeTask;

    setLayout(m_appletLayout);
    m_appletLayout->setContentsMargins(0, 0, 0, 0);
    m_appletLayout->setSpacing(0);

    m_currentTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_currentTaskTitle->setTextBackgroundColor(QColor());
    m_currentTaskTitle->setTextBackgroundColor(QColor(Qt::transparent));

    m_closeTask->setSvg("widgets/configuration-icons", "close");
    m_closeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_topTask->setSvg("widgets/configuration-icons", "size-vertical");
    m_topTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_alldesktopTask->setSvg("widgets/configuration-icons", "add");
    m_alldesktopTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_maximizeTask->setSvg("widgets/configuration-icons", "maximize");
    m_maximizeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);
    m_maximizeTask->setZValue(999);

    m_minimizeTask->setSvg("widgets/configuration-icons", "collapse");
    m_minimizeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);
    m_minimizeTask->setZValue(999);

    connect(m_closeTask, SIGNAL(clicked()), this, SLOT(closeWindow()));
    connect(m_closeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_maximizeTask, SIGNAL(clicked()), this, SLOT(toggleMaximizedWindow()));
    connect(m_maximizeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_minimizeTask, SIGNAL(clicked()), this, SLOT(toggleMinimizedWindow()));
    connect(m_minimizeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_alldesktopTask, SIGNAL(clicked()), this, SLOT(toggleAllDesktop()));
    connect(m_alldesktopTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_topTask, SIGNAL(clicked()), this, SLOT(toggleTop()));
    connect(m_topTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_currentTask, SIGNAL(clicked()), this, SLOT(listWindows()));
    connect(m_currentTaskTitle, SIGNAL(clicked()), this, SLOT(listWindows()));

    connect(&m_consumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(syncActiveWindow()));
}


YetAnotherWindowControl::~YetAnotherWindowControl()
{
    KWindowSystem::self()->disconnect(0, this, 0);
    Plasma::ToolTipManager::self()->unregisterWidget(this);
}

void YetAnotherWindowControl::init()
{
    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)),
            this, SLOT(activeWindowChanged(WId)));
    connect(KWindowSystem::self(), SIGNAL(windowChanged(WId)),
            this, SLOT(windowChanged(WId)));
    connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)),
            this, SLOT(windowRemoved(WId)));
    activeWindowChanged(KWindowSystem::activeWindow());
    configChanged();
}

void YetAnotherWindowControl::configChanged()
{
    QStringList defaultList;
    defaultList << "Icon" << "Title" << "Min" << "Max" << "Close";
    m_activeItem = config().readEntry("ActiveItem", defaultList);
    m_enableDrag = config().readEntry("WindowDrag", true);
    m_titleWidth = config().readEntry("TitleWidth", 15);
    KConfig config("kwinrc");
    KConfigGroup cg(&config, "Windows");
    m_borderlessMaximize = cg.readEntry("BorderlessMaximizedWindows", false);

    while(m_appletLayout->count() > 0)
        m_appletLayout->removeAt(0);

    foreach(Plasma::IconWidget* item, m_itemMap.values()) {
        item->hide();
    }

    foreach(const QString& item, m_activeItem) {
        if (m_itemMap.contains(item)) {
            Plasma::IconWidget* widget = m_itemMap[item];
            m_appletLayout->addItem(widget);
            widget->show();
        }
    }

    if (m_enableDrag) {
        m_currentTask->setEnableDragWindow(m_enableDrag);
        m_currentTaskTitle->setEnableDragWindow(m_enableDrag);
    }

    updateSize();
}

void YetAnotherWindowControl::constraintsEvent(Plasma::Constraints constraints)
{

    if ((constraints & Plasma::FormFactorConstraint) ||
        (constraints & Plasma::SizeConstraint) ) {
        updateSize();
    }
}

void YetAnotherWindowControl::updateSize()
{
    QFontMetrics fm(Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont));
    if (formFactor() == Plasma::Vertical) {
        m_currentTaskTitle->setOrientation(Qt::Vertical);
        //FIXME: all this minimum/maximum sizes shouldn't be necessary
        m_currentTaskTitle->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        m_currentTaskTitle->setMinimumSize(0, 0);
    } else {
        m_currentTaskTitle->setOrientation(Qt::Horizontal);
        int width = 0;
        if (m_activeItem.contains("Title"))
            width = qMin((qreal)(fm.width('M')*30), containment()->size().width() * m_titleWidth / 100 );
        else
            width = 0;

        m_currentTaskTitle->setMaximumSize(width, QWIDGETSIZE_MAX);
        m_currentTaskTitle->setMinimumSize(width, 0);
    }
}


void YetAnotherWindowControl::windowChanged(WId id)
{
    bool applicationActive = false;

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
         if (widget->winId() == id) {
             applicationActive = true;
             break;
         }
    }
    if (!applicationActive && id == m_activeWindow) {
        m_pendingActiveWindow = m_activeWindow;
        syncActiveWindow();
    }
}

void YetAnotherWindowControl::activeWindowChanged(WId id)
{
    m_pendingActiveWindow = id;
    //delay the switch to permit to pass the close action to the proper window if our view accepts focus
    if (!m_syncDelay) {
        syncActiveWindow();
    }
}

void YetAnotherWindowControl::windowRemoved(WId id)
{
    Q_UNUSED(id);
    QTimer::singleShot(300, this, SLOT(syncActiveWindow()));
}

void YetAnotherWindowControl::syncActiveWindow()
{
    m_syncDelay = false;
    bool applicationActive = false;

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
         if (widget->winId() == m_pendingActiveWindow ||
             widget->winId() == KWindowSystem::activeWindow()) {
             applicationActive = true;
             break;
         }
     }

    Plasma::ToolTipContent toolTipData;
    toolTipData.setAutohide(true);
    toolTipData.setSubText(i18n("Click here to have an overview of all the running applications"));

    if (applicationActive) {
        m_activeWindow = 0;
        int count = windowsCount();
        m_currentTask->setIcon("preferences-system-windows");
        const int activeWindows = qMax(0, count-1);
        if (activeWindows) {
            m_toolTipText = i18np("%1 running app", "%1 running apps", activeWindows);
        } else {
            m_toolTipText = i18n("No running apps");
        }
        m_closeTask->hide();
        m_maximizeTask->hide();
        m_minimizeTask->hide();
        m_alldesktopTask->hide();
        m_topTask->hide();

        m_currentTaskTitle->setText(m_toolTipText);
        m_currentTaskTitle->update();

        toolTipData.setMainText(m_toolTipText);
        toolTipData.setImage(KIcon("preferences-system-windows"));

    } else if (m_pendingActiveWindow <= 0) {
        toolTipData.setMainText(m_toolTipText);
        //toolTipData.setImage(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeHuge, KIconLoader::SizeHuge));
    } else {
        m_activeWindow = m_pendingActiveWindow;
        m_lastActiveWindow = m_pendingActiveWindow;
        KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMName|NET::WMState|NET::WMDesktop);
        m_currentTask->setIcon(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium));
        m_toolTipText = info.name();

        m_currentTaskTitle->setText(m_toolTipText);
        m_currentTaskTitle->update();

        foreach(const QString& item, m_activeItem) {
            if (m_itemMap.contains(item)) {
                Plasma::IconWidget* widget = m_itemMap[item];
                widget->show();
            }
        }

        toolTipData.setMainText(info.name());
        toolTipData.setImage(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeHuge, KIconLoader::SizeHuge));

        if (info.state() & (NET::MaxVert|NET::MaxHoriz)) {
            m_maximizeTask->setSvg("widgets/configuration-icons", "unmaximize");
        } else {
            m_maximizeTask->setSvg("widgets/configuration-icons", "maximize");
        }
        m_alldesktopTask->setSvg("widgets/configuration-icons", info.onAllDesktops() ? "remove" : "add");
    }

    Plasma::ToolTipManager::self()->registerWidget(this);
    Plasma::ToolTipManager::self()->setContent(m_currentTask, toolTipData);
    m_pendingActiveWindow = 0;
}

void YetAnotherWindowControl::setSyncDelay(bool delay)
{
    m_syncDelay = delay;
}

void YetAnotherWindowControl::closeWindow()
{
    m_syncDelay = false;

    if (m_activeWindow) {
#ifdef Q_WS_X11
        NETRootInfo ri( QX11Info::display(), NET::CloseWindow );
        ri.closeWindowRequest(m_activeWindow);
#endif
    }

    syncActiveWindow();
}

void YetAnotherWindowControl::toggleMinimizedWindow()
{
    //TODO: change the icon
#ifdef Q_WS_X11
    KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    if (info.isMinimized()) {
        KWindowSystem::unminimizeWindow(m_activeWindow);
    } else {
        KWindowSystem::minimizeWindow(m_activeWindow);
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(m_activeWindow);
    }
#endif
}

void YetAnotherWindowControl::toggleAllDesktop()
{
#ifdef Q_WS_X11
    KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMDesktop);
    bool onAllDesktops = info.onAllDesktops();

    KWindowSystem::setOnAllDesktops(m_activeWindow, !onAllDesktops);
    m_alldesktopTask->setSvg("widgets/configuration-icons", !onAllDesktops ? "remove" : "add");
#endif
}

void YetAnotherWindowControl::toggleTop()
{
#ifdef Q_WS_X11
    KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    NETWinInfo ni(QX11Info::display(), m_activeWindow, QX11Info::appRootWindow(), NET::WMState);

    if (!(ni.state() & NET::KeepAbove)) {
        ni.setState(NET::KeepAbove, NET::KeepAbove);
    } else {
        ni.setState(0, NET::KeepAbove);
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(m_activeWindow);
    }
#endif
}

void YetAnotherWindowControl::toggleMaximizedWindow()
{
#ifdef Q_WS_X11
    KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    NETWinInfo ni(QX11Info::display(), m_activeWindow, QX11Info::appRootWindow(), NET::WMState);

    if (!(ni.state() & NET::Max)) {
        ni.setState(NET::Max, NET::Max);
        m_maximizeTask->setSvg("widgets/configuration-icons", "unmaximize");
    } else {
        ni.setState(0, NET::Max);
        m_maximizeTask->setSvg("widgets/configuration-icons", "maximize");
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(m_activeWindow);
    }
#endif
}

void YetAnotherWindowControl::listWindows()
{
    QGraphicsView *v = view();
    if (v) {
        KWindowSystem::forceActiveWindow(v->winId());
    }

    if (Plasma::WindowEffects::isEffectAvailable(Plasma::WindowEffects::PresentWindows)) {
        Plasma::WindowEffects::presentWindows(view()->winId());
    } else {
        KWindowSystem::forceActiveWindow(m_lastActiveWindow);
    }
}

void YetAnotherWindowControl::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    m_generalUi.setupUi(widget);
    parent->addPage(widget, i18nc("General configuration page", "General"), icon());

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    foreach(const QString& item,m_activeItem) {
        if (m_allItem.contains(item)) {
            QListWidgetItem *listItem = new QListWidgetItem( m_generalUi.itemSelector->selectedListWidget());
            listItem->setText(m_itemName[item]);
            listItem->setData(Qt::UserRole, item);
            m_generalUi.itemSelector->selectedListWidget()->addItem(listItem);
        }
    }
    foreach(const QString& item,m_allItem) {
        if (!m_activeItem.contains(item)) {
            QListWidgetItem *listItem = new QListWidgetItem( m_generalUi.itemSelector->availableListWidget());
            listItem->setText(m_itemName[item]);
            listItem->setData(Qt::UserRole, item);
            m_generalUi.itemSelector->availableListWidget()->addItem(listItem);
        }
    }

    m_generalUi.borderlessMaximizedWindow->setChecked(m_borderlessMaximize);
    m_generalUi.windowDrag->setChecked(m_enableDrag);
    connect(m_generalUi.windowSizeSpinBox, SIGNAL(valueChanged(int)), parent, SLOT(settingsModified()));
    connect(m_generalUi.itemSelector, SIGNAL(movedUp(QListWidgetItem*)), parent, SLOT(settingsModified()));
    connect(m_generalUi.itemSelector, SIGNAL(movedDown(QListWidgetItem*)), parent, SLOT(settingsModified()));
    connect(m_generalUi.itemSelector, SIGNAL(added(QListWidgetItem*)), parent, SLOT(settingsModified()));
    connect(m_generalUi.itemSelector, SIGNAL(removed(QListWidgetItem*)), parent, SLOT(settingsModified()));
    connect(m_generalUi.borderlessMaximizedWindow, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(m_generalUi.windowDrag, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
}

void YetAnotherWindowControl::configAccepted()
{
    m_enableDrag = m_generalUi.windowDrag->checkState() == Qt::Checked;
    m_titleWidth = m_generalUi.windowSizeSpinBox->value();
    m_activeItem.clear();
    for ( int i = 0; i < m_generalUi.itemSelector->selectedListWidget()->count(); ++i ) {
        m_activeItem << m_generalUi.itemSelector->selectedListWidget()->item(i)->data(Qt::UserRole).toString();
    }
    bool borderlessMaximize =  m_generalUi.borderlessMaximizedWindow->checkState() == Qt::Checked;
    config().writeEntry("ActiveItem", m_activeItem);
    config().writeEntry("WindowDrag", m_enableDrag);
    config().writeEntry("TitleWidth", m_titleWidth);
    if (borderlessMaximize != m_borderlessMaximize) {
        m_borderlessMaximize = borderlessMaximize;
        KConfig config("kwinrc");
        KConfigGroup cg(&config, "Windows");
        cg.writeEntry("BorderlessMaximizedWindows", m_borderlessMaximize);

        QDBusInterface kwin("org.kde.kwin", "/KWin", "org.kde.KWin");
        kwin.call("reconfigure");
    }
}

int YetAnotherWindowControl::windowsCount() const
{
    int count = 0;
    foreach(WId window, KWindowSystem::stackingOrder()) {
        const unsigned long prop[] = {NET::WMWindowType | NET::WMPid | NET::WMState, NET::WM2Activities};
        NETWinInfo ni(QX11Info::display(), window, QX11Info::appRootWindow(), prop, 2);
        QString activity = QString(ni.activities());
        if (!(ni.state() & NET::SkipTaskbar) &&
            ni.windowType(NET::NormalMask | NET::DialogMask |
                            NET::OverrideMask | NET::UtilityMask) != NET::Utility &&
            ni.windowType(NET::NormalMask | NET::DialogMask |
                            NET::OverrideMask | NET::UtilityMask | NET::DockMask) != NET::Dock &&
            (activity.isEmpty() || activity.contains(m_consumer.currentActivity()))
           ) {
            ++count;
        }
    }
    return count;
}

K_EXPORT_PLASMA_APPLET(yawc, YetAnotherWindowControl)

#include "yawc.moc"