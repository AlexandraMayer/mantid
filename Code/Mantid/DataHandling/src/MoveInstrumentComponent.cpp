//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ParametrizedComponent.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MoveInstrumentComponent)

using namespace Kernel;
using namespace Geometry;
using namespace API;

Logger& MoveInstrumentComponent::g_log = Logger::get("MoveInstrumentComponent");

/// Empty default constructor
MoveInstrumentComponent::MoveInstrumentComponent()
{}

/// Initialisation method.
void MoveInstrumentComponent::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut));
  declareProperty("ComponentName","");
  declareProperty("DetectorID",-1);
  declareProperty("X",0.0);
  declareProperty("Y",0.0);
  declareProperty("Z",0.0);
  declareProperty("RelativePosition",true);
}

/** Executes the algorithm. 
 * 
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void MoveInstrumentComponent::exec()
{
  // Get the workspace
  MatrixWorkspace_sptr WS = getProperty("Workspace");
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const bool RelativePosition = getProperty("RelativePosition");

  boost::shared_ptr<IInstrument> inst = WS->getInstrument();
  boost::shared_ptr<IComponent> comp;

  // Find the component to move
  if (DetID != -1)
  {
      comp = findByID(inst,DetID);
      if (comp == 0)
      {
          std::ostringstream mess;
          mess<<"Detector with ID "<<DetID<<" was not found.";
          g_log.error(mess.str());
          throw std::runtime_error(mess.str());
      }
  }
  else if (!ComponentName.empty())
  {
      comp = findByName(inst,ComponentName);
      if (comp == 0)
      {
          std::ostringstream mess;
          mess<<"Component with name "<<ComponentName<<" was not found.";
          g_log.error(mess.str());
          throw std::runtime_error(mess.str());
      }
  }
  else
  {
      g_log.error("DetectorID or ComponentName must be given.");
      throw std::invalid_argument("DetectorID or ComponentName must be given.");
  }

  V3D Pos;// New relative position
  // First set it to the new absolute position
  if (RelativePosition)
  {
      Pos = comp->getPos() + V3D(X,Y,Z);
  }
  else
  {
      Pos = V3D(X,Y,Z);
  }

  // Then find the corresponding relative position
  const IComponent* parent = comp->getParent();
  if (parent)
  {
      Pos -= parent->getPos();
      Quat rot = parent->getRelativeRot();
      rot.inverse();
      rot.rotate(Pos);
  }

  //Need to get the address to the base instrument component
  boost::shared_ptr<Geometry::ParameterMap> pmap = WS->InstrumentParameters();
  ParametrizedComponent* pcomp = dynamic_cast<ParametrizedComponent*>(comp.get());
  const IComponent* baseComp;
  if (pcomp)
  {
      baseComp = pcomp->base();
  }
  else
  {
      baseComp = comp.get();
  }

  // Set "pos" instrument parameter. 
  Parameter_sptr par = pmap->get(baseComp,"pos");
  if (par) par->set(Pos);
  else
      pmap->addV3D(baseComp,"pos",Pos);

  return;
}

boost::shared_ptr<IComponent> MoveInstrumentComponent::findByID(boost::shared_ptr<IComponent> comp,int id)
{
    boost::shared_ptr<IDetector> det = boost::dynamic_pointer_cast<IDetector>(comp);
    if (det && det->getID() == id) return comp;
    boost::shared_ptr<ICompAssembly> asmb = boost::dynamic_pointer_cast<ICompAssembly>(comp);
    if (asmb)
        for(int i=0;i<asmb->nelements();i++)
        {
            boost::shared_ptr<IComponent> res = findByID((*asmb)[i],id);
            if (res) return res;
        }
    return boost::shared_ptr<IComponent>();
}

boost::shared_ptr<IComponent> MoveInstrumentComponent::findByName(boost::shared_ptr<IComponent> comp,const std::string& CName)
{
    if (comp->getName() == CName) return comp;
    boost::shared_ptr<ICompAssembly> asmb = boost::dynamic_pointer_cast<ICompAssembly>(comp);
    if (asmb)
        for(int i=0;i<asmb->nelements();i++)
        {
            boost::shared_ptr<IComponent> res = findByName((*asmb)[i],CName);
            if (res) return res;
        }
    return boost::shared_ptr<IComponent>();
}

} // namespace DataHandling
} // namespace Mantid
