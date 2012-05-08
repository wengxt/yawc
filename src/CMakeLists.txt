include_directories(${KDE4_INCLUDE_DIR})
include_directories(${KACTIVITIES_INCLUDE_DIRS})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

set(yawc_SRCS yawc.cpp titlewidget.cpp)

kde4_add_ui_files(yawc_SRCS yawc.ui)
kde4_add_plugin(plasma_applet_yawc ${yawc_SRCS})
target_link_libraries(plasma_applet_yawc ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${QT_X11_X11_LIBRARY} ${KACTIVITIES_LIBRARY})

install(TARGETS plasma_applet_yawc
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-yawc.desktop
        DESTINATION ${SERVICES_INSTALL_DIR}) 
