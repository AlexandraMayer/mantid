#include "MantidQtCustomInterfaces/WorkspaceOnDisk.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
      /**
      Constructor
      @param fileName : path + name of the file to load
      */
      WorkspaceOnDisk::WorkspaceOnDisk(std::string fileName) : m_fileName(fileName)
      {
        boost::regex pattern("(RAW)$", boost::regex_constants::icase); 

        if(!boost::regex_search(fileName, pattern))
        {
          std::string msg = "WorkspaceOnDisk:: Unknown File extension on: " + fileName;
          throw std::invalid_argument(msg);
        }

        if(!checkStillThere())
        {
          throw std::runtime_error("WorkspaceOnDisk:: File doesn't exist");
        }

        std::vector<std::string> strs;
        boost::split(strs, m_fileName, boost::is_any_of("/,\\"));
        m_adsID = strs.back();
        m_adsID = m_adsID.substr(0, m_adsID.find('.'));

        //Generate an initial report.
        Mantid::API::MatrixWorkspace_sptr ws = fetchIt();
        if(ws->mutableSample().hasOrientedLattice())
        {
          std::vector<double> ub = ws->mutableSample().getOrientedLattice().getUB().get_vector();
          this->setUB(ub[0], ub[1], ub[2], ub[3], ub[4], ub[5], ub[6], ub[7], ub[8]);
        }
        cleanUp();
      }

      /**
      Getter for the id of the workspace
      @return the id of the workspace
      */
      std::string WorkspaceOnDisk::getId() const
      {
        return m_fileName;
      }

      /**
      Getter for the type of location where the workspace is stored
      @ return the location type
      */
      std::string WorkspaceOnDisk::locationType() const
      {
        return locType();
      }

      /**
      Check that the workspace has not been deleted since instantiating this memento
      @return true if still in specified location
      */
      bool WorkspaceOnDisk::checkStillThere() const
      {
        std::ifstream ifile;
        ifile.open(m_fileName.c_str(), std::ifstream::in);
        return !ifile.fail();
      }

      /**
      Getter for the workspace itself
      @returns the matrix workspace
      @throw if workspace has been moved since instantiation.
      */
      Mantid::API::MatrixWorkspace_sptr WorkspaceOnDisk::fetchIt() const
      {
        using namespace Mantid::API;

        checkStillThere();

        if(!AnalysisDataService::Instance().doesExist(m_adsID))
        {
          IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadRaw");
          alg->initialize();
          alg->setRethrows(true);
          alg->setProperty("Filename", m_fileName);
          alg->setPropertyValue("OutputWorkspace", m_adsID);
          alg->execute();
        }
        Workspace_sptr ws = AnalysisDataService::Instance().retrieve(m_adsID);

        Mantid::API::WorkspaceGroup_sptr gws = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
        if(gws != NULL)
        {
          throw std::invalid_argument("This raw file corresponds to a WorkspaceGroup. Cannot process groups like this. Import via MantidPlot instead.");
        }
        return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      }

      /**
      Dump the workspace out of memory:
      @name : name of the workspace to clean-out.
      */
      void WorkspaceOnDisk::dumpIt(const std::string& name)
      {
        using Mantid::API::AnalysisDataService;
        if(AnalysisDataService::Instance().doesExist(name))
        {
          AnalysisDataService::Instance().remove(name);
        }
      }

      /// Destructor
      WorkspaceOnDisk::~WorkspaceOnDisk()
      {
      }

      /// Clean up.
      void WorkspaceOnDisk::cleanUp()
      {
          dumpIt(m_adsID);
      }

      /*
      Apply actions. Load workspace and apply all actions to it.
      */
      void WorkspaceOnDisk::applyActions()
      {
        Mantid::API::MatrixWorkspace_sptr ws = fetchIt();
        
        Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SetUB");
        alg->initialize();
        alg->setRethrows(true);
        alg->setPropertyValue("Workspace", this->m_adsID);
        alg->setProperty("UB", m_ub);
        alg->execute();

      }

  }
}