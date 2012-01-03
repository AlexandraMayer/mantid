#ifndef MANTID_MDEVENTS_CONVERTTODIFFRACTIONMDWORKSPACE_H_
#define MANTID_MDEVENTS_CONVERTTODIFFRACTIONMDWORKSPACE_H_

#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"
#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** ConvertToDiffractionMDWorkspace :
   * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace.
   * 
   * @author Janik Zikovsky, SNS
   * @date 2011-03-01 13:14:48.236513
   */
  class DLLExport ConvertToDiffractionMDWorkspace  : public BoxControllerSettingsAlgorithm
  {
  public:
    ConvertToDiffractionMDWorkspace();
    ~ConvertToDiffractionMDWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToDiffractionMDWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

    template <class T>
    void convertEventList(int workspaceIndex);

    /// The input event workspace
    DataObjects::EventWorkspace_sptr in_ws;
    /// The output MDEventWorkspace<3>
    MDEvents::MDEventWorkspace3Lean::sptr ws;
    /// Do we clear events on the input during loading?
    bool ClearInputWorkspace;
    /// Are we appending?
    bool Append;
    /// Perform LorentzCorrection on the fly.
    bool LorentzCorrection;
    /// Map of all the detectors in the instrument
    detid2det_map allDetectors;
    /// Primary flight path (source to sample)
    double l1;
    /// Beam direction and length
    Kernel::V3D beamline;
    /// Path length between source and sample
    double beamline_norm;
    /// Path length between source and sample
    size_t failedDetectorLookupCount;
    /// Beam direction (unit vector)
    Kernel::V3D beamDir;
    /// Sample position
    Kernel::V3D samplePos;
    /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
    /// Matrix. Multiply this by the lab frame Qx, Qy, Qz to get the desired Q or HKL.
    Kernel::Matrix<double> mat;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_CONVERTTODIFFRACTIONMDWORKSPACE_H_ */
