#--------------------------------
# MantidWidgets library
#--------------------------------
TEMPLATE = lib

# Import the config file
include(../mantidqt.pri)

!exists(\"$$MANTIDQTINCLUDES/MantidQtMantidWidgets\") {
  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtMantidWidgets\")
}

# Need to link with the API
unix:LIBS += -lMantidQtAPI
win32:LIBS += "$$DESTDIR/MantidQtAPI.lib"

DEFINES += IN_MANTIDQT_MANTIDWIDGETS

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/MantidWidget.cpp \
  $$SRCDIR/MWRunFiles.cpp \
  $$SRCDIR/pythonCalc.cpp \
  $$SRCDIR/DiagResults.cpp \
  $$SRCDIR/MWDiag.cpp \
  $$SRCDIR/MWDiagCalcs.cpp \
  $$SRCDIR/SaveWorkspaces.cpp \
  $$SRCDIR/UserFunctionDialog.cpp \
  $$SRCDIR/RenameParDialog.cpp \
  $$SRCDIR/ICatSearch.cpp  \
  $$SRCDIR/ICatInvestigation.cpp \
  $$SRCDIR/ICatMyDataSearch.cpp \
  $$SRCDIR/InstrumentSelector.cpp  \
  $$SRCDIR/ICatUtils.cpp  \
  $$SRCDIR/ICatAdvancedSearch.cpp
  
 
HEADERS = \
  $$HEADERDIR/MantidWidget.h \
  $$HEADERDIR/MWRunFiles.h \
  $$HEADERDIR/pythonCalc.h \
  $$HEADERDIR/WidgetDllOption.h \
  $$HEADERDIR/DiagResults.h \
  $$HEADERDIR/MWDiag.h \
  $$HEADERDIR/MWDiagCalcs.h \
  $$HEADERDIR/SaveWorkspaces.h \
  $$HEADERDIR/UserFunctionDialog.h \
  $$HEADERDIR/RenameParDialog.h \
  $$HEADERDIR/ICatSearch.h  \
  $$HEADERDIR/ICatInvestigation.h \
  $$HEADERDIR/ICatMyDataSearch.h \
  $$HEADERDIR/InstrumentSelector.h   \
  $$HEADERDIR/ICatUtils.h  \
  $$HEADERDIR/ICatAdvancedSearch.h
  
UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/MWRunFiles.ui \
  $$HEADERDIR/MWDiag.ui \
  $$HEADERDIR/UserFunctionDialog.ui \
  $$HEADERDIR/RenameParDialog.ui \
  $$HEADERDIR/ICatSearch.ui \
  $$HEADERDIR/ICatInvestigation.ui \
  $$HEADERDIR/ICatMyDataSearch.ui  \
  $$HEADERDIR/ICatAdvancedSearch.ui

UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtMantidWidgets"
  
#-----------------------------
# Target and dependancies
#-----------------------------

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtMantidWidgets"'
win32:headercopy.commands = cd "$$HEADERDIR" && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtMantidWidgets"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy

