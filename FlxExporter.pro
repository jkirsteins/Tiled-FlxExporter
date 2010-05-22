TEMPLATE = lib
CONFIG += plugin
INCLUDEPATH = ../..
TARGET = $$qtLibraryTarget(flx)
DESTDIR = ../../../lib/tiled/plugins
DEFINES += FLX_LIBRARY
OBJECTS += ../../.obj/map.o
SOURCES += flxexporter.cpp \
    settingsdialog.cpp \
    as3level.cpp \
    progressdialog.cpp
HEADERS += flxexporter.h \
    settingsdialog.h \
    as3level.h \
    as3levelplaceholders.h \
    progressdialog.h
RESOURCES += ASTemplates.qrc
FORMS += settingsdialog.ui \
    progressdialog.ui
