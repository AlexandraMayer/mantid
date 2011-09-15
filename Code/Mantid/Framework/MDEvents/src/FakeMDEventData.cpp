#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/FakeMDEventData.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <math.h>
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FakeMDEventData)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FakeMDEventData::FakeMDEventData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FakeMDEventData::~FakeMDEventData()
  {
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void FakeMDEventData::initDocs()
  {
    this->setWikiSummary("Adds fake multi-dimensional event data to an existing MDEventWorkspace, for use in testing.\nYou can create a blank MDEventWorkspace with CreateMDWorkspace.");
    this->setOptionalMessage("Adds fake multi-dimensional event data to an existing MDEventWorkspace, for use in testing.\nYou can create a blank MDEventWorkspace with CreateMDWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FakeMDEventData::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut),
        "An input workspace, that will get MDEvents added to it");

    declareProperty(new ArrayProperty<double>("UniformParams", ""),
        "Add a uniform, randomized distribution of events.\n"
        "1 parameter: number_of_events; they will be distributed across the size of the workspace.\n"
        "Multiple parameters: number_of_events, min,max (for each dimension); distribute the events inside the range given.\n");

    declareProperty(new ArrayProperty<double>("PeakParams", ""),
        "Add a peak with a normal distribution around a central point.\n"
        "Parameters: number_of_events, x, y, z, ..., radius.\n");

    declareProperty(new PropertyWithValue<int>("RandomSeed", 0),
        "Seed int for the random number generator.");

    declareProperty(new PropertyWithValue<bool>("RandomizeSignal", false),
        "If true, the events' signal and error values will be randomized around 1.0+-0.5.");

  }



  //----------------------------------------------------------------------------------------------
  /** Function makes up a fake single-crystal peak and adds it to the workspace.
   *
   * @param ws
   */
  template<typename MDE, size_t nd>
  void FakeMDEventData::addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::vector<double> params = getProperty("PeakParams");
    bool RandomizeSignal = getProperty("RandomizeSignal");
    if (params.size() == 0)
      return;

    if (params.size() != nd + 2)
      throw std::invalid_argument("PeakParams needs to have ndims+2 arguments.");
    if (params[0] <= 0)
      throw std::invalid_argument("PeakParams: number_of_events needs to be > 0");
    size_t num = size_t(params[0]);

    Progress prog(this, 0.0, 1.0, 100);
    size_t progIncrement = num / 100; if (progIncrement == 0) progIncrement = 1;

    // Width of the peak
    double desiredRadius = params.back();

    boost::mt19937 rng;
    boost::uniform_real<double> u2(0, 1.0); // Random from 0 to 1.0
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > genUnit(rng, u2);
    int randomSeed = getProperty("RandomSeed");
    rng.seed((unsigned int)(randomSeed));

    for (size_t i=0; i<num; ++i)
    {
      // Algorithm to generate points along a random n-sphere (sphere with not necessarily 3 dimensions)
      // from http://en.wikipedia.org/wiki/N-sphere as of May 6, 2011.

      // First, points in a hyper-cube of size 1.0, centered at 0.
      coord_t centers[nd];
      coord_t radiusSquared = 0;
      for (size_t d=0; d<nd; d++)
      {
        centers[d] = genUnit()-0.5; // Distribute around +- the center
        radiusSquared += centers[d]*centers[d];
      }

      // Make a unit vector pointing in this direction
      coord_t radius = sqrt(radiusSquared);
      for (size_t d=0; d<nd; d++)
        centers[d] /= radius;

      // Now place the point along this radius, scaled with ^1/n for uniformity.
      double radPos = genUnit();
      radPos = pow(radPos, 1.0/double(nd));
      for (size_t d=0; d<nd; d++)
      {
        // Multiply by the scaling and the desired peak radius
        centers[d] *= (radPos * desiredRadius);
        // Also offset by the center of the peak, as taken in Params
        centers[d] += params[d+1];
      }

      // Default or randomized error/signal
      float signal = 1.0;
      float errorSquared = 1.0;
      if (RandomizeSignal)
      {
        signal = float(0.5 + genUnit());
        errorSquared = float(0.5 + genUnit());
      }

      // Create and add the event.
      ws->addEvent( MDE( signal, errorSquared, centers) );
      // Progress report
      if ((i % progIncrement) == 0) prog.report();
    }

    ws->splitBox();
    Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);
    ws->splitAllIfNeeded(ts);
    tp.joinAll();
    ws->refreshCache();
  }

  //----------------------------------------------------------------------------------------------
  /** Function makes up a fake uniform event data and adds it to the workspace.
   *
   * @param ws
   */
  template<typename MDE, size_t nd>
  void FakeMDEventData::addFakeUniformData(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::vector<double> params = getProperty("UniformParams");
    bool RandomizeSignal = getProperty("RandomizeSignal");
    if (params.size() == 0)
      return;

    if (params[0] <= 0)
      throw std::invalid_argument("UniformParams: number_of_events needs to be > 0");
    size_t num = size_t(params[0]);

    Progress prog(this, 0.0, 1.0, 100);
    size_t progIncrement = num / 100; if (progIncrement == 0) progIncrement = 1;

    if (params.size() == 1)
    {
      for (size_t d=0; d<nd; ++d)
      {
        params.push_back( ws->getDimension(d)->getMinimum() );
        params.push_back( ws->getDimension(d)->getMaximum() );
      }
    }
    if (params.size() != 1 + nd*2)
      throw std::invalid_argument("UniformParams: needs to have ndims*2+1 arguments.");

    boost::mt19937 rng;
    int randomSeed = getProperty("RandomSeed");
    rng.seed((unsigned int)(randomSeed));

    // Unit-size randomizer
    boost::uniform_real<double> u2(0, 1.0); // Random from 0 to 1.0
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > genUnit(rng, u2);

    // Make a random generator for each dimensions
    typedef boost::variate_generator<boost::mt19937&, boost::uniform_real<double> >   gen_t;

    gen_t * gens[nd];
    for (size_t d=0; d<nd; ++d)
    {
      double min = params[d*2+1];
      double max = params[d*2+2];
      if (max <= min) throw std::invalid_argument("UniformParams: min must be < max for all dimensions.");
      boost::uniform_real<double> u(min,max); // Range
      gen_t * gen = new gen_t(rng, u);
      gens[d] = gen;
    }

    // Create all the requested events
    for (size_t i=0; i<num; ++i)
    {
      coord_t centers[nd];
      for (size_t d=0; d<nd; d++)
        centers[d] = (*gens[d])(); // use a different generator for each dimension

      // Default or randomized error/signal
      float signal = 1.0;
      float errorSquared = 1.0;
      if (RandomizeSignal)
      {
        signal = float(0.5 + genUnit());
        errorSquared = float(0.5 + genUnit());
      }

      // Create and add the event.
      ws->addEvent( MDE( signal, errorSquared, centers) );
      // Progress report
      if ((i % progIncrement) == 0) prog.report();
    }

    /// Clean up the generators
    for (size_t d=0; d<nd; ++d)
      delete gens[d];

    ws->splitBox();
    Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts);
    ws->splitAllIfNeeded(ts);
    tp.joinAll();
    ws->refreshCache();
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FakeMDEventData::exec()
  {
    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    if (getPropertyValue("UniformParams")=="" && getPropertyValue("PeakParams")=="")
      throw std::invalid_argument("You must specify at least one of PeakParams or UniformParams.");

    CALL_MDEVENT_FUNCTION(this->addFakePeak, in_ws)
    CALL_MDEVENT_FUNCTION(this->addFakeUniformData, in_ws)
  }



} // namespace Mantid
} // namespace MDEvents

