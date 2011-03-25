#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/ProgressText.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"

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

  bool DODEBUG = false;

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
    declareProperty(new PropertyWithValue<bool>("ClearInputWorkspace", false, Direction::Input), "Clear the events from the input workspace during conversion, to save memory.");
  }


  /// Our MDEvent dimension
  typedef MDEvent<3> MDE;


  //----------------------------------------------------------------------------------------------
  /** Convert an event list to 3D q-space and add it to the MDEventWorkspace
   *
   * @tparam T :: the type of event in the input EventList (TofEvent, WeightedEvent, etc.)
   * @param workspaceIndex :: index into the workspace
   */
  template <class T>
  void MakeDiffractionMDEventWorkspace::convertEventList(int workspaceIndex)
  {
    EventList & el = in_ws->getEventList(workspaceIndex);
    size_t numEvents = el.getNumberEvents();

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

      // This little dance makes the getting vector of events more general (since you can't overload by return type).
      typename std::vector<T> * events_ptr;
      getEventsFrom(el, events_ptr);
      typename std::vector<T> & events = *events_ptr;

      // Iterators to start/end
      typename std::vector<T>::iterator it = events.begin();
      typename std::vector<T>::iterator it_end = events.end();
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

      // Clear out the EventList to save memory
      if (ClearInputWorkspace)
      {

        // Track how much memory you cleared
        size_t memoryCleared = el.getMemorySize();
        // Clear it now
        el.clear();
        // For Linux with tcmalloc, make sure memory goes back, if you've cleared 200 Megs
        MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, 2e8);
      }

      // Add them to the MDEW
      ws->addEvents(out_events);
    }
    prog->reportIncrement(numEvents, "Adding Events");
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

    ClearInputWorkspace = getProperty("ClearInputWorkspace");

    // Create an output workspace with 3 dimensions.
    size_t nd = 3;
    IMDEventWorkspace_sptr i_out = MDEventFactory::CreateMDEventWorkspace(nd, "MDEvent");
    ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(i_out);

    if (!ws)
      throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

    // Give all the dimensions     //TODO: Find q limits
    std::string names[3] = {"Qx", "Qy", "Qz"};
    for (size_t d=0; d<nd; d++)
    {
      Dimension dim(-1000.0, +1000.0, names[d], "Angstroms^-1");
      ws->addDimension(dim);
    }
    ws->initialize();

    // Build up the box controller
    BoxController_sptr bc(new BoxController(3));
    bc->setSplitInto(4);
    bc->setSplitThreshold(1000);
    ws->setBoxController(bc);

    // We always want the box to be split (it will reject bad ones)
    ws->splitBox();

    // Extract some parameters global to the instrument
    AlignDetectors::getInstrumentParameters(in_ws->getInstrument(),l1,beamline,beamline_norm, samplePos);
    beamline_norm = beamline.norm();
    beamDir = beamline / beamline.norm();

    //To get all the detector ID's
    in_ws->getInstrument()->getDetectors(allDetectors);

    size_t totalCost = in_ws->getNumberEvents();
    prog = new Progress(this, 0, 1.0, totalCost);
    if (DODEBUG) prog = new ProgressText(0, 1.0, totalCost, true);
    if (DODEBUG) prog->setNotifyStep(1);

    // Create the thread pool that will run all of these.
    ThreadScheduler * ts = new ThreadSchedulerLargestCost();
    ThreadPool tp(ts);

    // To track when to split up boxes
    size_t eventsAdded = 0;
    size_t lastNumBoxes = ws->getBox()->getNumMDBoxes();

    Timer tim;

    for (int wi=0; wi < in_ws->getNumberHistograms(); wi++)
    {
      // Equivalent of: this->convertEventList(wi);
      EventList & el = in_ws->getEventList(wi);

      // We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
      boost::function<void ()> func;
      switch (el.getEventType())
      {
      case TOF:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<TofEvent>, &*this, wi);
        break;
      case WEIGHTED:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<WeightedEvent>, &*this, wi);
        break;
      case WEIGHTED_NOTIME:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<WeightedEventNoTime>, &*this, wi);
        break;
      default:
        throw std::runtime_error("EventList had an unexpected data type!");
      }

      // Give this task to the scheduler
      double cost = el.getNumberEvents();
      ts->push( new FunctionTask( func, cost) );

      // Keep a running total of how many events we've added
      eventsAdded += cost;
      if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes))
      {
        if (DODEBUG) std::cout << "We've added tasks worth " << eventsAdded << " events. There are " << lastNumBoxes << " boxes. Let's start adding.\n";
        // Do all the adding tasks
        tp.joinAll();

        // Now do all the splitting tasks
        if (DODEBUG) std::cout << "About to start splitAllIfNeeded() tasks. There are now " << ws->getBox()->getNumMDBoxes() << " boxes\n";
        ws->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          prog->doReport("Splitting Boxes");
        tp.joinAll();

        // Count the new # of boxes.
        lastNumBoxes = ws->getBox()->getNumMDBoxes();
        eventsAdded = 0;
        if (DODEBUG) std::cout << "There are now " << ws->getBox()->getNumMDBoxes() << " boxes. Let's keep adding tasks for events.\n";
      }
    }

    if (DODEBUG) std::cout << "We're done adding tasks for all event lists. " << eventsAdded << " events are left to add. Let's finish those actual tasks.\n";
    tp.joinAll();

    // Do a final splitting of everything
    if (DODEBUG) std::cout << "Do a final splitting of everything, if it is needed."  << "There are currently " << ws->getBox()->getNumMDBoxes() << " boxes\n";;
    ws->splitAllIfNeeded(ts);
    tp.joinAll();

    if (DODEBUG) std::cout << "There are now " << ws->getBox()->getNumMDBoxes() << " boxes\n";

    // Recount totals at the end.
    ws->refreshCache();

    if (DODEBUG) std::cout << "Workspace has " << ws->getNPoints() << " events. This took " << tim.elapsed() << " sec.\n\n\n";


    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws));
  }



} // namespace Mantid
} // namespace MDEvents

