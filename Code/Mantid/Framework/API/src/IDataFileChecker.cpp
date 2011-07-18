//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
  namespace API
  {

    /// Magic HDF cookie that is stored in the first 4 bytes of the file.
    const uint32_t IDataFileChecker::g_hdf_cookie = 0x0e031301;
    // Magic HDF5 signature (first value given as a decimal to avoid Intel compiler warning)
    const unsigned char IDataFileChecker::g_hdf5_signature[8] = { 137, 'H', 'D', 'F', '\r', '\n', '\032', '\n' };

    /// constructor
    IDataFileChecker::IDataFileChecker():API::Algorithm()
    {
    }

    /// destructor
    IDataFileChecker::~IDataFileChecker()
    {
    }

    /** returns the extension of the given file
     *  @param fileName :: name of the file.
     */
    std::string IDataFileChecker::extension(const std::string& fileName)
    {
      std::size_t pos=fileName.find_last_of(".");
      if(pos==std::string::npos)
      {
        return"" ;
      }
      std::string extn=fileName.substr(pos+1,fileName.length()-pos);
      std::transform(extn.begin(),extn.end(),extn.begin(),tolower);
      return extn;
    }

  }
}
