#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/ProgressText.h"
#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Algorithms;

namespace Mantid
{
namespace MDEvents
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MakeDiffractionMDEventWorkspace)
  
  /// Sets documentation strings for this algorithm
  void MakeDiffractionMDEventWorkspace::initDocs()
  {
    this->setWikiSummary("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace. ");
    this->setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MakeDiffractionMDEventWorkspace::MakeDiffractionMDEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MakeDiffractionMDEventWorkspace::~MakeDiffractionMDEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MakeDiffractionMDEventWorkspace::init()
  {
    //TODO: Make sure in units are okay
    declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input), "An input EventWorkspace.");
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "Name of the output MDEventWorkspace.");
  }


  /// Our MDEvent dimension
  typedef MDEvent<3> MDE;


  //----------------------------------------------------------------------------------------------
  /** Convert an event list and add it to the MDEventWorkspace
   *
   * @param workspaceIndex :: index into the workspace
   */
  void MakeDiffractionMDEventWorkspace::convertEventList(int workspaceIndex)
  {
    EventList & el = in_ws->getEventList(workspaceIndex);

    // Get the position of the detector there.
    if (el.getDetectorIDs().size() > 0)
    {
      // The 3D MDEvents that will be added into the MDEventWorkspce
      std::vector<MDE> out_events;
      out_events.reserve( el.getNumberEvents() );

      // TODO: Handle or warn if sum of more than one detector ID
      int detID = *el.getDetectorIDs().begin();
      IDetector_sptr det = allDetectors[detID];

      // Vector between the sample and the detector
      V3D detPos = det->getPos() - samplePos;

      // Neutron's total travelled distance
      double distance = detPos.norm() + l1;

      // Detector direction normalized to 1
      V3D detDir = detPos / detPos.norm();

      // The direction of momentum transfer = the output beam direction - input beam direction (normalized)
      V3D Q_dir = detDir - beamDir;
      double Q_dir_x = Q_dir.X();
      double Q_dir_y = Q_dir.Y();
      double Q_dir_z = Q_dir.Z();


      //std::cout << wi << " : " << el.getNumberEvents() << " events. Pos is " << detPos << std::endl;

      // TODO: Generalize to other types of events
      std::vector<TofEvent> & events = el.getEvents();
      std::vector<TofEvent>::iterator it = events.begin();
      std::vector<TofEvent>::iterator it_end = events.end();
      for (; it != it_end; it++)
      {
        // Time of flight of neutron in seconds
        double tof = it->tof() * 1e-6;
        // Wavenumber = momentum/h_bar = mass*distance/time / h_bar
        double wavenumber = (PhysicalConstants::NeutronMass * distance) / (tof * PhysicalConstants::h_bar);
        // Convert to units of Angstroms^-1
        wavenumber *= 1e-10;

        // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
        CoordType center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};
        //std::cout << center[0] << "," << center[1] << "," << center[2] << "\n";

        // Build a MDEvent
        out_events.push_back( MDE(it->weight(), it->errorSquared(), center) );
      }

      ws->addEvents(out_events);
    }
    prog->report("Adding Events");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MakeDiffractionMDEventWorkspace::exec()
  {
    // Input workspace
    in_ws = getProperty("InputWorkspace");
    if (!in_ws)
      throw std::invalid_argument("No input event workspace was passed to algorithm.");

    // Create an output workspace with 3 dimensions.
    size_t nd = 3;
    IMDEventWorkspace_sptr i_out = MDEventFactory::CreateMDEventWorkspace(nd, "MDEvent");
    ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(i_out);

    if (!ws)
      throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

    // Give all the dimensions     //TODO: Find q limits
    for (size_t d=0; d<nd; d++)
    {
      Dimension dim(-100.0, +100.0, "Qx", "Angstroms^-1");
      ws->addDimension(dim);
    }
    ws->initialize();

    // Build up the box controller
    BoxController_sptr bc(new BoxController(3));
    bc->setSplitInto(4);
    ws->setBoxController(bc);


    // Extract some parameters global to the instrument
    AlignDetectors::getInstrumentParameters(in_ws->getInstrument(),l1,beamline,beamline_norm, samplePos);
    beamline_norm = beamline.norm();
    beamDir = beamline / beamline.norm();

    //To get all the detector ID's
    allDetectors = in_ws->getInstrument()->getDetectors();

    prog = new ProgressText(0, 1.0, in_ws->getNumberHistograms());

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int wi=0; wi < in_ws->getNumberHistograms(); wi++)
    {
      this->convertEventList(wi);
    }

//    ws->data

    std::cout << ws->getNPoints() << " MD events in the workspace now!\n";

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws));
  }



} // namespace Mantid
} // namespace MDEvents

