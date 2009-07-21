#ifndef MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <climits>

namespace Mantid
{
  namespace NeXus
  {
    /** @class SaveNexusProcessed SaveNexusProcessed.h NeXus/SaveNexusProcessed.h

    Saves a workspace as a Nexus Processed entry in a Nexus file. 
    SaveNexusProcessed is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the output Nexus file (may exist) </LI>
    <LI> InputWorkspace - The name of the workspace to store the file </LI>
    <LI> Title - the title to describe the saved processed data
    </UL>

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
    class DLLExport SaveNexusProcessed : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveNexusProcessed();
      /// Destructor
      ~SaveNexusProcessed() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveNexusProcessed";};
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling";}

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;
      /// The name and path of the input file
      std::string m_entryname;
      /// The title of the processed data section
      std::string m_title;
      /// Pointer to the local workspace
      API::MatrixWorkspace_const_sptr m_inputWorkspace;
      /// Flag set if list of spectra to save is specifed
      bool m_list;
      /// Flag set if interval of spectra to write is set
      bool m_interval;
      /// The value of the spectrum_list property
      std::vector<int> m_spec_list;
      /// The value of the spectrum_min property
      int m_spec_min;
      /// The value of the spectrum_max property
      int m_spec_max;

      ///a flag int value that indicates a value that wasn't set
      static const int unSetInt = INT_MAX-15;

	  /// boolean flag used  to determine over write or append .nxs file
	  bool m_bAppend;


    };

  } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_*/
