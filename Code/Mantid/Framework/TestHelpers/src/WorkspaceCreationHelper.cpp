//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>

namespace WorkspaceCreationHelper
{

  using namespace Mantid;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;
  using Mantid::MantidVec;
  using Mantid::MantidVecPtr;

  Workspace1D_sptr Create1DWorkspaceRand(int size)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(size,1);
    y1.access().resize(size);
    std::generate(y1.access().begin(),y1.access().end(),rand);
    e1.access().resize(size);
    std::generate(e1.access().begin(),e1.access().end(),rand);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  Workspace1D_sptr Create1DWorkspaceConstant(int size, double value, double error)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(size,1);
    y1.access().resize(size);
    std::fill(y1.access().begin(), y1.access().end(), value);
    e1.access().resize(size);
    std::fill(y1.access().begin(), y1.access().end(), error);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  Workspace1D_sptr Create1DWorkspaceFib(int size)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(size,1);
    y1.access().resize(size);
    std::generate(y1.access().begin(),y1.access().end(),FibSeries<double>());
    e1.access().resize(size);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  Workspace2D_sptr Create2DWorkspace(int nhist, int numBoundaries)
  {
    return Create2DWorkspaceBinned(nhist, numBoundaries);
  }

  /** Create a Workspace2D where the Y value at each bin is
   * == to the workspace index
   * @param nhist :: # histograms
   * @param numBoundaries :: # of bins
   * @return Workspace2D
   */
  Workspace2D_sptr Create2DWorkspaceWhereYIsWorkspaceIndex(int nhist, int numBoundaries)
  {
    Workspace2D_sptr out = Create2DWorkspaceBinned(nhist, numBoundaries);
    for (int wi=0; wi < nhist; wi++)
      for (int x=0; x<numBoundaries; x++)
        out->dataY(wi)[x] = wi*1.0;
    return out;
  }



  Workspace2D_sptr Create2DWorkspace123(int64_t nHist, int64_t nBins,bool isHist,
					const std::set<int64_t> & maskedWorkspaceIndices)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(isHist?nBins+1:nBins,1);
    y1.access().resize(nBins,2);
    e1.access().resize(nBins,3);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(nHist,isHist?nBins+1:nBins,nBins);
    for (int i=0; i< nHist; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    retVal = maskSpectra(retVal, maskedWorkspaceIndices);

    return retVal;
  }

  Workspace2D_sptr Create2DWorkspace154(int64_t nHist, int64_t nBins,bool isHist,
					const std::set<int64_t> & maskedWorkspaceIndices)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(isHist?nBins+1:nBins,1);
    y1.access().resize(nBins,5);
    e1.access().resize(nBins,4);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(nHist,isHist?nBins+1:nBins,nBins);
    for (int i=0; i< nHist; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    retVal = maskSpectra(retVal, maskedWorkspaceIndices);

    return retVal;
  }

  Workspace2D_sptr maskSpectra(Workspace2D_sptr workspace, const std::set<int64_t> & maskedWorkspaceIndices)
  {
    // We need detectors to be able to mask them.
    workspace->setInstrument(boost::shared_ptr<Instrument>(new Instrument));
    boost::shared_ptr<Instrument> instrument = workspace->getBaseInstrument();

    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape +=	"<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" /> " ;
    xmlShape +=	"<radius val=\"0.05\" /> " ;
    xmlShape +=	"</sphere>";
    xmlShape +=	"<algebra val=\"shape\" /> ";  

    ShapeFactory sFactory;
    boost::shared_ptr<Object> shape = sFactory.createShape(xmlShape);

    const int nhist = static_cast<int>(workspace->getNumberHistograms());

    ParameterMap& pmap = workspace->instrumentParameters();
    for( int i = 0; i < nhist; ++i )
    {
      workspace->getAxis(1)->spectraNo(i) = i;
    }
    workspace->mutableSpectraMap().populateSimple(0, nhist);

    for( int i = 0; i < nhist; ++i )
    {
      Detector *det = new Detector("det",i,shape, NULL);
      det->setPos(i,i+1,1);
      instrument->add(det);
      instrument->markAsDetector(det);
      if ( maskedWorkspaceIndices.find(i) != maskedWorkspaceIndices.end() )
      {
        pmap.addBool(det,"masked",true);
      }
    }
    return workspace;
  }

  /** Create a 2D workspace with this many histograms and bins.
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins, double x0, double deltax)
  {
    MantidVecPtr x,y,e;
    x.access().resize(nbins+1);
    y.access().resize(nbins,2);
    e.access().resize(nbins,sqrt(2.0));
    for (int i =0; i < nbins+1; ++i)
    {
      x.access()[i] = x0+i*deltax;
    }
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(nhist,nbins+1,nbins);
    for (int i=0; i< nhist; i++)
    {
      retVal->setX(i,x);
      retVal->setData(i,y,e);
      retVal->getAxis(1)->setValue(i,i);
    }

    return retVal;
  }

  /** Create a 2D workspace with this many histograms and bins. The bins are assumed to be non-uniform and given by the input array
   * Filled with Y = 2.0 and E = sqrt(2.0)w
   */
  Workspace2D_sptr Create2DWorkspaceBinned(int nhist, const int numBoundaries, const double xBoundaries[])
  {
    MantidVecPtr x,y,e;
    const int numBins = numBoundaries - 1;
    x.access().resize(numBoundaries);
    y.access().resize(numBins,2);
    e.access().resize(numBins,sqrt(2.0));
    for (int i = 0; i < numBoundaries; ++i)
    {
      x.access()[i] = xBoundaries[i];
    }
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(nhist,numBins+1,numBins);
    for (int i=0; i< nhist; i++)
    {
      retVal->setX(i,x);
      retVal->setData(i,y,e);
    }

    return retVal;
  }


  //================================================================================================================
  /**
   * Create a test workspace with a fully defined instrument
   * Each spectra will have a cylindrical detector defined 2*cylinder_radius away from the centre of the
   * pervious. 
   * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0 
   */
  Workspace2D_sptr create2DWorkspaceWithFullInstrument(int nhist, int nbins, bool includeMonitors)
  {
    Workspace2D_sptr space = Create2DWorkspaceBinned(nhist, nbins);
    boost::shared_ptr<Instrument> testInst(new Instrument("testInst"));
    space->setInstrument(testInst);

    // Create detectors for each spectra and set a simple mapping between pixel ID = spectrum number = index
    const double pixelRadius(0.05);
    Object_sptr pixelShape = 
      ComponentCreationHelper::createCappedCylinder(pixelRadius, 0.02, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube"); 

    const double detXPos(5.0);
    int ndets = nhist;
    if( includeMonitors ) ndets -= 2;
    for( int i = 0; i < ndets; ++i )
    {
      std::ostringstream lexer;
      lexer << "pixel-" << i << ")";
      Detector * physicalPixel = new Detector(lexer.str(), i, pixelShape, testInst.get());
      const double ypos = i*2.0*pixelRadius;
      physicalPixel->setPos(detXPos, ypos,0.0);
      testInst->add(physicalPixel);
      testInst->markAsDetector(physicalPixel);
    }

    if( includeMonitors )
    {
      Detector *monitor1 = new Detector("mon1", ndets, Object_sptr(), testInst.get());
      monitor1->setPos(-9.0,0.0,0.0);
      monitor1->markAsMonitor();
      testInst->add(monitor1);
      testInst->markAsMonitor(monitor1);

      Detector *monitor2 = new Detector("mon2", ndets+1, Object_sptr(), testInst.get());
      monitor2->setPos(-2.0,0.0,0.0);
      monitor2->markAsMonitor();
      testInst->add(monitor2);
      testInst->markAsMonitor(monitor2);
      
      space->mutableSpectraMap().populateSimple(0, ndets+2);
    }
    else
    {
      space->mutableSpectraMap().populateSimple(0, ndets);
    }

    // Define a source and sample position
    //Define a source component
    ObjComponent *source = new ObjComponent("moderator", Object_sptr(), testInst.get());
    source->setPos(V3D(-20, 0.0, 0.0));
    testInst->add(source);
    testInst->markAsSource(source);

    // Define a sample as a simple sphere
    ObjComponent *sample = new ObjComponent("samplePos", Object_sptr(), testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);


    return space;
  }


  //================================================================================================================
  /** Create an Eventworkspace with an instrument that contains RectangularDetector's
   *
   * @param numBanks :: number of rectangular banks
   * @param numPixels :: each bank will be numPixels*numPixels
   * @return The EventWorkspace
   */
  Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument(int numBanks, int numPixels)
  {
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(numBanks, numPixels);
    EventWorkspace_sptr ws = CreateEventWorkspace2(numBanks*numPixels*numPixels, 10);
    ws->setInstrument(inst);
    ws->getAxis(0)->setUnit("dSpacing");
    int detID = numPixels*numPixels;
    for (int wi=0; wi< static_cast<int>(ws->getNumberHistograms()); wi++)
    {
      ws->getEventList(wi).clear(true);
      ws->getEventList(wi).addDetectorID(detID);
      detID++;
    }
    ws->doneAddingEventLists();
    // std::vector<int> dets = ws->getInstrument()->getDetectorIDs();  std::cout << dets.size() << " dets found " << dets[0] <<","<<dets[1]<<"\n";
    return ws;
  }


  //================================================================================================================
  WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value)
  {
    WorkspaceSingleValue_sptr retVal(new WorkspaceSingleValue(value,sqrt(value)));
    return retVal;
  }

  WorkspaceSingleValue_sptr CreateWorkspaceSingleValueWithError(double value, double error)
  {
    WorkspaceSingleValue_sptr retVal(new WorkspaceSingleValue(value, error));
    return retVal;
  }

  /** Perform some finalization on event workspace stuff */
  void EventWorkspace_Finalize(EventWorkspace_sptr ew)
  {
    // get a proton charge
    ew->mutableRun().integrateProtonCharge();
  }


  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  EventWorkspace_sptr CreateEventWorkspace()
  {
    return CreateEventWorkspace(500,1001,100,1000);
  }

  /** Create event workspace with:
   * 50 pixels
   * 100 histogrammed bins from 0.0 in steps of 1.0
   * 200 events; two in each bin, at time 0.5, 1.5, etc.
   * PulseTime = 0 second x2, 1 second x2, 2 seconds x2, etc. after 2010-01-01
   */
  EventWorkspace_sptr CreateEventWorkspace2(int numPixels, int numBins)
  {
    return CreateEventWorkspace(numPixels, numBins, 100, 0.0, 1.0, 2);
  }

  /** Create event workspace
   */
  EventWorkspace_sptr CreateEventWorkspace(int numPixels,
					   int numBins, int numEvents, double x0, double binDelta,
					   int eventPattern, int start_at_pixelID)
  {
    DateAndTime run_start("2010-01-01");

    //add one to the number of bins as this is histogram
    numBins++;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numPixels,1,1);

    //Make fake events
    if (eventPattern) // 0 == no events
    {
      for (int pix= start_at_pixelID+0; pix < start_at_pixelID+numPixels; pix++)
      {
        for (int i=0; i<numEvents; i++)
        {
          if (eventPattern == 1) // 0, 1 diagonal pattern
            retVal->getEventListAtPixelID(pix) += TofEvent((pix+i+0.5)*binDelta, run_start+double(i));
          else if (eventPattern == 2) // solid 2
          {
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
          }
          else if (eventPattern == 3) // solid 1
          {
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
          }
          else if (eventPattern == 4) // Number of events per bin = pixelId (aka workspace index in most cases)
          {
            retVal->getEventListAtPixelID(pix);
            for (int q=0; q<pix;q++)
              retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, run_start+double(i));
          }
        }
      }
    }
    retVal->doneLoadingData();

    //Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = x0+i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    return retVal;
  }


  // =====================================================================================
  /** Create event workspace, with several detector IDs in one event list.
     */
  EventWorkspace_sptr CreateGroupedEventWorkspace(std::vector< std::vector<int> > groups,
              int numBins, double binDelta)
  {

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(1,2,1);

    for (size_t g=0; g < groups.size(); g++)
    {
      std::vector<int> dets = groups[g];
      for (std::vector<int>::iterator it = dets.begin(); it != dets.end(); it++)
      {
        for (int i=0; i < numBins; i++)
          retVal->getOrAddEventList(g) += TofEvent((i+0.5)*binDelta, 1);
        retVal->getOrAddEventList(g).addDetectorID( *it );
      }
    }

    retVal->doneAddingEventLists();

    //Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    double x0=0;
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = x0+i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    return retVal;
  }


  // =====================================================================================
  /** Create an event workspace with randomized TOF and pulsetimes
   *
   * @param numbins :: # of bins to set. This is also = # of events per EventList
   * @param numpixels :: number of pixels
   * @return EventWorkspace
   */
  EventWorkspace_sptr CreateRandomEventWorkspace(size_t numbins, size_t numpixels, double bin_delta)
  {
    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numpixels,numbins,numbins-1);

    //Create the original X axis to histogram on.
    //Create the x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(numbins);
    for (int i = 0; i < static_cast<int>(numbins); ++i)
      xRef[i] = i*bin_delta;

    //Make up some data for each pixels
    for (size_t i=0; i< numpixels; i++)
    {
      //Create one event for each bin
      EventList& events = retVal->getEventListAtPixelID(static_cast<detid_t>(i));
      for (double ie=0; ie<numbins; ie++)
      {
        //Create a list of events, randomize
        events += TofEvent( std::rand() , std::rand());
      }
    }
    retVal->doneLoadingData();
    retVal->setAllX(axis);


    return retVal;
  }


  // =====================================================================================
  /** Create Workspace2d, with numHist spectra, each with 9 detectors,
   * with IDs 1-9, 10-18, 19-27
   */
  MatrixWorkspace_sptr CreateGroupedWorkspace2D(size_t numHist, int numBins, double binDelta)
  {

    Workspace2D_sptr retVal = Create2DWorkspaceBinned(static_cast<int>(numHist), numBins, 0.0, binDelta);
    retVal->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(static_cast<int>(numHist)) );

    // Set the detector IDs
    for (int g=0; g < static_cast<int>(numHist); g++)
    {
      std::vector<int> dets;
      for (int i=1; i<=9; i++)
        dets.push_back(g*9+i);
      retVal->mutableSpectraMap().addSpectrumEntries(g, dets);
    }

    return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);
  }

  MatrixWorkspace_sptr CreateGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist, int numBins, double binDelta)
  {
     size_t numHist=RootOfNumHist*RootOfNumHist;
     Workspace2D_sptr retVal = Create2DWorkspaceBinned(static_cast<int>(numHist), numBins, 0.0, binDelta);
     retVal->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(static_cast<int>(numHist)) );

    // Set the detector IDs
    for (int g=0; g < static_cast<int>(numHist); g++)
    {
      std::vector<int> dets;
      for (int i=1; i<=9; i++)
        dets.push_back(g*9+i);
      retVal->mutableSpectraMap().addSpectrumEntries(g, dets);
    }

    return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);

  }


  //not strictly creating a workspace, but really helpfull to see what one contains
  void DisplayDataY(const MatrixWorkspace_sptr ws)
  {
    const size_t numHists = ws->getNumberHistograms();
    for (size_t i = 0; i < numHists; ++i)
    {
      std::cout << "Histogram " << i << " = ";
      for (size_t j = 0; j < ws->blocksize(); ++j)
      {  
	std::cout <<ws->readY(i)[j]<<" ";
      }
      std::cout<<std::endl;
    }
  }
  void DisplayData(const MatrixWorkspace_sptr ws)
  {
    DisplayDataX(ws);
  }

  //not strictly creating a workspace, but really helpfull to see what one contains
  void DisplayDataX(const MatrixWorkspace_sptr ws)
  {
    const size_t numHists = ws->getNumberHistograms();
    for (size_t i = 0; i < numHists; ++i)
    {
      std::cout << "Histogram " << i << " = ";
      for (size_t j = 0; j < ws->blocksize(); ++j)
      {  
	std::cout <<ws->readX(i)[j]<<" ";
      }
      std::cout<<std::endl;
    }
  }

  //not strictly creating a workspace, but really helpfull to see what one contains
  void DisplayDataE(const MatrixWorkspace_sptr ws)
  {
    const size_t numHists = ws->getNumberHistograms();
    for (size_t i = 0; i < numHists; ++i)
    {
      std::cout << "Histogram " << i << " = ";
      for (size_t j = 0; j < ws->blocksize(); ++j)
      {  
	std::cout <<ws->readE(i)[j]<<" ";
      }
      std::cout<<std::endl;
    }
  }

}
