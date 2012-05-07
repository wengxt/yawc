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
    : Plasma::Applet(parent, args),
      m_syncDelay(false),
      m_activeWindow(0),
      m_lastActiveWindow(0),
      m_pendingActiveWindow(0),
      m_listDialog(0),
      m_listWidget(0),
      m_showMaximize(true),
      m_showMinimize(true),
      m_alwaysUseDialog(false),
      m_showTitle(true),
      m_showIcon(true),
      m_showClose(true),
      m_borderlessMaximize(false),
      m_titleWidth(15)
{
    m_currentTask = new TitleWidget(this);
    m_currentTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_currentTaskTitle = new TitleWidget(this);
    m_currentTaskTitle->setTextBackgroundColor(QColor());
    m_currentTaskTitle->setTextBackgroundColor(QColor(Qt::transparent));

    m_closeTask = new Plasma::IconWidget(this);
    m_closeTask->setSvg("widgets/configuration-icons", "close");
    m_closeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);

    m_maximizeTask = new Plasma::IconWidget(this);
    m_maximizeTask->setSvg("widgets/configuration-icons", "maximize");
    m_maximizeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);
    m_maximizeTask->setZValue(999);

    m_minimizeTask = new Plasma::IconWidget(this);
    m_minimizeTask->setSvg("widgets/configuration-icons", "collapse");
    m_minimizeTask->setMaximumWidth(KIconLoader::SizeSmallMedium);
    m_minimizeTask->setZValue(999);

    connect(m_closeTask, SIGNAL(clicked()), this, SLOT(closeWindow()));
    connect(m_closeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_maximizeTask, SIGNAL(clicked()), this, SLOT(toggleMaximizedWindow()));
    connect(m_maximizeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_minimizeTask, SIGNAL(clicked()), this, SLOT(toggleMinimizedWindow()));
    connect(m_minimizeTask, SIGNAL(pressed(bool)), this, SLOT(setSyncDelay(bool)));
    connect(m_currentTask, SIGNAL(clicked()), this, SLOT(listWindows()));
    connect(m_currentTaskTitle, SIGNAL(clicked()), this, SLOT(listWindows()));
}


YetAnotherWindowControl::~YetAnotherWindowControl()
{
}

void YetAnotherWindowControl::init()
{
    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)),
            this, SLOT(activeWindowChanged(WId)));
    connect(KWindowSystem::self(), SIGNAL(windowChanged(WId)),
            this, SLOT(windowChanged(WId)));
    connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)),
            this, SLOT(windowRemoved(WId)));
    QGraphicsLinearLayout *lay = new QGraphicsLinearLayout(Qt::Horizontal, this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    activeWindowChanged(KWindowSystem::activeWindow());
    configChanged();
}

void YetAnotherWindowControl::configChanged()
{
    m_showIcon = config().readEntry("ShowIcon", true);
    m_showMinimize = config().readEntry("ShowMinimize", true);
    m_showMaximize = config().readEntry("ShowMaximize", true);
    m_alwaysUseDialog = config().readEntry("AlwaysUseDialog", false);
    m_showTitle = config().readEntry("ShowWindowTitle", true);
    m_showClose = config().readEntry("ShowClose", true);
    m_enableDrag = config().readEntry("WindowDrag", true);
    m_titleWidth = config().readEntry("TitleWidth", 15);
    KConfig config("kwinrc");
    KConfigGroup cg(&config, "Windows");
    m_borderlessMaximize = cg.readEntry("BorderlessMaximizedWindows", false);

    QGraphicsLinearLayout *lay = static_cast<QGraphicsLinearLayout *>(layout());

    while(lay->count() > 0)
        lay->removeAt(0);

    if (m_showIcon) {
        lay->addItem(m_currentTask);
        m_currentTask->show();
    }
    else {
        lay->removeItem(m_currentTask);
        m_currentTask->hide();
    }

    if (m_showTitle) {
        lay->addItem(m_currentTaskTitle);
        m_currentTaskTitle->show();
    }
    else {
        lay->removeItem(m_currentTaskTitle);
        m_currentTaskTitle->hide();
    }

    if (m_showMinimize) {
        lay->addItem(m_minimizeTask);
        m_minimizeTask->show();
    } else {
        lay->removeItem(m_minimizeTask);
        m_minimizeTask->hide();
    }

    if (m_showMaximize) {
        lay->addItem(m_maximizeTask);
        m_maximizeTask->show();
    } else {
        lay->removeItem(m_maximizeTask);
        m_maximizeTask->hide();
    }

    if (m_showClose) {
        lay->addItem(m_closeTask);
        m_closeTask->show();
    } else {
        lay->removeItem(m_closeTask);
        m_closeTask->hide();
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
        if (m_showTitle)
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
    Q_UNUSED(id)

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

    Plasma::ToolTipContent toolTipData = Plasma::ToolTipContent();
    toolTipData.setAutohide(true);
    toolTipData.setSubText(i18n("Click here to have an overview of all the running applications"));

    if (applicationActive && m_pendingActiveWindow > 0) {
        m_activeWindow = 0;
        m_currentTask->setIcon("preferences-system-windows");
        const int activeWindows = qMax(0, windowsCount()-1);
        if (activeWindows) {
            m_toolTipText = i18np("%1 running app", "%1 running apps", activeWindows);
        } else {
            m_toolTipText = i18n("No running apps");
        }
        m_closeTask->hide();
        m_maximizeTask->hide();
        m_minimizeTask->hide();

        m_currentTaskTitle->setText(m_toolTipText);
        m_currentTaskTitle->update();

        toolTipData.setMainText(m_toolTipText);
        toolTipData.setImage(KIcon("preferences-system-windows"));

    } else if (m_pendingActiveWindow <= 0) {
        toolTipData.setMainText(m_toolTipText);
        toolTipData.setImage(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeHuge, KIconLoader::SizeHuge));
    } else {
        m_activeWindow = m_pendingActiveWindow;
        m_lastActiveWindow = m_pendingActiveWindow;
        KWindowInfo info = KWindowSystem::windowInfo(m_activeWindow, NET::WMName|NET::WMState);
        m_currentTask->setIcon(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium));
        m_toolTipText = info.name();

        m_currentTaskTitle->setText(m_toolTipText);
        m_currentTaskTitle->update();

        if (m_showClose) {
            m_closeTask->show();
        }
        if (m_showMaximize) {
            m_maximizeTask->show();
        }
        if (m_showMinimize) {
            m_minimizeTask->show();
        }

        toolTipData.setMainText(info.name());
        toolTipData.setImage(KWindowSystem::icon(m_activeWindow, KIconLoader::SizeHuge, KIconLoader::SizeHuge));

        if (info.state() & (NET::MaxVert|NET::MaxHoriz)) {
            m_maximizeTask->setSvg("widgets/configuration-icons", "unmaximize");
        } else {
            m_maximizeTask->setSvg("widgets/configuration-icons", "maximize");
        }
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


void YetAnotherWindowControl::toggleMaximizedWindow()
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

    if (!m_alwaysUseDialog && Plasma::WindowEffects::isEffectAvailable(Plasma::WindowEffects::PresentWindows)) {
        Plasma::WindowEffects::presentWindows(view()->winId() , KWindowSystem::currentDesktop());
    } else if (!m_listDialog || !m_listDialog->isVisible()) {
         if (!m_listDialog) {
            m_listDialog = new Plasma::Dialog();
            m_listWidget = new QGraphicsWidget(this);
            m_listWidget->installEventFilter(this);
            m_listWidget->setAcceptHoverEvents(true);
            m_listDialog->setGraphicsWidget(m_listWidget);
            Plasma::Corona *corona = 0;
            if (containment() && containment()->corona()) {
                corona = containment()->corona();
                corona->addOffscreenWidget(m_listWidget);
            }

            m_listDialog->setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog);
            KWindowSystem::setType(m_listDialog->winId(), NET::PopupMenu);
            KWindowSystem::setState(m_listDialog->winId(), NET::SkipTaskbar);
            m_listDialog->installEventFilter(this);

            connect(m_listDialog, SIGNAL(destroyed()), this, SLOT(closePopup()));

            m_layout = new QGraphicsLinearLayout(m_listWidget);
            m_layout->setOrientation(Qt::Vertical);
            m_itemBackground = new Plasma::ItemBackground(m_listWidget);
        } else {
            QHash<Plasma::IconWidget *, WId>::const_iterator i = m_windowIcons.constBegin();
            while (i != m_windowIcons.constEnd()) {
                i.key()->hide();
                m_oldIcons << i.key();
                ++i;
            }
            m_windowIcons.clear();
        }

        m_itemBackground->hide();

        Plasma::WindowEffects::slideWindow(m_listDialog, location());

        foreach(WId window, KWindowSystem::stackingOrder()) {
            KWindowInfo info = KWindowSystem::windowInfo(window, NET::WMName|NET::WMState|NET::WMWindowType);
            NET::WindowType type = info.windowType(NET::AllTypesMask);
            if (!(info.state() & NET::SkipTaskbar) &&
                ((type == NET::Normal) || (type == NET::Unknown))) {
                Plasma::IconWidget *icon;
                if (m_oldIcons.isEmpty()) {
                    icon = new Plasma::IconWidget(m_listWidget);
                    icon->setTextBackgroundColor(QColor());
                    icon->setTextBackgroundColor(QColor(Qt::transparent));
                    icon->setDrawBackground(false);
                    icon->setPreferredIconSize(QSizeF(KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium));

                    qreal left, top, right, bottom;
                    m_itemBackground->getContentsMargins(&left, &top, &right, &bottom);
                    icon->setContentsMargins(left, top, right, bottom);
                    icon->installEventFilter(this);
                } else {
                    icon = m_oldIcons.first();
                    m_oldIcons.pop_front();
                }

                icon->setOrientation(Qt::Horizontal);
                icon->setText(info.name());
                icon->setIcon(KWindowSystem::icon(window, KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium));
                icon->setMinimumSize(icon->effectiveSizeHint(Qt::PreferredSize));
                connect(icon, SIGNAL(clicked()), this, SLOT(windowItemClicked()));
                m_windowIcons[icon] = window;
                m_layout->addItem(icon);
                icon->show();
            }
        }

        if (containment() && containment()->corona()) {
            m_listDialog->move(containment()->corona()->popupPosition(this, m_listDialog->size()));
        }

        m_listDialog->show();
    } else {
        closePopup();
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

    m_generalUi.alwaysUseDialog->setChecked(m_alwaysUseDialog);
    m_generalUi.showMaximize->setChecked(m_showMaximize);
    m_generalUi.showMinimize->setChecked(m_showMinimize);
    m_generalUi.showTitle->setChecked(m_showTitle);
    m_generalUi.showIcon->setChecked(m_showIcon);
    m_generalUi.showClose->setChecked(m_showClose);
    m_generalUi.borderlessMaximizedWindow->setChecked(m_borderlessMaximize);
    m_generalUi.windowDrag->setChecked(m_enableDrag);
    connect(m_generalUi.alwaysUseDialog, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.showMaximize, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.showMinimize, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.showTitle, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.showIcon, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.showClose, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.borderlessMaximizedWindow, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
    connect(m_generalUi.windowDrag, SIGNAL(toggled(bool)), parent, SLOT(settingsChangedSlot()));
}

void YetAnotherWindowControl::configAccepted()
{
    m_alwaysUseDialog = m_generalUi.alwaysUseDialog->checkState() == Qt::Checked;
    m_showMaximize = m_generalUi.showMaximize->checkState() == Qt::Checked;
    m_showMinimize = m_generalUi.showMinimize->checkState() == Qt::Checked;
    m_showTitle = m_generalUi.showTitle->checkState() == Qt::Checked;
    m_showIcon = m_generalUi.showIcon->checkState() == Qt::Checked;
    m_showClose = m_generalUi.showClose->checkState() == Qt::Checked;
    m_enableDrag = m_generalUi.windowDrag->checkState() == Qt::Checked;
    m_titleWidth = m_generalUi.windowSizeSlider->value();
    bool borderlessMaximize =  m_generalUi.borderlessMaximizedWindow->checkState() == Qt::Checked;
    config().writeEntry("AlwaysUseDialog", m_alwaysUseDialog);
    config().writeEntry("ShowMaximize", m_showMaximize);
    config().writeEntry("ShowMinimize", m_showMinimize);
    config().writeEntry("ShowWindowTitle", m_showTitle);
    config().writeEntry("ShowIcon", m_showIcon);
    config().writeEntry("ShowClose", m_showClose);
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
        KWindowInfo info = KWindowSystem::windowInfo(window, NET::WMWindowType | NET::WMPid | NET::WMState);
        if (!(info.state() & NET::SkipTaskbar) &&
            info.windowType(NET::NormalMask | NET::DialogMask |
                            NET::OverrideMask | NET::UtilityMask) != NET::Utility &&
            info.windowType(NET::NormalMask | NET::DialogMask |
                            NET::OverrideMask | NET::UtilityMask | NET::DockMask) != NET::Dock) {
            ++count;
        }
    }
    return count;
}

void YetAnotherWindowControl::windowItemClicked()
{
    if (sender() && m_windowIcons.contains(static_cast<Plasma::IconWidget*>(sender()))) {
        KWindowSystem::forceActiveWindow(m_windowIcons.value(static_cast<Plasma::IconWidget*>(sender())));
    }
}

void YetAnotherWindowControl::closePopup()
{
    m_listDialog->deleteLater();
    m_listWidget->deleteLater();
    m_listDialog = 0;
    m_listWidget = 0;
}

bool YetAnotherWindowControl::eventFilter(QObject *watched, QEvent *event)
{
    Plasma::IconWidget *icon = qobject_cast<Plasma::IconWidget *>(watched);

    if (watched == m_listDialog && event->type() == QEvent::WindowDeactivate) {
        closePopup();
    } else if (icon && event->type() == QEvent::GraphicsSceneHoverEnter) {
        m_itemBackground->show();
        m_itemBackground->setTargetItem(icon);
    } else if (watched == m_listWidget && event->type() == QEvent::GraphicsSceneHoverLeave) {
        m_itemBackground->hide();
    }

    return false;
}

#include "yawc.moc"