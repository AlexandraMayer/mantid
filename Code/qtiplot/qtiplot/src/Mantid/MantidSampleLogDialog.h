#ifndef MANTIDSAMPLELOGDIALOG_H_
#define MANTIDSAMPLELOGDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include <QDialog>
#include <QPoint>

//----------------------------------
// Forward declarations
//----------------------------------
class QTreeWidgetItem;
class QTreeWidget;
class QPushButton;
class QRadioButton;
class MantidUI;

/** 
    This class displays a list of log files for a selected workspace. It
    allows the user to plot selected log files.

    @author Martyn Gigg, Tessella Support Services plc
    @date 05/11/2009

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
class MantidSampleLogDialog : public QDialog
{
  Q_OBJECT
  
public:
  //Constructor
  MantidSampleLogDialog(const QString & wsname, MantidUI* mui, Qt::WFlags flags = 0);

private slots:
  //Plot log files
  void importSelectedFiles();

  //Context menu popup
  void popupMenu(const QPoint & pos);

  //Import a single item
  void importItem(QTreeWidgetItem *item);

private:
  //Initialize the layout
  void init();
  
  //A tree widget
  QTreeWidget *m_tree;

  //The workspace name
  QString m_wsname;

  //Buttons to do things  
  QPushButton *buttonPlot, *buttonClose;

  // Filter radio buttons
  QRadioButton *filterNone, *filterStatus, *filterPeriod, *filterStatusPeriod;
  
  //A pointer to the MantidUI object
  MantidUI* m_mantidUI;
};

#endif //MANTIDSAMPLELOGDIALOG_H_
