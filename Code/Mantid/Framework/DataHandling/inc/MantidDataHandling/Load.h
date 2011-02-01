#ifndef MANTID_DATAHANDLING_LOAD_H_
#define MANTID_DATAHANDLING_LOAD_H_

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
    Loads a workspace from a data file. The algorithm tries to determine the actual type
    of the file (raw, nxs, ...) and use the specialized loading algorith to load it.

    @author Roman Tolchenov, Tessella plc
    @date 38/07/2010

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

 
    class DLLExport Load : public API::Algorithm
    {
    public:
      /// Default constructor
      Load(){}
       /// Destructor
      ~Load() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Load"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }
      /// Category
      virtual const std::string category() const { return "DataHandling"; }

    private:
      ///init
      void init();
      /// execute
      void exec();

      /// This method returns shared pointer to load algorithm which got 
      /// the highest preference after file check.
      API::IAlgorithm_sptr getFileLoader(const std::string& filePath);
      /// Set the output workspace(s)
      void setOutputWorkspace(API::IAlgorithm_sptr&);
      /// intiliases the load algorithm with highest preference and sets this as a child algorithm
      void initialiseLoadSubAlgorithm(API::IAlgorithm_sptr alg, const double startProgress, 
				      const double endProgress, const bool enableLogging, const int& version);
    private:
      /// union used for identifying teh file type
      unsigned char* m_header_buffer;
       union { 
        unsigned u; 
        unsigned long ul; 
        unsigned char c[bufferSize+1]; 
      } header_buffer_union;
     };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOAD_H_  */
