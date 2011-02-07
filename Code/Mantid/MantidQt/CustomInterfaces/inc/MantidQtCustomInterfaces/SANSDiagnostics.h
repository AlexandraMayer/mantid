#ifndef MANTIDQTCUSTOMINTERFACES_SANSDIAGNOSTICS_H_
#define MANTIDQTCUSTOMINTERFACES_SANSDIAGNOSTICS_H_

#include "ui_SANSRunWindow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  /** 
  The RectDetectorDetails class stores the rectangualr detector name 
  and minimum and maximum detector id
  
  @author Sofia Antony, Rutherford Appleton Laboratory
  @date03/02/2011


  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  */
  class RectDetectorDetails
  {
  public:
    /// constructor
    RectDetectorDetails(){}
    ///destructor
    ~RectDetectorDetails(){}
    /// set minimum detector id
    void setMinimumDetcetorId(const int& minDetId){m_minDetId=minDetId;}

    /// set maximum detector id
    void setMaximumDetcetorId(const int& minDetId){m_maxDetId=minDetId;}
    /// set detector name
    void setDetcetorName(const QString & detName){m_detName=detName;}

    /// get minimum detector id
    const int&  getMinimumDetcetorId(){return m_minDetId;}
    /// get maximum detector id
    const int& getMaximumDetcetorId(){return m_maxDetId;}
    /// get detector name
    const QString& getDetcetorName(){return m_detName;}
  private:
      /// minimum detecor id
      int m_minDetId;
      /// maximum detecor id
      int m_maxDetId;
      /// detector name
      QString m_detName;
  };
     
/** 
    The SANSDiagnostics is responsible for the diagnostics tab of SANS interface.
    @author Sofia Antony, Rutherford Appleton Laboratory
    @date27/01/2011
    
    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
*/
class SANSDiagnostics : public MantidQt::API::UserSubWindow
{
  Q_OBJECT
public:
  /// Default Constructor
  SANSDiagnostics(QWidget* parent, Ui::SANSRunWindow* ParWidgets);
  /// Destructor
  virtual ~SANSDiagnostics();

private:
  
 /// Initilaise the current tab
  void initLayout();
  ///set tool tips
  void setToolTips();
  ///execute sumrowcolumn algorithm
  void executeSumRowColumn(const std::vector<unsigned int>& values,
                           const QString ipws,const QString& op,const QString& orientation);
  /// load the settings from the registry
  void loadSettings();
  ///save settings
  void saveSettings();
  /// returns the total number of periods
  int  getTotalNumberofPeriods(); 

  ///returns true if the user string contains sequential data
  bool isSequentialValues(const std::vector<unsigned int>& values);
  /// returns the workspace name
  QString getWorkspaceName(const QString& fileName);
  /// returns the filename
  QString getFileName();
  /// run loadraw algorithm
  void runLoadAlgorithm(const QString& fileName,const QString& specMin,const QString& specMax);
  /// returns sumrowcolumn script
  QString sumRowColumnScript(const QString ipwsName,const QString& opwsName,const QString& orientation,
                             const QString& hvMin,const QString& hvMax);
  //get rectangular detector details
  std::vector<boost::shared_ptr<RectDetectorDetails> >  rectangularDetectorDetails(Mantid::API::Workspace_sptr& ws_sptr);
  /// returns sumspectra script
  QString sumSpectraScript(const QString& opwsName);
  /// display total number of periods box
  void displayTotalPeriods();
  /// display rectangualr detectors
  void displayRectangularDetectors(const QString & wsName);

  // This method executes loadraw and sumrow column algorithm
  void IntegralClicked(const QString& range,const QString& orientation,
                                          const QString& specMin,const QString& specMax,const QString& opws);
  /// plot spectrum
  void plotSpectrum(const QString& wsName,int specNum);

  /// hide the group boxes
  void disableDetectorGroupBoxes(bool bStatus);
   
  /// minimum and maximum spectrum ids for detector 
  void minandMaxSpectrumIds(const int detNum,QString& minSpec, QString& maxSpec);

  /// get detector name
  QString getDetectorName(int index);

  /// get the user entered period number
  int getPeriodNumber();
  /// get the member workspace name for the period
  QString getMemberWorkspace(int period);
  
  /// returns true if the loaded workspace is multiperiod(group workspace)
  bool isMultiPeriod();
  ///This method returns name of the   workspace which is to be
  /// used as the i/p  for sumrowcolumn or sumspectra algorithm 
  QString getWorkspaceToProcess();

  ///returns true if the spec min and max are in the valid range
  bool isValidSpectra(const QString& specMin,const QString& specMax);

private:
  QString m_dataDir; ///< default data search directory
  QString m_settingsGroup;///< settings used for permanent store.
  QString m_wsName;///<  workspace name created by load raw
  QString m_fileName;///< name of the file
  QString m_spec_min;///<spectrum min
  QString m_spec_max;///<spectrum max
  QString m_outws_load;///<output workspace for load algorithm 
  QString m_memberwsName;
  ///set to point to the object that has the Add Files controls
  Ui::SANSRunWindow *m_SANSForm;
  //set to a pointer to the parent form
  QWidget *parForm;
  
  int m_totalPeriods;///<total periods
  std::vector<std::string> m_wsVec;///< workspace vector
  std::vector<boost::shared_ptr<RectDetectorDetails> > m_rectDetectors;
  Mantid::Kernel::Logger& g_log; ///< reference to logger class.

  int m_Period;
private slots:

  /// first detector horizontal integral button clicked.
  void firstDetectorHorizontalIntegralClicked();
  /// first detector vertical integral button clicked.
  void firstDetectorVerticalIntegralClicked();
  /// first detector Time Integral button clicked.
  void firstDetectorTimeIntegralClicked();
  /// second  detector horizontal integral button clicked.
  void secondDetectorHorizontalIntegralClicked();
  /// second detector vertical integral button clicked.
  void secondDetectorVerticalIntegralClicked();
  /// second detector Time Integral button clicked.
  void secondDetectorTimeIntegralClicked();

  /// display the detector banks for the selected period
  void displayDetectorBanksofMemberWorkspace();
  
  /// load the first spectrum using the user given file.
  void loadFirstSpectrum();
  
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
