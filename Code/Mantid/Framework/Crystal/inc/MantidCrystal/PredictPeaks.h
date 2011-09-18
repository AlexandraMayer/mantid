#ifndef MANTID_CRYSTAL_PREDICTPEAKS_H_
#define MANTID_CRYSTAL_PREDICTPEAKS_H_
/*WIKI* 

This algorithm uses the InputWorkspace to determine the instrument in use, as well as the UB Matrix and Unit Cell of the sample used.

The algorithm operates by calculating the scattering direction (given the UB matrix) for a particular HKL, and determining whether that hits a detector. The MinDSpacing parameter is used to determine what HKL's to try.

The parameters of WavelengthMin/WavelengthMax also limit the peaks attempted to those that can be detected/produced by your instrument.
*WIKI*/
    
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace Crystal
{

  /** Using a known crystal lattice and UB matrix, predict where single crystal peaks
   * should be found in detector/TOF space. Creates a PeaksWorkspace containing
   * the peaks at the expected positions.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-29 16:30:52.986094
   */
  class DLLExport PredictPeaks  : public API::Algorithm
  {
  public:
    PredictPeaks();
    ~PredictPeaks();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "PredictPeaks";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    void doHKL(const int h, const int k, const int l);

  private:
    /// Reflection conditions possible
    std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;

    /// Min wavelength parameter
    double wlMin;
    /// Max wavelength parameter
    double wlMax;
    /// Instrument reference
    Geometry::Instrument_const_sptr inst;
    /// Output peaks workspace
    Mantid::DataObjects::PeaksWorkspace_sptr pw;
    /// Counter of possible peaks
    size_t numInRange;
    /// Crystal applied
    OrientedLattice crystal;
    /// Min D spacing to apply.
    double minD;
    /// Max D spacing to apply.
    double maxD;
    /// Rotation matrix
    Mantid::Kernel::DblMatrix mat;

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_PREDICTPEAKS_H_ */
