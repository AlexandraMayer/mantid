#ifndef MANTID_DATAHANDLING_SAVECSV_H_
#define MANTID_DATAHANDLING_SAVECSV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DataHandlingCommand.h"
#include "Logger.h"

namespace Mantid
{
namespace DataHandling
{
/** @class SaveCSV SaveCSV.h DataHandling/SaveCSV.h

    Saves a 1D or 2D workspace to a CSV file. SaveCSV is an algorithm and as such 
    inherits from the Algorithm class, via DataHandlingCommand, and overrides
    the init(), exec() & final() methods.
    
    Required Properties:
       <UL>
       <LI> Filename - The name of file to store the workspace to </LI>
       <LI> InputWorkspace - The name of a workspace </LI>
       </UL>

    Optional Properties:
       <UL>
       <LI> Seperator - defaults to "," </LI>
       <LI> LineSeperator - defaults to "\n" </LI>
       </UL>
			 
		The format of the saved ascii CSV file for a 1D worksspace consists of three
		columns where the numbers of each row are seperated by the Seperator and
		each line by the LineSeperator.
		
		The format of the saved CSV file for a 2D workspace is as follows:
		
    A      0, 200, 400, 600, ..., 50000 <BR>
		0     10,   4, 234,  35, ...,    32 <BR>
		1      4, 234,   4,   9, ...,    12 <BR>
		A      0, 100, 200, 300, ..., 25000 <BR>
		2     34,   0,   0,   0, ...,    23
		
		ERRORS<BR>
		0    0.1, 3.4, 2.4, 3.5, ...,     2 <BR>
		1    3.1, 3.3, 2.5, 3.5, ...,     2 <BR>
		2    1.1, 3.3, 2.4,   5, ...,   2.4
		
		where for the matrix above the ERRORS line the first column 
		shows the content of the numbers on the of the same line; i.e.
		'A' is followed by x-axis values (e.g. TOF values) and any number
		(e.g. '2') followed by y-axis values (e.g. neutron counts). Multiple
		'A' may be present to allow for the a-axis to change. So in
		the example above the saved 2D workspace consists of three histograms
	  (y-axes) where the first two have the same x-axis but the third
		histogram has a different x-axis. 
		
		The matrix following the ERRORS line lists the errors as recorded
		for each histogram.   

    @author Anders Markvardsen, ISIS, RAL
    @date 15/10/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
  class DLLExport SaveCSV : public DataHandlingCommand
  {
  public:
    /// Default constructor
    SaveCSV();
    
    /// Destructor
    ~SaveCSV() {}
    
  private:

    /// Overwrites Algorithm method. Does nothing at present
    Kernel::StatusCode init();
    
    /// Overwrites Algorithm method
    Kernel::StatusCode exec();
    
    /// Overwrites Algorithm method. Does nothing at present
    Kernel::StatusCode final();
    
    /// The name of the file used for storing the workspace
    std::string m_filename;
    
    /// The seperator for the CSV file
    std::string m_seperator;
    
    /// The line seperator for the CSV file
    std::string m_lineSeperator;   

    ///static reference to the logger class
    static Kernel::Logger& g_log;
  };

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVECSV_H_*/
