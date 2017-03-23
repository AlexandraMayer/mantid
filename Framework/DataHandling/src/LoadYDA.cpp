#include "MantidDataHandling/LoadYDA.h"

#include "MantidAPI/FileProperty.h"

#include "yaml-cpp/yaml.h"
#include <iostream>


namespace Mantid {
namespace DataHandling {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadYDA)

//----------------------------------------------------------------------------------------------

LoadYDA::LoadYDA() {}

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadYDA::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadYDA::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

int LoadYDA::confidence(Kernel::FileDescriptor &descriptor) const {
    return 50;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadYDA::init() {
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "",API::FileProperty::Load,".yaml"),
      "An input File.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "",Kernel::Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadYDA::exec() {
  const std::string filename = getPropertyValue("Filename");
  YAML::Node f = YAML::LoadFile(filename);
  g_log.debug("Problem with debug?");
  g_log.debug(std::to_string(f.IsNull()));
  for(YAML::const_iterator it = f.begin(); it != f.end();++it) {
      g_log.debug(it->first.as<std::string>());
  }

  g_log.debug(f["History"][0].as<std::string>());

  auto hist = f["History"];
  std::string propn = hist[0].as<std::string>();
  propn = propn.back();
  g_log.debug(propn);
  std::string propt = hist[1].as<std::string>();
   g_log.debug(propt);
  std::string expteam = hist[2].as<std::string>();
  g_log.debug(expteam);

  auto rpar = f["RPar"];
  std::string temp = rpar[0]["val"].as<std::string>();
  g_log.debug(temp);
  std::string ei = rpar[1]["val"].as<std::string>();
  g_log.debug(ei);
  auto coord = f["Coord"];
  std::string z = coord["z"]["name"].as<std::string>();
  g_log.debug(z);

  auto slices = f["Slices"];


  g_log.debug(slices[0]["x"][0].as<std::string>());



}

} // namespace DataHandling
} // namespace Mantid
