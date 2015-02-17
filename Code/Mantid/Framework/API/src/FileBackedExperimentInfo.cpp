#include "MantidAPI/FileBackedExperimentInfo.h"

namespace Mantid
{
namespace API
{
namespace {
/// static logger object
Kernel::Logger g_log("FileBackedExperimentInfo");
}

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FileBackedExperimentInfo::FileBackedExperimentInfo(::NeXus::File file, std::string groupName)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FileBackedExperimentInfo::~FileBackedExperimentInfo()
  {
  }

  //------------------------------------------------------------------------------------------------------
  // Private members
  //------------------------------------------------------------------------------------------------------
  /**
   * Here we actually load the data
   */
  void FileBackedExperimentInfo::intialise() {
      std::string parameterStr;
      try {
        // Get the sample, logs, instrument
        this->loadExperimentInfoNexus(file, parameterStr);
        // Now do the parameter map
        this->readParameterMap(parameterStr);
      } catch (std::exception &e) {
        g_log.information("Error loading section '" + groupName +
                          "' of nxs file.");
        g_log.information(e.what());
      }
  }

} // namespace API
} // namespace Mantid
