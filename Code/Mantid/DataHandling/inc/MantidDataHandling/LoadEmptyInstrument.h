#ifndef MANTID_DATAHANDLING_LOADEMPTYINSTRUMENT_H_
#define MANTID_DATAHANDLING_LOADEMPTYINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"


namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadEmptyInstrument LoadEmptyInstrument.h DataHandling/LoadEmptyInstrument.h

    Loads an instrument definition file into a workspace, with purpose of being
    able to visualise an instrument directly and without requiring to read in a 
    ISIS raw datafile first. The name of the algorithm refers to the fact that an instrument
    is loaded into a workspace but without any real data - hence the reason for referring to
    it as an 'empty' instrument.

    Required Properties:
    <UL>
    <LI> Filename - The name of an instrument definition file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported instrument</LI>
    </UL>

    Optional Properties: (note that these options are not available if reading a multiperiod file)
    <UL>
    <LI> detector_value  - This value may affect the colour of the detector shapes when visualising the instrument</LI>
    <LI> monitor_value  - This value may affect the colour of the monitor shapes when visualising the instrument</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 31/10/2008

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
    class DLLExport LoadEmptyInstrument : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadEmptyInstrument();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadEmptyInstrument"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      void runLoadInstrument(DataObjects::Workspace2D_sptr);

      /// The name and path of the input file
      std::string m_filename;

      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADEMPTYINSTRUMENT_H_*/
