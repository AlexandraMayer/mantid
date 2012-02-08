#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSITREESULTTABLETAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <QTableWidget>

namespace Ui
{
  class MuonAnalysis;
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


/** 
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.    

@author Robert Whitley, ISIS, RAL

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class MuonAnalysisResultTableTab : public QWidget
{
 Q_OBJECT
public:
  MuonAnalysisResultTableTab(Ui::MuonAnalysis& uiForm) : m_uiForm(uiForm), m_numLogsdisplayed(0) {}
  void initLayout();
  void populateTables(const QStringList& wsList);

private slots:
  void helpResultsClicked();
  void selectAllLogs(bool);
  void selectAllFittings(bool);
  void createTable();

private:
  void populateLogsAndValues(const QVector<QString>& fittedWsList);
  void populateFittings(const QVector<QString>& fittedWsList);

  bool haveSameParameters(const QVector<QString>& wsList);
  QVector<QString> getSelectedWs();
  QVector<QString> getSelectedLogs();
  std::string getFileName();
  QMap<int,int> getWorkspaceColors(const QVector<QString>& wsList);

  Ui::MuonAnalysis& m_uiForm;
  int m_numLogsdisplayed;
  QMap<QString, QMap<QString, double> > m_tableValues;
};

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISTESULTTABLETAB_H_
