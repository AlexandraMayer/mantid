#ifndef MANTID_NEXUS_LOADINSTRUMENTFROMNEXUS_H_
#define MANTID_NEXUS_LOADINSTRUMENTFROMNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
	
namespace Geometry
{
	class CompAssembly;
	class Component;
}
namespace API
{
	class Instrument;
}
	
  namespace Nexus
  {
    /** @class LoadInstrumentFromNexus LoadInstrumentFromNexus.h Nexus/LoadInstrumentFromNexus.h

    Attempt to load information about the instrument from a ISIS nexus file. In particular attempt to 
    read L2 and 2-theta detector position values and add detectors which are positioned relative
    to the sample in spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also adds dummy source
    and samplepos components to instrument.
    As this information appears to be absent from the existing nexus Muon sample files, it seems that 
    little to this can be done at present. The new version of Muon nexus files may be more useful.

    LoadInstrumentFromNexus is intended to be used as a child algorithm of 
    other Loadxxx algorithms, rather than being used directly.
    LoadInstrumentFromNexus is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec()  methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input NEXUS file </LI>
    <LI> Workspace - The name of the workspace in which to use as a basis for any data to be added.</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL (LoadInstrumentFromRaw)
    @date 2/5/2008
    @author Ronald Fowler, ISIS, RAL (LoadInstrumentFromNexus)

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */
    class DLLExport LoadInstrumentFromNexus : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadInstrumentFromNexus();

      /// Destructor
      ~LoadInstrumentFromNexus() {}

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadInstrumentFromNexus";};

      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Nexus\\Instrument";}

    private:

      /// Overwrites Algorithm method. Does nothing at present
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;

      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace Nexus
} // namespace Mantid

#endif /*MANTID_NEXUS_LOADINSTRUMENTFROMNEXUS_H_*/

