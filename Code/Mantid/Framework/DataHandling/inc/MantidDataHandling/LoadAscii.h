#ifndef MANTID_DATAHANDLING_LOADASCII_H_
#define MANTID_DATAHANDLING_LOADASCII_H_
/*WIKI* 

The LoadAscii algorithm reads in spectra data from a text file and stores it in a [[Workspace2D]] as data points. The data in the file must be organized in columns separated by commas, tabs, spaces, colons or semicolons. Only one separator type can be used throughout the file; use the "Separator" property to tell the algorithm which to use. 

By default the algorithm attempts to guess which lines are header lines by trying to see where a contiguous block of numbers starts. This can be turned off by specifying the "SkipNumLines" property, which will then tell the algorithm to simply use that as the the number of header lines.

The format can be one of:
* Two columns: 1st column=X, 2nd column=Y, E=0
* For a workspace of ''n'' spectra, 2''n''+1 columns: 1''st'' column=X, 2i''th'' column=Y, 2i+1''th'' column =E

The number of bins is defined by the number of rows.

The resulting workspace will have common X binning for all spectra.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a workspace from an ascii file. Spectra must be stored in columns.

    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    <li>Separator - the column separation character: comma (default),tab,space,colon,semi-colon.</li>
    <li>Unit      - the unit to assign to the X axis (default: Energy).</li>
    </ul>

    @author Roman Tolchenov, Tessella plc
    @date 3/07/09

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadAscii :public API::IDataFileChecker 
    {
    public:
      /// Default constructor
      LoadAscii();
      /// The name of the algorithm
      virtual const std::string name() const { return "LoadAscii"; }
      /// The version number
      virtual int version() const { return 1; }
      /// The category
      virtual const std::string category() const { return "DataHandling"; }
      /// Do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 
      /// 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);

      static bool isAscii(FILE *file);

    protected:
      /// Process the header information within the file.
      virtual void processHeader(std::ifstream & file) const;
      /// Read the data from the file
      virtual API::Workspace_sptr readData(std::ifstream & file) const;

      /// Peek at a line without extracting it from the stream
      void peekLine(std::ifstream & is, std::string & str) const;
      /// Return true if the line is to be skipped
      bool skipLine(const std::string & line) const;
      /// Split the data into columns.
      int splitIntoColumns(std::list<std::string> & columns, const std::string & str) const;
      /// Fill the given vector with the data values
      void fillInputValues(std::vector<double> &values, const std::list<std::string>& columns) const;

      /// The column separator
      std::string m_columnSep;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Declare properties
      void init();
      /// Execute the algorithm
      void exec();

      /// Map the separator options to their string equivalents
      std::map<std::string,std::string> m_separatorIndex;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADASCII_H_  */
