/*WIKI* 

Save a [[MDEventWorkspace]] to a .nxs file. The workspace's current box structure and entire list of events is preserved.
The resulting file can be loaded via [[LoadMD]].

If you specify MakeFileBacked, then this will turn an in-memory workspace to a file-backed one. Memory will be released as it is written to disk.

If you specify UpdateFileBackEnd, then any changes (e.g. events added using the PlusMD algorithm) will be saved to the file back-end.

*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/SaveMD.h"
#include "nexus/NeXusFile.hpp"
#include "MantidMDEvents/MDBox.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveMD)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveMD::SaveMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveMD::~SaveMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveMD::initDocs()
  {
    this->setWikiSummary("Save a MDEventWorkspace to a .nxs file.");
    this->setOptionalMessage("Save a MDEventWorkspace to a .nxs file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::OptionalSave, exts),
        "The name of the Nexus file to write, as a full or relative path.\n"
        "Optional if UpdateFileBackEnd is checked.");
    // Filename is NOT used if UpdateFileBackEnd
    setPropertySettings("Filename", new EnabledWhenProperty(this,"UpdateFileBackEnd", IS_EQUAL_TO, "0"));

    declareProperty("UpdateFileBackEnd", false,
        "Only for MDEventWorkspaces with a file back end: check this to update the NXS file on disk\n"
        "to reflect the current data structure. Filename parameter is ignored.");
    setPropertySettings("UpdateFileBackEnd", new EnabledWhenProperty(this,"MakeFileBacked", IS_EQUAL_TO, "0"));

    declareProperty("MakeFileBacked", false,
        "For an MDEventWorkspace that was created in memory:\n"
        "This saves it to a file AND makes the workspace into a file-backed one.");
    setPropertySettings("MakeFileBacked", new EnabledWhenProperty(this,"UpdateFileBackEnd", IS_EQUAL_TO, "0"));
  }

  //----------------------------------------------------------------------------------------------
  /** Save the MDEventWorskpace to a file.
   * Based on the Intermediate Data Format Detailed Design Document, v.1.R3 found in SVN.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void SaveMD::doSave(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::string filename = getPropertyValue("Filename");
    bool update = getProperty("UpdateFileBackEnd");
    bool MakeFileBacked = getProperty("MakeFileBacked");

    if (update && MakeFileBacked)
      throw std::invalid_argument("Please choose either UpdateFileBackEnd or MakeFileBacked, not both.");

    if (MakeFileBacked && ws->isFileBacked())
      throw std::invalid_argument("You picked MakeFileBacked but the workspace is already file-backed!");

    BoxController_sptr bc = ws->getBoxController();

    // Open/create the file
    ::NeXus::File * file;
    if (update)
    {
      progress(0.01, "Flushing Cache");
      // First, flush to disk. This writes all the event data to disk!
      bc->getDiskMRU().flushCache();

      // Use the open file
      file = bc->getFile();
      if (!file)
        throw std::invalid_argument("MDEventWorkspace is not file-backed. Do not check UpdateFileBackEnd!");

      // Normally the file is left open with the event data open, but in READ only mode.
      // Needs to be closed and reopened for things to work
      MDE::closeNexusData(file);
      file->close();
      // Reopen the file
      filename = bc->getFilename();
      file = new ::NeXus::File(filename, NXACC_RDWR);
    }
    else
    {
		// Erase the file if it exists
		Poco::File oldFile(filename);
		if (oldFile.exists())
			oldFile.remove();
      // Create a new file in HDF5 mode.
      file = new ::NeXus::File(filename, NXACC_CREATE5);
    }

    // The base entry. Named so as to distinguish from other workspace types.
    if (!update)
      file->makeGroup("MDEventWorkspace", "NXentry", 0);
    file->openGroup("MDEventWorkspace", "NXentry");

    // General information
    if (!update)
    {
      // Write out some general information like # of dimensions
      file->writeData("dimensions", int32_t(nd));
      file->putAttr("event_type", MDE::getTypeName());
    }

    // Save each NEW ExperimentInfo to a spot in the file
    std::map<std::string,std::string> entries;
    file->getEntries(entries);
    for (uint16_t i=0; i < ws->getNumExperimentInfo(); i++)
    {
      ExperimentInfo_sptr ei = ws->getExperimentInfo(i);
      std::string groupName = "experiment" + Strings::toString(i);
      if (entries.find(groupName) == entries.end())
      {
        // Can't overwrite entries. Just add the new ones
        file->makeGroup(groupName, "NXgroup", true);
        file->putAttr("version", 1);
        ei->saveExperimentInfoNexus(file);
        file->closeGroup();
      }
    }

    // Save some info as attributes. (Note: need to use attributes, not data sets because those cannot be resized).
    file->putAttr("definition",  ws->id());
    file->putAttr("title",  ws->getTitle() );
    // Save each dimension, as their XML representation
    for (size_t d=0; d<nd; d++)
    {
      std::ostringstream mess;
      mess << "dimension" << d;
      file->putAttr( mess.str(), ws->getDimension(d)->toXMLString() );
    }


    // Start the event Data group
    if (!update)
      file->makeGroup("event_data", "NXdata");
    file->openGroup("event_data", "NXdata");
    file->putAttr("version", "1.0");

    // Prepare the data chunk storage.
    size_t chunkSize = 10000; // TODO: Determine a smart chunk size!
    if (!update)
    {
      MDE::prepareNexusData(file, chunkSize);
      // Initialize the file-backing
      if (MakeFileBacked)
        bc->setFile(file, filename, 0);
    }
    else
    {
      uint64_t totalNumEvents = MDE::openNexusData(file);
      // Set it back to the new file handle
      bc->setFile(file, filename, totalNumEvents);
    }

    size_t maxBoxes = bc->getMaxId();

    // Prepare the vectors we will fill with data.

    // Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> box_type(maxBoxes, 0);
    // Recursion depth
    std::vector<int> depth(maxBoxes, -1);
    // Start/end indices into the list of events
    std::vector<uint64_t> box_event_index(maxBoxes*2, 0);
    // Min/Max extents in each dimension
    std::vector<double> extents(maxBoxes*nd*2, 0);
    // Inverse of the volume of the cell
    std::vector<double> inverse_volume(maxBoxes, 0);
    // Box cached signal/error squared
    std::vector<double> box_signal_errorsquared(maxBoxes*2, 0);
    // Start/end children IDs
    std::vector<int> box_children(maxBoxes*2, 0);

    // The slab start for events, start at 0
    uint64_t start = 0;

    // Get a starting iterator
    MDBoxIterator<MDE,nd> it(ws->getBox(), 1000, false);

    Progress * prog = new Progress(this, 0.05, 0.9, maxBoxes);

    IMDBox<MDE,nd> * box;
    while (true)
    {
      box = it.getBox();
      size_t id = box->getId();
      if (id < maxBoxes)
      {
        // The start/end children IDs
        size_t numChildren = box->getNumChildren();
        if (numChildren > 0)
        {
          // Make sure that all children are ordered. TODO: This might not be needed if the IDs are rigorously done
          size_t lastId = box->getChild(0)->getId();
          for (size_t i = 1; i < numChildren; i++)
          {
            if (box->getChild(i)->getId() != lastId+1)
              throw std::runtime_error("Non-sequential child ID encountered!");
            lastId = box->getChild(i)->getId();
          }

          box_children[id*2] = int(box->getChild(0)->getId());
          box_children[id*2+1] = int(box->getChild(numChildren-1)->getId());
          box_type[id] = 2;
        }
        else
          box_type[id] = 1;


        MDBox<MDE,nd> * mdbox = dynamic_cast<MDBox<MDE,nd> *>(box);
        if (mdbox)
        {
          if (update)
          {
            // File-backed: re-save any boxes THAT WERE MODIFED
            // This will relocate and save the box if it has any events
            mdbox->save();
            // We've now forced it to go on disk
            mdbox->setOnDisk(true);
            // Save the index
            box_event_index[id*2] = mdbox->getFileIndexStart();
            box_event_index[id*2+1] = mdbox->getFileNumEvents();
//            file->closeData();
//            file->openData("event_data");
            //std::cout << file->getInfo().dims[0] << " size of event_data (updating) \n";
          }
          else
          {
            // Save for the first time
            const std::vector<MDE> & events = mdbox->getConstEvents();
            uint64_t numEvents = uint64_t(events.size());
            if (numEvents > 0)
            {
              mdbox->setFileIndex(uint64_t(start), numEvents);
              // Just save but don't clear the events or anything
              mdbox->saveNexus(file);
              if (MakeFileBacked)
              {
                // Save, set that it is on disk and clear the actual events to free up memory
                mdbox->setOnDisk(true);
                mdbox->clearDataOnly();
//                mdbox->setDataAdded(false);
//                mdbox->setDataModified(false);
              }
              // Save the index
              box_event_index[id*2] = start;
              box_event_index[id*2+1] = numEvents;
              // Move forward in the file.
              start += numEvents;
            }

            //std::cout << file->getInfo().dims[0] << " size of event_data (writing) \n";
            mdbox->releaseEvents();
          }
        }

        // Various bits of data about the box
        depth[id] = int(box->getDepth());
        box_signal_errorsquared[id*2] = double(box->getSignal());
        box_signal_errorsquared[id*2+1] = double(box->getErrorSquared());
        inverse_volume[id] = box->getInverseVolume();
        for (size_t d=0; d<nd; d++)
        {
          size_t newIndex = id*(nd*2) + d*2;
          extents[newIndex] = box->getExtents(d).min;
          extents[newIndex+1] = box->getExtents(d).max;
        }

        // Move on to the next box
        prog->report("Saving Box");
        if (!it.next()) break;
      }
      else
      {
        // Some sort of problem
        g_log.warning() << "Unexpected box ID found (" << id << ") which is > than maxBoxes (" << maxBoxes << ")" << std::endl;
        break;
      }
    }

    // Done writing the event data.
    MDE::closeNexusData(file);

    // ------------------------- Save Free Blocks --------------------------------------------------
    // Get a vector of the free space blocks to save to the file
    std::vector<uint64_t> freeSpaceBlocks;
    bc->getDiskMRU().getFreeSpaceVector(freeSpaceBlocks);
    if (freeSpaceBlocks.size() == 0)
      freeSpaceBlocks.resize(2, 0); // Needs a minimum size
    std::vector<int64_t> free_dims(2,2); free_dims[0] = int64_t(freeSpaceBlocks.size()/2);
    std::vector<int64_t> free_chunk(2,2); free_chunk[0] = 1000;

    // Now the free space blocks under event_data
    if (!update)
      file->writeExtendibleData("free_space_blocks", freeSpaceBlocks, free_dims, free_chunk);
    else
      file->writeUpdatedData("free_space_blocks", freeSpaceBlocks, free_dims);
    file->closeGroup();


    // -------------- Save Box Structure  -------------------------------------
    // OK, we've filled these big arrays of data. Save them.
    progress(0.91, "Writing Box Data");
    prog->resetNumSteps(8, 0.92, 1.00);

    // Start the box data group
    if (!update)
      file->makeGroup("box_structure", "NXdata");
    file->openGroup("box_structure", "NXdata");
    file->putAttr("version", "1.0");

    // Add box controller info to this group
    file->putAttr("box_controller_xml", bc->toXMLString());

    std::vector<int64_t> exents_dims(2,0);
    exents_dims[0] = (int64_t(maxBoxes));
    exents_dims[1] = (nd*2);
    std::vector<int64_t> exents_chunk(2,0);
    exents_chunk[0] = int64_t(16384);
    exents_chunk[1] = (nd*2);

    std::vector<int64_t> box_2_dims(2,0);
    box_2_dims[0] = int64_t(maxBoxes);
    box_2_dims[1] = (2);
    std::vector<int64_t> box_2_chunk(2,0);
    box_2_chunk[0] = int64_t(16384);
    box_2_chunk[1] = (2);

    if (!update)
    {
      // Write it for the first time
      file->writeExtendibleData("box_type", box_type);
      file->writeExtendibleData("depth", depth);
      file->writeExtendibleData("inverse_volume", inverse_volume);
      file->writeExtendibleData("extents", extents, exents_dims, exents_chunk);
      file->writeExtendibleData("box_children", box_children, box_2_dims, box_2_chunk);
      file->writeExtendibleData("box_signal_errorsquared", box_signal_errorsquared, box_2_dims, box_2_chunk);
      file->writeExtendibleData("box_event_index", box_event_index, box_2_dims, box_2_chunk);
    }
    else
    {
      // Update the extendible data sets
      file->writeUpdatedData("box_type", box_type);
      file->writeUpdatedData("depth", depth);
      file->writeUpdatedData("inverse_volume", inverse_volume);
      file->writeUpdatedData("extents", extents, exents_dims);
      file->writeUpdatedData("box_children", box_children, box_2_dims);
      file->writeUpdatedData("box_signal_errorsquared", box_signal_errorsquared, box_2_dims);
      file->writeUpdatedData("box_event_index", box_event_index, box_2_dims);
    }

    // Finished - close the file. This ensures everything gets written out even when updating.
    file->close();

    if (update || MakeFileBacked)
    {
      //std::cout << "Finished updating file " << uint64_t(bc->getFile()) << std::endl;
      // Need to keep the file open since it is still used as a back end.
      // Reopen the file
      filename = bc->getFilename();
      file = new ::NeXus::File(filename, NXACC_RDWR);
      // Re-open the data for events.
      file->openGroup("MDEventWorkspace", "NXentry");
      file->openGroup("event_data", "NXdata");
      uint64_t totalNumEvents = MDE::openNexusData(file);
      std::cout << totalNumEvents << " events in reopened file \n";
      bc->setFile(file, filename, totalNumEvents);
      // Mark file is up-to-date
      ws->setFileNeedsUpdating(false);

    }

    delete prog;

  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveMD::exec()
  {
    IMDEventWorkspace_sptr ws = getProperty("InputWorkspace");

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->doSave, ws);
  }



} // namespace Mantid
} // namespace MDEvents

