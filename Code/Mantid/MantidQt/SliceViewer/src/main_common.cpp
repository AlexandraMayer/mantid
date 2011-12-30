#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::Geometry::MDHistoDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Kernel::VMD;


/** Creates a fake MDHistoWorkspace
 *
 * @param signal :: signal and error squared in every point
 * @param numDims :: number of dimensions to create. They will range from 0 to max
 * @param numBins :: bins in each dimensions
 * @param max :: max position in each dimension
 * @return the MDHisto
 */
Mantid::MDEvents::MDHistoWorkspace_sptr makeFakeMDHistoWorkspace(double signal, size_t numDims, size_t numBins,
    double max)
{
  Mantid::MDEvents::MDHistoWorkspace * ws = NULL;
  if (numDims ==1)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)) );
  }
  else if (numDims == 2)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins))  );
  }
  else if (numDims == 3)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("yy","y","furlongs", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("energy","z","meV", 0.0, max, numBins))   );
  }
  else if (numDims == 4)
  {
    ws = new Mantid::MDEvents::MDHistoWorkspace(
        MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("z","z","m", 0.0, max, numBins)),
        MDHistoDimension_sptr(new MDHistoDimension("t","z","m", 0.0, max, numBins))
        );
  }
  Mantid::MDEvents::MDHistoWorkspace_sptr ws_sptr(ws);
  ws_sptr->setTo(signal, signal);
  return ws_sptr;
}



//-------------------------------------------------------------------------------
/** Add a fake "peak"*/
static void addPeak(size_t num, double x, double y, double z, double radius)
{
  std::ostringstream mess;
  mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
  FrameworkManager::Instance().exec("FakeMDEventData", 6,
      "InputWorkspace", "mdew",
      "PeakParams", mess.str().c_str(),
      "RandomSeed", "1234");
}

//-------------------------------------------------------------------------------
/** Make a demo data set for testing */
IMDWorkspace_sptr makeDemoData(bool binned = false)
{
  // Create a fake workspace
  //size_t numBins = 100;

  // ---- Start with empty MDEW ----
  FrameworkManager::Instance().exec("CreateMDWorkspace", 16,
      "Dimensions", "3",
      "Extents", "-10,10,-10,10,-10,10",
      "Names", "h,k,l",
      "Units", "lattice,lattice,lattice",
      "SplitInto", "5",
      "SplitThreshold", "100",
      "MaxRecursionDepth", "20",
      "OutputWorkspace", "mdew");
  addPeak(15000,0,0,0, 1);
  addPeak(5000,0,0,0, 0.3);
  addPeak(5000,0,0,0, 0.2);
  addPeak(5000,0,0,0, 0.1);
//  addPeak(12000,0,0,0, 0.03);
  IMDEventWorkspace_sptr mdew = boost::dynamic_pointer_cast<IMDEventWorkspace>( AnalysisDataService::Instance().retrieve("mdew") );
  mdew->splitAllIfNeeded(NULL);
  if (binned)
  {
    // Bin aligned to original
//    FrameworkManager::Instance().exec("BinMD", 12,
//        "InputWorkspace", "mdew",
//        "OutputWorkspace", "binned",
//        "AxisAligned", "1",
//        "AlignedDimX", "h, -10, 10, 100",
//        "AlignedDimY", "k, -10, 10, 100",
//        "AlignedDimZ", "l, -10, 10, 100"
//        );
    FrameworkManager::Instance().exec("BinMD", 16,
        "InputWorkspace", "mdew",
        "OutputWorkspace", "binned",
        "AxisAligned", "0",
        "BasisVectorX", "rx, m, 1.0, 0.0, 0.0, 20.0, 100",
        "BasisVectorY", "ry, m, 0.0, 1.0, 0.0, 20.0, 100",
        "BasisVectorZ", "ry, m, 0.0, 0.0, 1.0, 20.0, 100",
        "ForceOrthogonal", "1",
        "Origin", "-10, -10, -10");

    return boost::dynamic_pointer_cast<IMDWorkspace>( AnalysisDataService::Instance().retrieve("binned") );
  }
  else
    return boost::dynamic_pointer_cast<IMDWorkspace>(mdew);
}
