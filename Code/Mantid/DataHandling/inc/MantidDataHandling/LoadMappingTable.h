#ifndef MANTID_DATAHANDLING_LOADMAPPINGTABLE_H_
#define MANTID_DATAHANDLING_LOADMAPPINGTABLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DataHandlingCommand.h"
#include "MantidKernel/Logger.h"

class Workspace2D;

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadMappingTable LoadMappingTable.h DataHandling/LoadMappingTable.h

    Loads the mapping table between spectra and IDetector
    from a raw file. It returns a SpectraDetectorTable which maintain a multimap.
    The key of the multimap is the spectra number and the value
    is the pointer to IDetector. The association is one to many, i.e. a spectrum can have one or many 
    detectors contributing to it. Alternatively the same spectrum can contributes to different spectra 
    (for example in DAE2 (Data Aquisition Electronic) when a spectra containing electronically focussed data is created simultaneously 
    with indivudual spectra). LoadMappingTable is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input RAW file </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the imported data </LI>
    </UL>

    @author Laurent Chapon, ISIS Rutherford Appleton Laboratory
    @date 25/04/2008

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
    class DLLExport LoadMappingTable : public DataHandlingCommand
    {
    public:
      /// Default constructor
      LoadMappingTable();

      /// Destructor
      ~LoadMappingTable() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMappingTable";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}
    private:
      /// The name and path of the input file
      std::string m_filename;
      /// Shared pointer to a Workspace2D
     boost::shared_ptr<Workspace2D> m_localWorkspace;
      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();  
      
      /// Pointer to the local workspace

      ///static reference to the logger class
      static Kernel::Logger& g_log;

      /// Run the sub-algorithms
      void runSubAlgorithms();
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADRAW_H_*/
