#ifndef MANTID_DATAHANDLING_GROUPDETECTORS2_H_
#define MANTID_DATAHANDLING_GROUPDETECTORS2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <climits>
#include <map>
#ifdef HAS_UNORDERED_MAP_H
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace DataHandling
{
  using DataObjects::Workspace2D_const_sptr;
  using API::MatrixWorkspace_sptr;
/** An algorithm for grouping detectors and the spectra associated with them
    into a single DetectorGroup and spectrum.
    This algorithm can only be used on a workspace that has common X bins.
    
    File input must have the following format:

  |      The format of the grouping file each phrase in "" is replaced by
  | a single integer (ignore all | and my insersions in []). Extra space
  | and comments starting with # are both allowed

  | "unused number"                                         |[in some implementation this is the number of groups in the file but here all groups in the file are read regardless]
  | "unused number"                                         |[an integer must be here but the group's spectra number is the spectra number of the first spectra that went into the group and its index number is the number of groups in the list before it]
  | "number_of_input_spectra1"                              |[this number must equal the number of spectra numbers on the next lines]
  | "input spec1" "input spec2" "input spec3" "input spec4" |[these spectra numbers can be listed over any number of lines]
  | "input spec5 input spec6"                               |
  ------------------------------------------------------------------------
  | "unused number"                                         |[this section of the file is repeated once for each group to form]
  | "number_of_input_spectra2"                              |[not all the input sectra have to be included in a group
  | "input spec1" "input spec2" "input spec3" "input spec4" |
  | "input spec5 input spec6"                               |
  ------------------------------------------------------------------------

  An example of an input file follows:
  2  
  2 1  
  3 64  
  4 1 2 3 4 5 6 7 8 9 10  
  5 11 12 13 14 15 16 17 18 19 20  
  6 21 22 23 24 25 26 27 28 29 30  
  7 31 32 33 34 35 36 37 38 39 40  
  8 41 42 43 44 45 46 47 48 49 50  
  9 51 52 53 54 55 56 57 58 59 60  
  10 61 62 63 64  
  11 2  
  12 64  
  13 65 66 67 68 69 70 71 72 73 74  
  14 75 76 77 78 79 80 81 82 83 84  
  15 85 86 87 88 89 90 91 92 93 94  
  16 95 96 97 98 99 100 101 102 103 104  
  17 105 106 107 108 109 110 111 112 113 114  
  18 115 116 117 118 119 120 121 122 123 124  
  19 125 126 127 128  

    @author Steve Williams and Russell Taylor, Tessella Support Services plc
    @date 27/07/2009

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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
class DLLExport GroupDetectors2 : public API::Algorithm
{
public:
  GroupDetectors2();
  virtual ~GroupDetectors2();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GroupDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 2;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}

private:
	#ifndef HAS_UNORDERED_MAP_H
  /// used to store the lists of spectra numbers that will be grouped, the keys are not used
  typedef std::map<int, std::vector<int> > storage_map;
	#else
  typedef std::tr1::unordered_map<int, std::vector<int> > storage_map;
	#endif

  /// An estimate of the percentage of the algorithm runtimes that has been completed 
  double m_PercentDone;

  // Implement abstract Algorithm methods
  void init();
  void exec();
  
  /// read in the input parameters and see what findout what will be to grouped
  void getList(Workspace2D_const_sptr workspace, storage_map &outputData,
                                             std::vector<int> &unUsedSpec);
  /// gets the list of spectra _index_ _numbers_ from a file of _spectra_ _numbers_ 
  void readTheFile(std::string fname,  Workspace2D_const_sptr workspace,
                     storage_map &outputData, std::vector<int> &unUsedSpec);
  /// used while reading the file turns the string into an integer number (if possible), white space and # comments ignored
  int readInt(std::string line);
  /// used while reading the file reads reads specftra numbers from the string and returns spectra indexes 
  void readSpectraIndexes(std::string line, std::map<int,int> &specs2index,
                   std::vector<int> &output, std::vector<int> &unUsedSpec);
  void readGroups(std::map<int,int> &specs2index, std::ifstream &File,
    int &lineNum, storage_map &output, std::vector<int> &unUsedSpec);

  /// Estimate how much what has been read from the input file constitutes progress for the algorithm
  double fileReadProg(int numGroupsRead, int numInHists);

  /// Copy the and combine the histograms that the user requested from the input into the output workspace
  int makeGroups(const storage_map &inputList, Workspace2D_const_sptr inputWS,
                 MatrixWorkspace_sptr outputWS, const double prog4Copy);
  /// Copy the data data in ungrouped histograms from the input workspace to the output
  void moveOthers(const std::set<int> &unGroupedSet,
    Workspace2D_const_sptr inputWS,MatrixWorkspace_sptr outputWS,int outIndex);

  enum flagvalues                                        ///< flag values used in arrays or as return values
  {
    USED = 1000-INT_MAX,                                 ///< goes in the unGrouped spectra list to say that a spectrum will be included in a group, any other value and it isn't. Spectra numbers should always be positive so we shouldn't accidientally set a spectrum number to the this
    EMPTY_LINE = 1001-INT_MAX                            ///< when reading from the input file this value means that we found any empty line
  };
  
  enum progressReps                                       ///< contains the numbers used for progress notifcation
  {
    CHECKBINS = 15,                                       ///< a (worse case) estimate of the time required to check that the X bin boundaries are the same as a percentage of total algorithm run time
    OPENINGFILE = 5,                                      ///< gives the progress bar a nudge when the file opens
  // if CHECKBINS+OPENINGFILE+2*READFILE > 100 then the algorithm might report progress that is greater than 100
    READFILE = 30,                                        ///< if a file must be read in estimate that reading it will take this percentage of the algorithm execution time
    INTERVAL = 128                                        ///< copy this many histograms and then check for an algorithm notification and update the progress bar
  };
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GROUPDETECTORS2_H_*/
