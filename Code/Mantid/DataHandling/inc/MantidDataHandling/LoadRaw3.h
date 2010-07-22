#ifndef MANTID_DATAHANDLING_LoadRaw3_H_
#define MANTID_DATAHANDLING_LoadRaw3_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include <climits>

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadRaw3 LoadRaw3.h DataHandling/LoadRaw3.h

    Loads an file in ISIS RAW format and stores it in a 2D workspace
    (Workspace2D class). LoadRaw is an algorithm and LoadRawHelper class and
	overrides the init() & exec() methods.
    LoadRaw3 uses less memory by only loading up the datablocks as required.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data
         (a multiperiod file will store higher periods in workspaces called OutputWorkspace_PeriodNo)</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> spectrum_min  - The spectrum to start loading from</LI>
    <LI> spectrum_max  - The spectrum to load to</LI>
    <LI> spectrum_list - An ArrayProperty of spectra to load</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadRaw3 : public LoadRawHelper
    {
	
    public:
      /// Default constructor
      LoadRaw3();
      /// Destructor
      ~LoadRaw3();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRaw"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 3; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
	        
      /// returns true if the given spectrum is a monitor
      bool isMonitor(const std::vector<int>& monitorIndexes,int spectrumNum);
      /// returns true if the Exclude Monitor option(property) selected
      bool isExcludeMonitors();
      ///  returns true if the Separate Monitor Option  selected
      bool isSeparateMonitors();
      ///  returns true if the Include Monitor Option  selected
      bool isIncludeMonitors();

      /// validate workspace sizes
      void validateWorkspaceSizes( bool bexcludeMonitors ,bool bseparateMonitors,
                                   const int normalwsSpecs,const int  monitorwsSpecs);

      /// this method will be executed if not enough memory.
      void goManagedRaw(bool bincludeMonitors,bool bexcludeMonitors,
                        bool bseparateMonitors,const std::string& fileName);
      

      /// This method is useful for separating  or excluding   monitors from the output workspace
      void  separateOrexcludeMonitors(DataObjects::Workspace2D_sptr localWorkspace,
                                      bool binclude,bool bexclude,bool bseparate,
                                      int numberOfSpectra,const std::string &fileName);

      /// creates output workspace, monitors excluded from this workspace
      void excludeMonitors(FILE* file,const int& period,const std::vector<int>& monitorList,
                           DataObjects::Workspace2D_sptr ws_sptr);

      /// creates output workspace whcih includes monitors
      void includeMonitors(FILE* file,const int& period,DataObjects::Workspace2D_sptr ws_sptr);

      /// creates two output workspaces none normal workspace and separate one for monitors
      void separateMonitors(FILE* file,const int& period,const std::vector<int>& monitorList,
                            DataObjects::Workspace2D_sptr ws_sptr,DataObjects::Workspace2D_sptr mws_sptr);

      ///sets optional properties
      void setOptionalProperties();
      
      /// The name and path of the input file
      std::string m_filename;

      /// The number of spectra in the raw file
      int m_numberOfSpectra;
      /// The number of periods in the raw file
      int m_numberOfPeriods;
      /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;
      /// A map for storing the time regime for each spectrum
      std::map<int,int> m_specTimeRegimes;
      /// number of time regime
      int m_noTimeRegimes;
      /// The current value of the progress counter
      double m_prog;

      /// A flag int value to indicate that the value wasn't set by users
      static const int unSetInt = INT_MAX-15;
      /// a vector holding the indexes of monitors
      std::vector<int> m_monitordetectorList;
      /// Read in the time bin boundaries
      int m_lengthIn;
      /// boolean for list spectra options
      bool m_bmspeclist;
      /// TimeSeriesProperty<int> containing data periods.
      boost::shared_ptr<Kernel::Property> m_perioids;
	  /// time channels vector
	  std::vector<boost::shared_ptr<MantidVec> > m_timeChannelsVec;
	  /// total number of specs
	  int m_total_specs;
	   

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadRaw3_H_*/
