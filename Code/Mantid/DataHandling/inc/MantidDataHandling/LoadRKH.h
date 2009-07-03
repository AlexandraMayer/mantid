#ifndef MANTID_DATAHANDLING_LOADRKH_H_
#define MANTID_DATAHANDLING_LOADRKH_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <istream>

namespace Mantid
{
namespace DataHandling
{
  /**
     Loads an RKH file into a Mantid 1D workspace

     Required properties:
     <UL>
     <LI> Filename - The path to the file in RKH format</LI>
     <LI> FirstColumnValue - The units of the first column in the file</LI>
     <LI> OutputWorkspace - The name output workspace.</LI>
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
     Copyright &copy; 2009 STFC Rutherford Appleton Laboratory
     
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
class DLLExport LoadRKH : public Mantid::API::Algorithm
{
public:
  /// Constructor
  LoadRKH() : Mantid::API::Algorithm(), m_unitKeys(), m_RKHKeys() {}
  /// Virtual destructor
  virtual ~LoadRKH() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadRKH"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  // Initialisation code
  void init();
  // Execution code
  void exec();

  // Remove lines from an input stream
  void skipLines(std::istream & strm, int nlines);

  /// Store the units known to the UnitFactory
  std::set<std::string> m_unitKeys;
  /// Store the units added as options for this algorithm
  std::set<std::string> m_RKHKeys;

};

}
}
#endif /*MANTID_DATAHANDLING_LOADRKH_H_*/
