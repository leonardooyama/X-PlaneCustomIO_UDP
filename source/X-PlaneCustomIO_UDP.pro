TEMPLATE = lib

QT += core network

#CONFIG += warn_on plugin release
#CONFIG -= thread exceptions qt rtti debug

CONFIG += warn_off plugin
CONFIG -= thread exceptions rtti gui


INCLUDEPATH += ../XPLMSDK/CHeaders/XPLM
INCLUDEPATH += ../XPLMSDK/CHeaders/Wrappers
INCLUDEPATH += ../XPLMSDK/CHeaders/Widgets


DEFINES += XPLM200
DEFINES += XPLM210
DEFINES += XPLM300

win32 {
    DEFINES += APL=0 IBM=1 LIN=0
    LIBS += -L../XPLMSDK/Libraries/Win
    LIBS += -lXPLM_64 -lXPWidgets_64
    TARGET = CustomIO_UDP
    TARGET_EXT = .xpl
    TARGET_CUSTOM_EXT = .xpl
    DESTDIR = "C:/X-Plane 11/Resources/plugins/CustomIO_UDP/win_x64"
}


SOURCES += \
    plugin.cpp

win32 {
    DEPLOY_COMMAND = $$[QT_INSTALL_BINS]/windeployqt
}
macx {
    DEPLOY_COMMAND = macdeployqt
}

CONFIG( debug, debug|release ) {
    # debug
    #DEPLOY_FOLDER = $$shell_quote($$shell_path($${OUT_PWD}/debug))
    #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/debug/$${TARGET}$${TARGET_CUSTOM_EXT}))
    DEPLOY_TARGET = $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
} else {
    # release
    #DEPLOY_FOLDER = $$shell_quote($$shell_path($${OUT_PWD}/release))
    #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}))
    DEPLOY_TARGET = $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
}

QMAKE_POST_LINK = $${DEPLOY_COMMAND} $${DEPLOY_TARGET} --no-translations $$escape_expand(\n\t)

