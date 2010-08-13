# --------------------------------
# The CustomDialogs library -
# MantidQtCustomDialogs
# --------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)
CONFIG += qt
QT += opengl


!exists(\"$$MANTIDQTINCLUDES/MantidQtCustomDialogs\") {
  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtCustomDialogs\")
}

# Need to link with the API
unix:LIBS += -L$$TOPBUILDDIR/lib \
    -lMantidQtAPI
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib"

# ------------------------
# Source fies
# ------------------------
HEADERDIR = inc
SRCDIR = src
SOURCES = $$SRCDIR/LoadRawDialog.cpp \
    $$SRCDIR/LOQScriptInputDialog.cpp \
    $$SRCDIR/CreateSampleShapeDialog.cpp \
    $$SRCDIR/SampleShapeHelpers.cpp \
    $$SRCDIR/MantidGLWidget.cpp \
    $$SRCDIR/PlotAsymmetryByLogValueDialog.cpp \
    $$SRCDIR/LoadDAEDialog.cpp \
    $$SRCDIR/LoadAsciiDialog.cpp \
    $$SRCDIR/LoginDialog.cpp
    
HEADERS = $$HEADERDIR/LoadRawDialog.h \
    $$HEADERDIR/LOQScriptInputDialog.h \
    $$HEADERDIR/CreateSampleShapeDialog.h \
    $$HEADERDIR/SampleShapeHelpers.h \
    $$HEADERDIR/MantidGLWidget.h \
    $$HEADERDIR/PlotAsymmetryByLogValueDialog.h \
    $$HEADERDIR/LoadDAEDialog.h \
    $$HEADERDIR/LoadAsciiDialog.h \
    $$HEADERDIR/LoginDialog.h
    
    
UI_DIR = $$HEADERDIR

FORMS = $$HEADERDIR/LOQScriptInputDialog.ui \
    $$HEADERDIR/CreateSampleShapeDialog.ui \
    $$HEADERDIR/PlotAsymmetryByLogValueDialog.ui \
    $$HEADERDIR/LoginDialog.ui
    
UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtCustomDialogs"

TARGET = MantidQtCustomDialogs
unix:headercopy.commands = cd \
    $$HEADERDIR \
    && \
    $(COPY) \
    *.h \
    '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"'
win32:headercopy.commands = cd \
    $$HEADERDIR \
    && \
    $(COPY) \
    *.h \
    '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"'
PRE_TARGETDEPS = headercopy
QMAKE_EXTRA_TARGETS += headercopy
