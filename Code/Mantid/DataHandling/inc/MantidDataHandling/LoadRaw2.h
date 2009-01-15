#ifndef MANTID_DATAHANDLING_LOADRAW2_H_
#define MANTID_DATAHANDLING_LOADRAW2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
//#include "isisraw2.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ISISRAW2;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadRaw LoadRaw.h DataHandling/LoadRaw.h

    Loads an file in ISIS RAW format and stores it in a 2D workspace
    (Workspace2D class). LoadRaw is an algorithm and as such inherits
    from the Algorithm class and overrides the init() & exec() methods.

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

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport LoadRaw2 : public API::Algorithm//, public ISISRAW2
    {
    public:
      /// Default constructor
      LoadRaw2();
      /// Destructor
      ~LoadRaw2();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadRaw"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 2; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      void checkOptionalProperties();
      void loadData(const DataObjects::Histogram1D::RCtype::ptr_type&,int, int&, const int& , int*, DataObjects::Workspace2D_sptr );
      void runLoadInstrument(DataObjects::Workspace2D_sptr);
      void runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr);
      void runLoadMappingTable(DataObjects::Workspace2D_sptr);
      void runLoadLog(DataObjects::Workspace2D_sptr);
      void populateInstrumentParameters(DataObjects::Workspace2D_sptr);

      /// ISISRAW class instance which does raw file reading
      ISISRAW2 *isisRaw;
      /// The name and path of the input file
      std::string m_filename;

      /// The number of spectra in the raw file
      int m_numberOfSpectra;
      /// The number of periods in the raw file
      int m_numberOfPeriods;
      /// Has the spectrum_list property been set?
      bool m_list;
      /// Have the spectrum_min/max properties been set?
      bool m_interval;
      /// The value of the spectrum_list property
      std::vector<int> m_spec_list;
      /// The value of the spectrum_min property
      int m_spec_min;
      /// The value of the spectrum_max property
      int m_spec_max;
      /// Allowed values for the cache property
      std::vector<std::string> m_cache_options;

      ///static reference to the logger class
      static Kernel::Logger& g_log;

      /// Personal wrapper for sqrt to allow msvs to compile
      static double dblSqrt(double in);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADRAW2_H_*/
