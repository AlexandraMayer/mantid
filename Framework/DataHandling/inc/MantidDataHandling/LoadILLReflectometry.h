#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLReflectometry : Loads a ILL Reflectometry data file.

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport LoadILLReflectometry
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLReflectometry();
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override {
    return "LoadILLReflectometry";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1;}
  /// Algorithm's category for search and find. @see Algorithm::category
  const std::string category() const override {return "DataHandling\\Nexus";}
  /// Algorithm's summary. @see Algorithm::summary
  const std::string summary() const override {
    return "Loads a ILL/D17 nexus file.";
  }
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  void initWorkspace(NeXus::NXEntry &entry,
                     std::vector<std::vector<int>> monitorsData);
  void setInstrumentName(const NeXus::NXEntry &firstEntry,
                         const std::string &instrumentNamePath);
  void loadDataDetails(NeXus::NXEntry &entry);
  void loadData(NeXus::NXEntry &entry,
                                std::vector<std::vector<int>> monitorsData,
                                std::string &filename);
  void loadNexusEntriesIntoProperties(std::string nexusfilename);
  std::vector<int>loadSingleMonitor(NeXus::NXEntry &entry, std::string monitor_data);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);
  void runLoadInstrument();
  //void centerDetector(double);
  void placeDetector(NeXus::NXEntry &entry);

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument

  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z

  size_t m_numberOfHistograms;

  /* Values parsed from the nexus file */
  double m_wavelength;
  double m_channelWidth;

  std::unordered_set<std::string> m_supportedInstruments;
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_ */
