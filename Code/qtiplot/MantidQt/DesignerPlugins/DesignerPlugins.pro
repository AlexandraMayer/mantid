#--------------------------------------------
# This library is solely for plugins to the
# Qt designer
#--------------------------------------------
TEMPLATE = lib

# Main config file
include(../mantidqt.pri)

# Qt options for compiling plugins
CONFIG += plugin designer

# Give the target a more appropriate name
TARGET = MantidWidgetPlugins

# Add a define so that the code can pick up the library name which is required by the Qt plugin mechanism
DEFINES += LIBRARY_NAME=$$TARGET

# Need to link with the MantidWidgets
unix:LIBS += -lMantidWidgets
win32:LIBS += MantidWidgets.lib

HEADERDIR = inc/MantidQtDesignerPlugins
SRCDIR = src

SOURCES = \
  $$SRCDIR/FileFinderPlugin.cpp
 
HEADERS = \
  $$HEADERDIR/FileFinderPlugin.h

INCLUDEPATH += inc
  
# Qt expects to find the plugins on $QT_PLUGIN_PATH/designer so set destination directory to DESTDIR/designer
# and then each user needs to set QT_PLUGIN_PATH to DESTDIR
DESTDIR=$$DESTDIR/designer
