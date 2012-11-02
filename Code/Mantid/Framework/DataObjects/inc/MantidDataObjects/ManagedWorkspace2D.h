#ifndef MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_
#define MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/AbsManagedWorkspace2D.h"

namespace Mantid
{
namespace DataObjects
{
/** The ManagedWorkspace2D allows the framework to handle 2D datasets that are too
    large to fit in the available system memory by making use of a temporary file.
    It is a specialisation of Workspace2D.

    The optional configuration property ManagedWorkspace.DataBlockSize sets the size
    (in bytes) of the blocks used to internally buffer data. The default is 1MB.

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2008

    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ManagedWorkspace2D : public AbsManagedWorkspace2D
{
public:
  ManagedWorkspace2D();
  virtual ~ManagedWorkspace2D();

  virtual const std::string id() const {return "ManagedWorkspace2D";}

  virtual size_t getMemorySize() const;
  virtual bool threadSafe() const { return false; }

  std::string get_filename() const;

  /// Get number of temporary files used to store the data.
  size_t getNumberFiles() const {return m_datafile.size();}

protected:

  /// Reads in a data block.
  virtual void readDataBlock(ManagedDataBlock2D *newBlock,std::size_t startIndex)const;
  /// Saves the dropped data block to disk.
  virtual void writeDataBlock(ManagedDataBlock2D *toWrite) const;

private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedWorkspace2D(const ManagedWorkspace2D&);
  /// Private copy assignment operator
  ManagedWorkspace2D& operator=(const ManagedWorkspace2D&);

  virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);

  virtual std::size_t getHistogramNumberHelper() const;

  ManagedDataBlock2D* getDataBlock(const std::size_t index) const;

  /// The number of blocks per temporary file
  std::size_t m_blocksPerFile;
  /// Size of each temp file in bytes
  long long m_fileSize;

  /// The name of the temporary file
  std::string m_filename;
  /// The stream handle to the temporary file used to store the data
  mutable std::vector<std::fstream*> m_datafile;
  /// Index written up to in temporary file
  mutable int m_indexWrittenTo;

  /// Static instance count. Used to ensure temporary filenames are distinct.
  static int g_uniqueID;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_*/
