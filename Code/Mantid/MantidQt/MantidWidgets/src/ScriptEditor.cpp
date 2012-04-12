//---------------------------------------------
// Includes
//-----------------------------------------------
#include "MantidQtMantidWidgets/ScriptEditor.h"
#include "MantidQtMantidWidgets/FindReplaceDialog.h"

// Qt
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>

#include <QTextStream>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QPrintDialog>
#include <QPrinter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QShortcut>
#include <QSettings>

// Qscintilla
#include <Qsci/qscilexer.h> 
#include <Qsci/qsciapis.h>
#include <Qsci/qscilexerpython.h> 


// std
#include <cmath>
#include <iostream>

//***************************************************************************
//
// CommandHistory struct
//
//***************************************************************************
/**
 * Add a block of lines to the store
 * @param block
 */
void CommandHistory::addCode(QString block)
{
  QStringList lines = block.split("\n");
  QStringListIterator iter(lines);
  while(iter.hasNext())
  {
    this->add(iter.next());
  }
}

/**
 * Add a command to the store. 
 */
void CommandHistory::add(QString cmd)
{
  //Ignore duplicates that are the same as the last command entered and just reset the pointer
  int ncmds = m_commands.count();
  if( ncmds > 1 && m_commands.lastIndexOf(cmd) == ncmds - 2 ) 
  {
    //Reset the index pointer
    m_current = ncmds - 1;
    return;
  }

  // If the stack is full, then remove the oldest
  if( ncmds == m_hist_maxsize + 1)
  {
    m_commands.removeFirst();
  }
  if( !m_commands.isEmpty() )
  {
    //Remove blankline
    m_commands.removeLast();
  }
  // Add the command and an extra blank line
  m_commands.append(cmd);
  m_commands.append("");
  
  //Reset the index pointer
  m_current = m_commands.count() - 1;
}

/** 
 * Is there a previous command
 * @returns A boolean indicating whether there is command on the left of the current index
 */
bool CommandHistory::hasPrevious() const
{
  if( !m_commands.isEmpty() &&  m_current > 0 ) return true;
  else return false;
}

/**
 * Get the item pointed to by the current index and move it back one
 */
QString CommandHistory::getPrevious() const
{
  return m_commands.value(--m_current);
}

/** 
 * Is there a command next on the stack
 */
bool CommandHistory::hasNext() const
{
  if( !m_commands.isEmpty() &&  m_current < m_commands.count() - 1 ) return true;
  else return false;
}

/** 
 * Get the item pointed to by the current index and move it down one
 */
QString CommandHistory::getNext() const
{
  return m_commands.value(++m_current);
}

//***************************************************************************
//
// ScriptEditor class
//
//***************************************************************************
// The colour for a success marker
QColor ScriptEditor::g_success_colour = QColor("lightgreen");
// The colour for an error marker
QColor ScriptEditor::g_error_colour = QColor("red");

//------------------------------------------------
// Public member functions
//------------------------------------------------
/**
 * Constructor
 * @param parent :: The parent widget (can be NULL)
 */
ScriptEditor::ScriptEditor(QWidget *parent, QsciLexer *codelexer) :
  QsciScintilla(parent), m_filename(""), m_progressArrowKey(markerDefine(QsciScintilla::RightArrow)),
  m_completer(NULL),m_previousKey(0),  m_zoomLevel(0),
  m_findDialog(new FindReplaceDialog(this))
{
  //Syntax highlighting and code completion
  setLexer(codelexer);
  readSettings();

#ifdef __APPLE__
  // Make all fonts 4 points bigger on the Mac because otherwise they're tiny!
  if( m_zoomLevel == 0 ) m_zoomLevel = 4;
#endif

  zoomIn(m_zoomLevel);
  setMarginLineNumbers(1,true);

  //Editor properties
  setAutoIndent(true);
  setFocusPolicy(Qt::StrongFocus);

  emit undoAvailable(isUndoAvailable());
  emit redoAvailable(isRedoAvailable());
}

/**
 * Destructor
 */
ScriptEditor::~ScriptEditor()
{
  writeSettings();
  if( m_completer )
  {
    delete m_completer;
  }
  if( QsciLexer * current = lexer() )
  {
    delete current;
  }
}

/**
 * Set a new code lexer for this object. Note that this clears all auto complete information
 */
void ScriptEditor::setLexer(QsciLexer *codelexer)
{
  if( !codelexer )
  {
    if( m_completer )
    {
      delete m_completer;
      m_completer = NULL;
    }
    return;
  }

  //Delete the current lexer if one is installed
  if( QsciLexer * current = lexer() )
  {
    delete current;
  }
  this->QsciScintilla::setLexer(codelexer);

  if( m_completer )
  {
    delete m_completer;
    m_completer = NULL;
  }
  
  m_completer = new QsciAPIs(codelexer);
}

/**
 * Make the object resize to margin to fit the contents with padding
 */
void ScriptEditor::setAutoMarginResize()
{
  connect(this, SIGNAL(linesChanged()), this, SLOT(padMargin()));
}

/**
 * Enable the auto complete
 */
void ScriptEditor::enableAutoCompletion()
{
  setAutoCompletionSource(QsciScintilla::AcsAPIs);
  setCallTipsVisible(QsciScintilla::CallTipsNoAutoCompletionContext);
  setAutoCompletionThreshold(2);
  setCallTipsVisible(0); // This actually makes all of them visible
}

/**
 * Disable the auto complete
 * */
void ScriptEditor::disableAutoCompletion()
{
  setAutoCompletionSource(QsciScintilla::AcsNone);
  setCallTipsVisible(QsciScintilla::CallTipsNone);
  setAutoCompletionThreshold(-1);
  setCallTipsVisible(-1);
}

/**
 * Default size hint
 */
QSize ScriptEditor::sizeHint() const
{
  return QSize(600, 500);
}

/**
 * Save the script, opening a dialog to ask for the filename
 */
void ScriptEditor::saveAs()
{
  QString selectedFilter;
  QString filter = "Scripts (*.py *.PY);;All Files (*)";
  QString filename = QFileDialog::getSaveFileName(NULL, "MantidPlot - Save", "",filter, &selectedFilter);

  if( filename.isEmpty() ) return;
  if( QFileInfo(filename).suffix().isEmpty() )
  {
    QString ext = selectedFilter.section('(',1).section(' ', 0, 0);
    ext.remove(0,1);
    if( ext != ")" ) filename += ext;
  }
  saveScript(filename);
}

/// Save to the current filename, opening a dialog if blank
void ScriptEditor::saveToCurrentFile()
{
  QString filename = fileName();
  if(filename.isEmpty())
  {
    saveAs();
    return;
  }
  else
  {
    saveScript(filename);
  }
}

/**
 * Save the text to the given filename
 * @param filename :: The filename to use
 */
bool ScriptEditor::saveScript(const QString & filename)
{
  QFile file(filename);
  if( !file.open(QIODevice::WriteOnly) )
  {
    QMessageBox::critical(this, tr("MantidPlot - File error"), 
        tr("Could not open file \"%1\" for writing.").arg(filename));
    return false;
  }

  m_filename = filename;
  writeToDevice(file);
  file.close();
  setModified(false);

  return true;
}

/**
 * Set the text on the given line, something I feel is missing from the QScintilla API. Note
 * that like QScintilla line numbers start from 0
 * @param lineno :: A zero-based index representing the linenumber, 
 * @param text :: The text to insert at the given line
 * @param index :: The position of text in a line number,default value is zero
 */
void ScriptEditor::setText(int lineno, const QString& txt,int index)
{
  int line_length = txt.length();
  // Index is max of the length of current/new text
  setSelection(lineno, index, lineno, qMax(line_length, this->text(lineno).length()));
  removeSelectedText();
  insertAt(txt, lineno, index);
  setCursorPosition(lineno, line_length);
}

/** 
 * Capture key presses. Enter/Return executes the code or asks for more input if necessary.
 * Up/Down search the command history
 * @event A pointer to the QKeyPressEvent object
 */
void ScriptEditor::keyPressEvent(QKeyEvent* event)
{
  // Avoids a bug in QScintilla
  forwardKeyPressToBase(event);
}


/** Ctrl + Rotating the mouse wheel will increase/decrease the font size
 *
 */
void ScriptEditor::wheelEvent( QWheelEvent * e )
{
  if ( e->state() == Qt::ControlButton )
  {
    if ( e->delta() > 0 )
    {
      zoomIn();
    }
    else
    {
      zoomOut();
    }
  }
  else
  {
    QsciScintilla::wheelEvent(e);
  }
}

//-----------------------------------------------
// Public slots
//-----------------------------------------------
/// Ensure the margin width is big enough to hold everything
void ScriptEditor::padMargin()
{
  const int minWidth = 38;
  int width = minWidth;
  int ntens = static_cast<int>(std::log10(static_cast<double>(lines())));
  if( ntens > 1 )
  {
    width += 5*ntens;
  }
  setMarginWidth(1, width);
}

/**
 * Set the marker state
 * @param enable :: If true then the progress arrow is enabled
 */
void ScriptEditor::setMarkerState(bool enabled)
{
  if( enabled )
  {
    setMarkerBackgroundColor(QColor("gray"), m_progressArrowKey);
    markerAdd(0, m_progressArrowKey);
  }
  else
  {
    markerDeleteAll(); 
  }
}

/**
 * Update the arrow marker to point to the correct line and colour it depending on the error state
 * @param lineno :: The line to place the marker at. A negative number will clear all markers
 * @param error :: If true, the marker will turn red
 */
void ScriptEditor::updateProgressMarker(int lineno, bool error)
{
  if(error)
  {
    setMarkerBackgroundColor(g_error_colour, m_progressArrowKey);
  }
  else
  {
    setMarkerBackgroundColor(g_success_colour, m_progressArrowKey);
  }
  markerDeleteAll();
  // Check the lineno actually exists, -1 means delete
  if( lineno < 0 || lineno > this->lines() ) return;

  ensureLineVisible(lineno);
  markerAdd(lineno - 1, m_progressArrowKey);
}

/**
 * Update the completion API with a new list of keywords. Note that the old is cleared
 */
void ScriptEditor::updateCompletionAPI(const QStringList & keywords)
{
  if( !m_completer ) return;
  QStringListIterator iter(keywords);
  m_completer->clear();
  while( iter.hasNext() )
  {
    m_completer->add(iter.next());
  }
  m_completer->prepare();
}

/**
 * Print the current text
 */
void ScriptEditor::print()
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog *print_dlg = new QPrintDialog(&printer, this);
  print_dlg->setWindowTitle(tr("Print Script"));
  if(print_dlg->exec() != QDialog::Accepted)
  {
    return;
  }
  QTextDocument document(text());
  document.print(&printer);
}

/**
 * Raises the find replace dialog
 */
void ScriptEditor::showFindReplaceDialog()
{
  m_findDialog->show();
}

/**
 * Override the zoomIn slot to keep a count of the level
 */
void ScriptEditor::zoomIn()
{
  ++m_zoomLevel;
  QsciScintilla::zoomIn();
}

/**
 * Override the zoomIn slot to keep a count of the level
 * @param level Increase font size by this many points
 */
void ScriptEditor::zoomIn(int level)
{
  m_zoomLevel = level;
  QsciScintilla::zoomIn(level);
}

/**
 * Override the zoomIn slot to keep a count of the level
 */

void ScriptEditor::zoomOut()
{
  --m_zoomLevel;
  QsciScintilla::zoomOut();
}
/**
 * Override the zoomIn slot to keep a count of the level
 * @param level Decrease font size by this many points
 */
void ScriptEditor::zoomOut(int level)
{
  m_zoomLevel = -level;
  QsciScintilla::zoomOut(level);
}

/**
 * Write to the given device
 */
void ScriptEditor::writeToDevice(QIODevice & device) const
{
  this->write(&device);
}

//------------------------------------------------
// Private member functions
//------------------------------------------------

/// Settings group
/**
 * Returns a string containing the settings group to use
 * @return A QString containing the group to use within the QSettings class
 */
QString ScriptEditor::settingsGroup() const
{
  return "/ScriptWindow";
}

/**
 * Read settings saved to persistent store
 */
void ScriptEditor::readSettings()
{
  QSettings settings;
  settings.beginGroup(settingsGroup());
  m_zoomLevel = settings.readNumEntry("ZoomLevel", 0);
  settings.endGroup();
}

/**
 * Read settings saved to persistent store
 */
void ScriptEditor::writeSettings()
{
  QSettings settings;
  settings.beginGroup(settingsGroup());
  settings.setValue("ZoomLevel", m_zoomLevel);
  settings.endGroup();
}

/**
 * Forward the QKeyEvent to the QsciScintilla base class. 
 * Under Gnome on Linux with Qscintilla versions < 2.4.2 there is a bug with the autocomplete
 * box that means the editor loses focus as soon as it the box appears. This functions
 * forwards the call and sets the correct flags on the resulting window so that this does not occur
 */
void ScriptEditor::forwardKeyPressToBase(QKeyEvent *event)
{
  // Hack to get around a bug in QScitilla
  //If you pressed ( after typing in a autocomplete command the calltip does not appear, you have to delete the ( and type it again
  //This does that for you!
  if (event->text()=="(")
  {
    QKeyEvent * backspEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QKeyEvent * bracketEvent = new QKeyEvent(*event);
    QsciScintilla::keyPressEvent(bracketEvent);
    QsciScintilla::keyPressEvent(backspEvent);

    delete backspEvent;
    delete bracketEvent;
  }
  

  QsciScintilla::keyPressEvent(event);
  
  // Only need to do this for Unix and for QScintilla version < 2.4.2. Moreover, only Gnome but I don't think we can detect that
#ifdef Q_OS_LINUX
#if QSCINTILLA_VERSION < 0x020402
  // If an autocomplete box has surfaced, correct the window flags. Unfortunately the only way to 
  // do this is to search through the child objects.
  if( isListActive() )
  {
    QObjectList children = this->children();
    QListIterator<QObject*> itr(children);
    // Search is performed in reverse order as we want the last one created
    itr.toBack();
    while( itr.hasPrevious() )
    {
      QObject *child = itr.previous();
      if( child->inherits("QListWidget") )
      {
        QWidget *w = qobject_cast<QWidget*>(child);
        w->setWindowFlags(Qt::ToolTip|Qt::WindowStaysOnTopHint);
        w->show();
        break;
      }
    }
  }  
#endif
#endif
}
