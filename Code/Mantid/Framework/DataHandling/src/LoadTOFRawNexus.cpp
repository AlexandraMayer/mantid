// Includes

#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
//DECLARE_ALGORITHM( LoadTOFRawNexus)

using namespace Kernel;
using namespace API;

LoadTOFRawNexus::LoadTOFRawNexus()
{
  // TODO Auto-generated constructor stub

}

/// Initialisation method.
void LoadTOFRawNexus::init()
{

  std::vector < std::string > exts;
  exts.push_back(".nxs");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the NeXus file to load");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "", Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumMin", 0, mustBePositive);
  declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
  declareProperty(new Kernel::ArrayProperty<int>("SpectrumList"));
  declareProperty("EntryNumber", 0, mustBePositive->clone(),
      "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void LoadTOFRawNexus::exec()
{
  // Create the root Nexus class
  Mantid::NeXus::NXRoot root(getPropertyValue("Filename"));

  // Open the default data group 'entry'
  // TODO: Loop over the available entries
  Mantid::NeXus::NXEntry entry = root.openEntry("entry");

}

} // namespace DataHandling
} // namespace Mantid

