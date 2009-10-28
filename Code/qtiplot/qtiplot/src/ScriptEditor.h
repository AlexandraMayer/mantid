#ifndef SCRIPTEDITOR_H_
#define SCRIPTEDITOR_H_

//----------------------------------
// Includes
//----------------------------------
#include <Qsci/qsciscintilla.h>
#include <QDialog>
#include <QTextDocument>

//----------------------------------
// Forward declarations
//----------------------------------
class QAction;
class QKeyEvent;
class QMouseEvent;

/**
 * A small wrapper around a QStringList to manage a command history
 */
struct CommandHistory
{
  ///Default constructor
  CommandHistory() : m_commands(), m_hist_maxsize(100), m_current(0) {}
  /// Add a command. 
  void add(QString command);
  /// Is there a previous command
  bool hasPrevious() const;
  /// Get the item pointed to by the current index and move it up one
  QString getPrevious() const;
  /// Is there a command next on the stack
  bool hasNext() const;
  /// Get the item pointed to by the current index and move it down one
  QString getNext() const;

private:
  /// Store a list of command strings
  QStringList m_commands;
  /// History size
  int m_hist_maxsize;
  /// Index "pointer"
  mutable int m_current;
};

/** 
    This class provides an area to write scripts. It inherits from QScintilla to use
    functionality such as auto-indent and if supported, syntax highlighting.
        
    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>   
*/
class ScriptEditor : public QsciScintilla
{
  // Qt macro
  Q_OBJECT;

public:
  /// Constructor
  ScriptEditor(QWidget* parent = 0, bool interpreter_mode = false);
  ///Destructor
  ~ScriptEditor();
  // Size hint
  QSize sizeHint() const;
  /// Set the text on a given line number
  void setText(int lineno, const QString& text);
  /// Save a the text to the given filename
  bool saveScript(const QString & filename);
  ///Capture key presses
  void keyPressEvent(QKeyEvent* event);
  /// Set whether or not the current line(where the cursor is located) is editable
  void setEditingState(int line);
  ///Capture mouse clicks to prevent moving the cursor to unwanted places
  void mousePressEvent(QMouseEvent *event);
  /// Create a new input line
  void newInputLine();
  /// The current filename
  inline QString fileName() const
  {
    return m_filename;
  }

  /**
   * Set a new file name
   * @param filename The new filename
   */
  inline void setFileName(const QString & filename)
  {
    m_filename = filename;
  }

  /// Undo action for this editor
  inline QAction* undoAction() const
  {
    return m_undo;
  }
  /// Redo action for this editor
  inline QAction* redoAction() const
  {
    return m_redo;
  }

  /// Cut action for this editor
  inline QAction* cutAction() const
  {
    return m_cut;
  }
  /// Copy action for this editor
  inline QAction* copyAction() const
  {
    return m_copy;
  }
  /// Paste action for this editor
  inline QAction* pasteAction() const
  {
    return m_paste;
  }

  /// Print action for this editor
  inline QAction* printAction() const
  {
    return m_print;
  }  

public slots:
  /// Update the editor
  void update();
  /// Update the marker on this widget
  void updateMarker(int lineno, bool success);
  /// Print the text within the widget
  void print();
  ///Display the output from a script that has been run in interpeter mode
  void displayOutput(const QString& msg, bool error);

signals:
  /// Inform observers that undo information is available
  void undoAvailable(bool);
  /// Inform observers that redo information is available
  void redoAvailable(bool);
  /// Notify manager that there is code to execute (only used in interpreter mode)
  void executeLine(const QString&);

private:
  /// The file name associated with this editor
  QString m_filename;

  //Each editor needs its own undo/redo etc
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste, *m_print;
  /// The margin marker 
  int m_marker_handle;
  /// Flag that we are in interpreter mode
  bool m_interpreter_mode;
  //Store a command history, only used in interpreter mode
  CommandHistory m_history;
  /// Flag whether editing is possible (only used in interpreter mode)
  bool m_read_only;
  // The colour of the marker for a success state
  static QColor g_success_colour;
  // The colour of the marker for an error state
  static QColor g_error_colour;
};






#endif //SCRIPTEDITOR_H_
