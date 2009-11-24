#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include "LoadRaw/isisraw2.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadDetectorInfo)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

const float LoadDetectorInfo::UNSETOFFSET = float(-1e12);
/// Empty default constructor
LoadDetectorInfo::LoadDetectorInfo() : Algorithm(),
  m_workspace(), m_instrument(), m_paraMap(NULL), m_numHists(-1), m_monitors(),
  m_monitorXs(), m_commonXs(false), m_monitOffset(UNSETOFFSET), m_error(false),
  m_FracCompl(0.0)
{
}

void LoadDetectorInfo::init()
{
  // Declare required input parameters for algorithm
  CompositeValidator<Workspace2D> *val = new CompositeValidator<Workspace2D>;
  val->add(new WorkspaceUnitValidator<Workspace2D>("TOF"));
  val->add(new HistogramValidator<Workspace2D>);
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","",Direction::InOut,val),
    "The name of the workspace to that the detector information will be loaded into" );
  std::vector<std::string> exts;
  // each of these allowed extensions must be dealt with in exec() below
  exts.push_back("dat");
  exts.push_back("raw");
  exts.push_back("sca");
  declareProperty(new FileProperty("DataFilename","", FileProperty::Load, exts),
    "A .DAT or .raw file that contains information about the detectors in the\n"
    "workspace. Partial pressures of 3He will be loaded assuming units of\n"
    "atmospheres, offset times in the same units as the workspace X-values and\n"
    "and wall thicknesses in metres.");
}

/** Executes the algorithm
*  @throw invalid_argument if the detection delay time is different for different monitors
*  @throw FileError if there was a problem opening the file or its format
*  @throw MisMatch<int> if not very spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*  @throw runtime_error if the SpectraDetectorMap had not been filled
*/
void LoadDetectorInfo::exec()
{
  // get the infomation that will be need from the user selected workspace, assume it exsists because of the validator in init()
  m_workspace = getProperty("Workspace");
  m_instrument = m_workspace->getInstrument();
  m_paraMap = &(m_workspace->instrumentParameters());
  m_numHists = m_workspace->getNumberHistograms();
  // when we change the X-values will care take to maintain sharing. I have only implemented maintaining sharing where _all_ the arrays are initiall common
  m_commonXs = WorkspaceHelpers::sharedXData(m_workspace);
  // set the other member variables to their defaults
  m_FracCompl = 0.0;
  m_monitors.clear();
  m_monitOffset = UNSETOFFSET;
  m_error = false;

  // get the user selected filename
  std::string filename = getPropertyValue("DataFilename");
  // load the data from the file using the correct algorithm depending on the assumed type of file
  if ( filename.find(".dat") == filename.size()-4 ||
    filename.find(".DAT") == filename.size()-4 )
  {
    readDAT(filename);
  }
  if ( filename.find(".sca") == filename.size()-4 ||
    filename.find(".SCA") == filename.size()-4 )
  {
    readDAT(filename);
  }
  
  if ( filename.find(".raw") == filename.size()-4 ||
    filename.find(".RAW") == filename.size()-4)
  {
    readRAW(filename);
  }

  if (m_error)
  {
    g_log.warning() << "Note workspace " << getPropertyValue("Workspace") << " has been changed so if you intend to fix detector mismatch problems by running " << name() << " on this workspace again is likely to corrupt it" << std::endl;
  }
}

/** Reads detector information from a .dat file, the file contains one line per detector
*  and its format is documented in "DETECTOR.DAT format" (more details in LoadDetectorInfo.h)
*  @param fName name and path of the data file to read
*  @throw invalid_argument if the detection delay time is different for different monitors
*  @throw FileError if there was a problem opening the file or its format
*  @throw MisMatch<int> if not very spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*  @throw runtime_error if the SpectraDetectorMap had not been filled
*/
void LoadDetectorInfo::readDAT(const std::string& fName)
{
  g_log.information() << "Opening DAT file " << fName << std::endl;
  std::ifstream sFile(fName.c_str());
  if (!sFile)
  {
    throw Exception::FileError("Can't open file", fName);
  }
  // update the progress monitor and allow for user cancel
  progress(0.05);
  interruption_point();
 
  std::string str;
  // skip header line which contains something like <filename> generated by <prog>
  getline(sFile,str);
  g_log.debug() << "Reading " << fName << std::endl;
  
  int detectorCount;
  getline(sFile,str);
  std::istringstream header2(str);
  // there are two numbers on this detector count line, we want the first
  header2 >> detectorCount;
  if(detectorCount<1)
  {
    g_log.error("Bad detector count in data file");
    throw Exception::FileError("Problem reading the detector count on the second line of the data file", fName);
  }
 
  // skip title line
  getline(sFile,str);

  // will store all hte detector IDs that we get data for
  std::vector<int> detectorList;
  detectorList.reserve(detectorCount);
  // stores the time offsets that the TOF X-values will be adjusted by at the end
  std::vector<float> offsets;
  offsets.reserve(detectorCount);
  float detectorOffset = UNSETOFFSET;
  bool differentOffsets = false;
  // used only for progress and error reporting
  std::vector<int> missingDetectors;
  int count = 0, detectorProblemCount = 0;
  // Now loop through lines, one for each detector or monitor. The latter are ignored.
  while( getline(sFile, str) )
  {
    if (str.empty() || str[0] == '#')
    {// comments and empty lines are allowed and ignored
      continue;
    }
    std::istringstream istr(str);
    
    // columns in the file, the detector ID and a code for the type of detector CODE = 3 (psd gas tube)
    int det_no, code;
    float delta;
    // column names det_2   ,   det_3  , assumes that code=3
    double         pressure, wallThick;
    // data for each detector in the file that we don't need
    float dump;
    // parse the line
    istr >> det_no >> delta >> dump >> code;
    detectorList.push_back(det_no);
    offsets.push_back(delta);

    if ( code != PSD_GAS_TUBE && code != MONITOR_DEVICE )
    {// the type of detector is wrong, we can't use the information
      if (code != DUMMY_DECT)
      {//we can't use data for detectors with other codes because it could be in the wrong format, ignore the data and write to g_log.warning() once at the end
        detectorProblemCount ++;
        g_log.debug() << "Ignoring data for a detector with code " << code << std::endl;
      }
      // dummy or not skip the code below and move on to the next detector
      continue;
    }
    if ( code == MONITOR_DEVICE )
    {// throws invalid_argument if the detection delay time is different for different monitors
      noteMonitorOffset( delta, det_no);
      // skip the rest of this loop and move on to the next detector
      continue;
    }
    // PSD gas tube specific code now until the end of the for block
    
    // normally all the offsets are the same and things work faster, check for this
    if ( delta != detectorOffset )
    {// could mean different detectors have different offsets and we need to do things thoroughly
      if ( detectorOffset !=  UNSETOFFSET )
      {
        differentOffsets = true;
      }
      detectorOffset = delta;
    }

    //there are 12 uninteresting columns
    istr >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump;
    // read more data, leaving the last number at the end of the line
    istr >> pressure >> wallThick;
    try
    {
      setDetectorParams(det_no, pressure, wallThick);
    }
    catch (Exception::NotFoundError)
    {// there are likely to be some detectors that we can't find in the instrument definition and we can't save parameters for these. We can't do anything about this just report the problem at the end
      missingDetectors.push_back(det_no);
      continue;
    }

    // report progress and check for a user cancel message at regualar intervals
    count ++;
    if ( count % INTERVAL == INTERVAL/2 )
    {
      progress(static_cast<double>(count)/(3*detectorCount));
      interruption_point();
    }
  }
  
  g_log.information() << "Adjusting time of flight X-values by detector delay times" << std::endl;
  adjDelayTOFs(detectorOffset, differentOffsets, detectorList, offsets);
  
  if ( detectorProblemCount > 0 )
  {
    g_log.warning() << "Data for " << detectorProblemCount << " detectors that are neither monitors or psd gas tubes, the data have been ignored" << std::endl;
  }
  logErrorsFromRead(missingDetectors);
  g_log.debug() << "Successfully read DAT file " << fName << std::endl;
}

/** Reads data about the detectors from the header section of a RAW file it
* relies on the user table being in the correct format
* @param fName path to the RAW file to read
* @throw FileError if there is a problem opening the file or the header is in the wrong format
* @throw invalid_argument if the detection delay time is different for different monitors
* @throw MisMatch<int> if not very spectra is associated with exaltly one detector
* @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
* @throw runtime_error if the SpectraDetectorMap had not been filled
*/
void LoadDetectorInfo::readRAW(const std::string& fName)
{
  g_log.information() << "Opening RAW file " << fName << std::endl;
  // open raw file
  ISISRAW2 iraw;
  if (iraw.readFromFile(fName.c_str(),false) != 0)
  {
    g_log.error("Unable to open file " + fName);
    throw Exception::FileError("Unable to open File:" , fName);
  }
  // update the progress monitor and allow for user cancel
  progress(0.05);
  interruption_point();
  g_log.debug() << "Reading file " << fName << std::endl;

  // the number of detectors according to the raw file header
  const int numDets = iraw.i_det;
  // check the number of user tables in the raw file where we will read pressures and wall thinknesses for each detector
  if ( iraw.i_use != OUR_TOTAL_NUM_TAB )
  {
    g_log.warning() << "The user table has " << iraw.i_use << " entries expecting " << OUR_TOTAL_NUM_TAB << ". The workspace has not been altered" << std::endl;
    g_log.debug() << "This algorithm reads some data in from the user table. The data in the user table can vary between RAW files and we use the total number of user table entries and the code field as checks that we have the correct format" << std::endl;
    throw Exception::FileError("Detector gas pressure or wall thickness information is missing in the RAW file header or is in the wrong format", fName);
  }

  int detectorProblemCount = 0;
  std::vector<int> missingDetectors;
  // the process will run a lot more quickly if all the detectors have the same offset time, monitors can have a different offset but it is an error if the offset for two monitors is different
  float detectorOffset =  UNSETOFFSET;
  bool differentOffsets = false;
  for (int i = 0; i < numDets; ++i)
  {
    // this code tells us what the numbers in the user table (iraw.ut), which we are about to use, mean
    const int format = iraw.code[i];
    if ( format != OUR_USER_TABLE_FORM )
    {// the data is not in the format that we expect for a PSD detector
      if ( format == USER_TABLE_MONITOR )
      {// throws invalid_argument if the detection delay time is different for different monitors
        noteMonitorOffset(iraw.delt[i], iraw.udet[i]);
      }
      else
      {// the format of the data for this detector may be wrong, ignore the data and write to g_log.warning() once at the end
        detectorProblemCount ++;
        g_log.debug() << "Ignoring RAW file header user table information that has code " << format << std::endl;
      }
      continue;
    }
    // PSD gas tube specific code now until the end of the for block

    // iraw.delt contains the all the detector offset times in the same order as the detector IDs in iraw.udet
    if ( iraw.delt[i] != detectorOffset )
    {// could mean different detectors have different offsets and we need to do things thoroughly
      if ( detectorOffset !=  UNSETOFFSET )
      {
        differentOffsets = true;
      }
      detectorOffset = iraw.delt[i];
    }

    try
    {// iraw.udet contains the detector IDs and the other parameters are stored in order in the user table array (ut)
      setDetectorParams(iraw.udet[i],
                        iraw.ut[i+PRESSURE_TAB_NUM*numDets],
                        iraw.ut[i+WALL_THICK_TAB_NUM*numDets]);
    }
    catch (Exception::NotFoundError)
    {// there are likely to be some detectors that we can't find in the instrument definition and we can't save parameters for these. We can't do anything about this just report the problem at the end
      missingDetectors.push_back(iraw.udet[i]);
      continue;
    }

    // report progress and check for a user cancel message sometimes
    if ( i % INTERVAL == INTERVAL/2 )
    {
      progress( static_cast<double>(i)/(3*numDets) );
      interruption_point();
    }
  }

  g_log.information() << "Adjusting time of flight X-values by detector delay times" << std::endl;
  adjDelayTOFs(detectorOffset, differentOffsets, iraw.udet, iraw.delt,numDets);

  if ( detectorProblemCount > 0 )
  {
    g_log.warning() << detectorProblemCount << " entries in the user table had the wrong format, these data have been ignored and some detectors parameters were not updated" << std::endl;
  }
  logErrorsFromRead(missingDetectors);
  g_log.debug() << "Successfully read RAW file " << fName << std::endl;
}
/** Creates or modifies the parameter map for the specified detector adding
*  pressure and wall thickness information
*  @param detID the ID of detector that you want to write the parameters for
*  @param pressure the value to set the 3He partial pressure
*  @param wallThick the wall thickness of the detector in metres
*  @throw NotFoundError if a pointer to the specified detector couldn't be retrieved
*/
void LoadDetectorInfo::setDetectorParams(int detID, double pressure, double wallThick)
{
  IComponent* baseComp = NULL;
  try
  {// there are often exceptions caused by detectors not being listed in the instrument file
    Geometry::IDetector_sptr det = m_instrument->getDetector(detID);
    baseComp = det->getComponent();
  }
  catch( std::runtime_error &e)
  {// when detector information is incomplete there are a range of exceptions you can get, I think none of which should kill the algorithm
    throw Exception::NotFoundError(e.what(), detID);
  }

  // set the detectors pressure, first check if it already has a setting, if not add it
  Parameter_sptr setting = m_paraMap->get(baseComp, "3He(atm)");
  if (setting) setting->set(pressure);
  else
    m_paraMap->add("double", baseComp, "3He(atm)", pressure);

  setting = m_paraMap->get(baseComp, "wallT(m)");
  if (setting) setting->set(wallThick);
  else
    m_paraMap->add("double", baseComp, "wallT(m)", wallThick);
}
/** Decides if the bin boundaries for all non-monitor spectra will be the same and runs the appropiate
*  function. It is possible for this function to set all detectors spectra X-values when they shouldn't
*  @param lastOffset the delay time of the last detector is only used if differentDelays is false or if detectID and delays are leave empty e.g. when we use common bins
*  @param differentDelays if the number of adjustments is greater than or equal to the number of spectra and this is false then common bins will be used
*  @param detectIDs the list of detector IDs needs to corrospond to the next argument, list of delays
*  @param delays ommitting or passing an empty list of delay times forces common bins to be used, any delays need to be in the same order as detectIDs
*/
void LoadDetectorInfo::adjDelayTOFs(double lastOffset, bool &differentDelays, const std::vector<int> &detectIDs, const std::vector<float> &delays)
{
  // a spectra wont be adjusted if their detector wasn't included in the input file. So for differentDelays to false there need to be at least as many detectors in the data file as in the workspace
  differentDelays = differentDelays || ( delays.size() < m_numHists );
  // if we don't have a list of delays then we have no choice
  differentDelays = delays.size() > 0 && differentDelays;
  // see if adjusting the TOF Xbin boundaries requires knowledge of individual detectors or if they are all the same
  if ( differentDelays )
  {// not all the detectors have the same offset, do the offsetting thoroughly
    g_log.debug() << "Adjusting all the X-values by their offset times, which varied depending on the detector. The offset time of the last detector is " << lastOffset << " microseconds" << std::endl;
    adjustXs(detectIDs, delays);
  }
  else
  {// all the detectors have the same offset _much_ easier to do
    g_log.debug() << "Adjusting all the X-values by the constant offset time " << lastOffset << " microseconds" << std::endl;
    adjustXs(lastOffset);
  }
}
/** Decides if the bin boundaries for all non-monitor spectra will be the same and runs the appropiate
*  function. It is possible for this function to set all detectors spectra X-values when they shouldn't
*  @param lastOffset the delay time of the last detector is only used if differentDelays is false, e.g. we use common bins
*  @param differentDelays if the number of adjustments is greater than or equal to the number of spectra and this is false then common bins will be used
*  @param detectIDs the list of detector IDs is required regardless of how differentDelays is needs to be in the same order as delays
*  @param delays required regardless of if differentDelays is true or false and needs to be in the same order as detectIDs
*  @param numDetectors the size of the arrays pointed to by delays and detectIDs
*/void LoadDetectorInfo::adjDelayTOFs(double lastOffset, bool &differentDelays, const int * const detectIDs, const float * const delays, int numDetectors)
{
  // a spectra wont be adjusted if their detector wasn't included in the RAW file. So for differentDelays to false there need to be at least as many detectors in the data file as in the workspace 
  differentDelays = differentDelays || ( numDetectors < m_numHists );
  if ( differentDelays )
  {
    const std::vector<int> detectorList(detectIDs, detectIDs + numDetectors);
    const std::vector<float> offsets(delays, delays + numDetectors);
    adjDelayTOFs(lastOffset, differentDelays, detectorList, offsets);
  }
  else
  {
    adjDelayTOFs(lastOffset, differentDelays);
  }
}
/** This function finds the spectra associated with each detector passed ID and subtracts
*  the corrosponding value in the offsets array from all bin boundaries in that spectrum. If
*  all the X-values share the same storage array then some sharing is maintained
*  @param detIDs ID list of IDs numbers of all the detectors with offsets
*  @param offsets an array of values to change the bin boundaries by, these must be listed in the same order as detIDs
*  @throw MisMatch<int> if not every spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*  @throw runtime_error if the SpectraDetectorMap had not been filled
*/
void LoadDetectorInfo::adjustXs(const std::vector<int> &detIDs, const std::vector<float> &offsets)
{
  // getting spectra numbers from detector IDs is hard because the map works the other way, getting index numbers from spectra numbers has the same problem and we are about to do both
  // function adds zero if it can't find a detector into the new, probably large, multimap of detectorIDs to spectra numbers
  std::vector<int> spectraList =
    m_workspace->spectraMap().getSpectra(detIDs);

  // allow spectra number to spectra index look ups
  std::map<int,int> specs2index;
  m_workspace->getAxis(1)->getSpectraIndexMap(specs2index);

  if ( spectraList.size() != detIDs.size() )
  {// this shouldn't really happen but would cause a crash if it weren't handled ...
    g_log.debug() << "Couldn't associate some detectors or monitors to spectra, are there some spectra missing?" << std::endl;
    throw Exception::MisMatch<int>(spectraList.size(), detIDs.size(), "Couldn't associate some detectors or monitors to spectra, are there some spectra missing?");
  }
  //used for logging
  std::vector<int> missingDetectors;

  if ( m_commonXs )
  {// we can be memory efficient and only write a new set of bins when the offset has changed
    adjustXsCommon(offsets, spectraList, specs2index, missingDetectors);
  }//end shared X-values arrays
  else
  {// simplist case to code, adjust the bins in each spectrum
    adjustXsUnCommon(offsets, spectraList, specs2index, missingDetectors);
  }
  if ( missingDetectors.size() > 0 )
  {
    g_log.warning() << "The following detector IDs were read in the input file but aren't associated with any spectra: " << detIDs[missingDetectors[0]] << std::endl;
    for ( std::vector<int>::size_type i = 0; i < missingDetectors.size(); ++i )
    {
      g_log.warning() << ", " << detIDs[missingDetectors[i]] << std::endl;
    }
    g_log.warning() << "Data listed for those detectors was ignored" << std::endl;
  }
}
/** Substracts the given off value from all the bin boundaries in all the spectra. If the arrays
*  containing the X-values are all shared then they remain shared 
*  @param detectorOffset the value to remove from the bin boundaries
*  @throw  IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace 
*/
void LoadDetectorInfo::adjustXs(const double detectorOffset)
{
  Histogram1D::RCtype monitorXs;
  // for g_log keep a count of the number of spectra that we can't find detectors for in the raw file
  int spuriousSpectra = 0;
  double fracCompl = 1.0/3.0;

  Histogram1D::RCtype newXs;

  for ( int specInd = 0; specInd < m_numHists; ++specInd )
  {// check if we dealing with a monitor as these are dealt by a different function
    const int specNum = m_workspace->getAxis(1)->spectraNo(specInd);
    const std::vector<int> dets =
      m_workspace->spectraMap().getDetectors(specNum);
    if ( dets.size() > 0 ) 
    {// is it in the monitors list
      if ( m_monitors.find(dets[0]) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector 
        if ( (*newXs).empty() )
        {// we don't have any cached values from doing the offsetting previously, do the calculation
          if ( m_commonXs )
          {// common Xs means we only need to go through and change the bin boundaries once, we then copy this data
            // this must be first non-monitor spectrum has been found, this will be used as the base for all others
            setUpXArray(newXs, specInd, detectorOffset);
          }
          else// no common bins
          {// move the bin boundaries each time for each array
            MantidVec &Xbins = m_workspace->dataX(specInd);
            std::transform(Xbins.begin(), Xbins.end(), Xbins.begin(),
              std::bind2nd(std::minus<double>(), detectorOffset) );
          }
        }
        else// we have cached values in newXs
        {// this copies a pointer so that the histogram looks for it data in the correct cow_ptr
          m_workspace->setX(specInd, newXs);
        }
      }
      else// ( m_monitors.find(dets[0]) != m_monitors.end() )
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {// we have no cached values
          // negative because we add the monitor offset, not take away as for detectors
          setUpXArray(monitorXs, specInd, -m_monitOffset);
        }// this algorthim requires that all monitors have the same offset and so we can always use cached values
        else m_workspace->setX(specInd, monitorXs);
      }
    }
    else
    { //  The detector is not in the instrument definition file
      // we don't have any information on the spectrum and so we can't correct it
      for ( MantidVec::size_type j = 0; j < m_workspace->dataY(specInd).size(); j++ )
      {//as this algorithm is about obtaining corrected data I'll mark this this uncorrectable data as bad by setting it to zero
        m_workspace->dataY(specInd)[j] = 0;
        m_workspace->dataE(specInd)[j] = 0;
      }
      // as this stuff happens a lot don't write much to high log levels but do a full log to debug
      spuriousSpectra ++;
      if (spuriousSpectra == 1) g_log.debug() << "Missing detector information cause the following spectra to be set to zero, suspect missing detectors in instrument definition : " << specInd ;
      else g_log.debug() << "," << specInd;
    }
    if ( specInd % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*INTERVAL/3.0)/m_numHists;
      progress( fracCompl );
      interruption_point();
    }
  }//move on to the next histogram

  if (spuriousSpectra > 0)
  {
    g_log.debug() << std::endl;
    g_log.information() << "Found " << spuriousSpectra << " spectra without associated detectors, probably the detectors are not present in the instrument definition and this is not serious. The Y and error values for those spectra have be set to zero" << std::endl;
  }
}
/** A memory efficient function that adjusts the X-value bin boundaries that only creates a new
*  cow_ptr array when the offset has changed
* @param offsets an array of times to adjust all the bins in each workspace histogram by
* @param spectraList a list of spectra numbers in the same order as the offsets
* @param specs2index a map that allows finding a spectra indexes from spectra numbers
* @param missingDetectors this will be filled with the array indices of the detector offsets that we can't find spectra indices for
*/
void LoadDetectorInfo::adjustXsCommon(const std::vector<float> &offsets, const std::vector<int> &spectraList, std::map<int,int> &specs2index, std::vector<int> missingDetectors)
{
  // space for cached values
  float cachedOffSet = UNSETOFFSET;
  Histogram1D::RCtype monitorXs;
  Histogram1D::RCtype cachedXs;

  double fracCompl = 1.0/3.0;
  
  for ( std::vector<int>::size_type j = 0; j < spectraList.size(); ++j )
  {// first check that our spectranumber to spectra index map is working for us
    if ( specs2index.find(spectraList[j]) == specs2index.end() )
    {// we can't find the spectrum associated the detector prepare to log that
      missingDetectors.push_back(j);
      // and then move on to the next detector in the loop
      continue;
    }
    const int specIndex = specs2index[spectraList[j]];
    // check if we dealing with a monitor as these are dealt by a different function
    const std::vector<int> dets =
      m_workspace->spectraMap().getDetectors(spectraList[j]);
    if ( dets.size() > 0 )
    {// is it in the monitors list
      if ( m_monitors.find(dets[0]) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector
        if ( offsets[j] != cachedOffSet )
        {
          setUpXArray(cachedXs, specIndex, offsets[j]);
          cachedOffSet = offsets[j];
        }
        else m_workspace->setX(specIndex, cachedXs);
      }
      else
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {
          // negative because we add the monitor offset, not take away as for detectors, the difference between the monitor delay and the detectors that counts
          setUpXArray(monitorXs, specIndex, -m_monitOffset);
        }
        else m_workspace->setX(specIndex, monitorXs);
      }
    }
    if ( j % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*INTERVAL/3.0)/spectraList.size();
      progress( fracCompl );
      interruption_point();
    }
  }
}
/** Adjusts the X-value bin boundaries given offsets and makes no assumptions about
* that the histograms have shared bins or the time offsets are the same
* @param offsets an array of times to adjust all the bins in each workspace histogram by
* @param spectraList a list of spectra numbers in the same order as the offsets
* @param specs2index a map that allows finding a spectra indexes from spectra numbers
* @param missingDetectors this will be filled with the array indices of the detector offsets that we can't find spectra indices for
*/
void LoadDetectorInfo::adjustXsUnCommon(const std::vector<float> &offsets, const std::vector<int> &spectraList, std::map<int,int> &specs2index, std::vector<int> missingDetectors)
{// the monitors can't have diferent offsets so I can cache the bin boundaries for all the monitors
  Histogram1D::RCtype monitorXs;

  double fracCompl = 1.0/3.0;

  for ( std::vector<int>::size_type j = 0; j < spectraList.size(); ++j )
  {// first check that our spectranumber to spectra index map is working for us
    if ( specs2index.find(spectraList[j]) == specs2index.end() )
    {// we can't find the spectrum associated the detector prepare to log that
      missingDetectors.push_back(j);
      // and then move on to the next detector in the loop
      continue;
    }
    const int specIndex = specs2index[spectraList[j]];
    // check if we dealing with a monitor as these are dealt by a different function
    const std::vector<int> dets =
      m_workspace->spectraMap().getDetectors(spectraList[j]);
    if ( dets.size() > 0 ) 
    {// is it in the monitors list
      if ( m_monitors.find(dets[0]) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector
        MantidVec &Xbins = m_workspace->dataX(specIndex);
        std::transform(Xbins.begin(), Xbins.end(), Xbins.begin(),
          std::bind2nd(std::minus<double>(), offsets[j]) );
      }
      else
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {
          // negative because we add the monitor offset, not take away as for detectors, the difference between the monitor delay and the detectors is the quanity we are after
          setUpXArray(monitorXs, specIndex, -m_monitOffset);
        }
        else m_workspace->setX(specIndex, monitorXs);
      }
    }
    if ( j % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*INTERVAL/3.0)/spectraList.size();
      progress( fracCompl );
      interruption_point();
    }
  }
}
/**Changes the TOF (X values) by the offset time monitors but it chacks that first that
*  the monitor offset is non-zero. Throws if not all MonitorOffsets are the same
*  @param offSet the next offset time for a detector that was read in
*  @param detID the the monitor's detector ID number
*  @throws invalid_argument if it finds a monitor that has a different offset from the rest
*/
void LoadDetectorInfo::noteMonitorOffset(const float offSet, const int detID)
{
  // this algorithm assumes monitors have the same offset (it saves looking for the "master" or "time zero" monitor). So the first time this function is called we accept any offset, on subsequent calls we check
  if ( offSet != m_monitOffset && m_monitOffset != UNSETOFFSET )
  {// this isn't the first monitor we've found so we can check it has the same offset as the previous ones
    g_log.error() << "Found one monitor with an offset time of " << m_monitOffset << " and another with an offset of " << offSet << std::endl;
    throw std::invalid_argument("All monitors must have the same offset");
  }
  m_monitors.insert(detID);
  // this line will only change the value of m_monitOffset the first time, after that it's redundant
  m_monitOffset = offSet;
}

/** Modify X-values from the workspace and store them in the shared array
* containted within the cow pointer
* @param theXValuesArray this will contain the new values in it's array
* @param specInd index number of histogram from with to take the original X-values 
* @param offset _subtract_ this number from all the X-values
*/
void LoadDetectorInfo::setUpXArray(Histogram1D::RCtype &theXValuesArray, int specInd, double offset)
{
  std::vector<double> &AllXbins = theXValuesArray.access();
  AllXbins.resize(m_workspace->dataX(specInd).size());
  std::transform(
    m_workspace->readX(specInd).begin(), m_workspace->readX(specInd).end(),
    AllXbins.begin(),
    std::bind2nd(std::minus<double>(), offset) );
  m_workspace->setX(specInd, theXValuesArray);
}
/** Reports information on detectors that we couldn't get a pointer to, if any of these errors occured
*  @param missingDetectors detector IDs of detectors that we couldn't get a pointer to
*/
void LoadDetectorInfo::logErrorsFromRead(const std::vector<int> &missingDetectors)
{
  if ( missingDetectors.size() > 0 )
  {
    g_log.warning() << "Entries exist in the input file for " << missingDetectors.size() << " detectors that could not be accessed, data ignored. Probably the detectors are not present in the instrument definition" << std::endl;
    g_log.information() << "The detector IDs are: " << missingDetectors[0];
    for ( std::vector<int>::size_type k = 1; k < missingDetectors.size(); k++ )
    {
      g_log.information() << ", " << missingDetectors[k];
    }
    g_log.information() << std::endl;
    m_error = true;
  }
}

} // namespace DataHandling
} // namespace Mantid
