#ifndef MANTID_DATAHANDLING_LOADSASSENA_H_
#define MANTID_DATAHANDLING_LOADSASSENA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid
{

  namespace DataHandling
  {

  /** Load Sassena Output files.

  Required Properties:
  <UL>
  <LI> Filename - The name of and path to the Sassena file </LI>
  <LI> Workspace - The name of the group workspace to output
  </UL>

  @author Jose Borreguero
  @date 2012-04-24

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
  */

  /* Base class to Load a sassena dataset into a MatrixWorkspace
   * Derived implementations will load different scattering functions

  class LoadDataSet
  {

  };
  */

  class DLLExport LoadSassena : public API::IDataFileChecker
  {
  public:
    /// Constructor
    LoadSassena(): IDataFileChecker(), m_filename("") {};
    /// Virtual Destructor
    virtual ~LoadSassena() {}
    /// Algorithm's name
    virtual const std::string name() const { return "LoadSassena"; }
    /// Algorithm's version
    virtual int version() const { return 1; }
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling\\Sassena"; }
    /**
     * Do a quick check that this file can be loaded
     *
     * @param filePath the location of and the file to check
     * @param nread number of bytes to read
     * @param header the first 100 bytes of the file as a union
     * @return true if the file can be loaded, otherwise false
     */
    virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    /**
     * Check the structure of the file and return a value between 0 and 100 of
     * how much this file can be loaded
     *
     * @param filePath the location of and the file to check
     * @return a confidence level indicator between 0 and 100
     */
    int fileCheck(const std::string& filePath);

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs(); // Sets documentation strings for this algorithm
    /// Initialization code
    void init();             // Overwrites Algorithm method.
    /// Execution code
    void exec();             // Overwrites Algorithm method

    ///valid datasets
    std::vector<const char*> m_validSets;
    /// name and path of input file
    std::string m_filename;

  }; // class LoadSassena

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSASSENA_H_*/
