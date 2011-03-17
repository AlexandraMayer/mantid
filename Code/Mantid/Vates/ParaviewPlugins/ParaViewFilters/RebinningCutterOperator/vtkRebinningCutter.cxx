#include "vtkRebinningCutter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImplicitFunction.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h"
#include "MantidVatesAPI/vtkProxyFactory.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "boost/functional/hash.hpp"
#include <sstream>
#include "ParaViewProgressAction.h"
#include <time.h>

/** Plugin for ParaView. Performs simultaneous rebinning and slicing of Mantid data.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

vtkCxxRevisionMacro(vtkRebinningCutter, "$Revision: 1.0 $")
;
vtkStandardNewMacro(vtkRebinningCutter)
;

using namespace Mantid::VATES;

vtkRebinningCutter::vtkRebinningCutter() :
  m_presenter(),
  m_clipFunction(NULL),
  m_cachedVTKDataSet(NULL),
  m_isSetup(false),
  m_timestep(0),
  m_thresholdMax(10000),
  m_thresholdMin(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkRebinningCutter::~vtkRebinningCutter()
{
}

std::string vtkRebinningCutter::createRedrawHash() const
{
  size_t seed = 1;
  using namespace boost;
  hash_combine(seed, m_thresholdMax);
  hash_combine(seed, m_thresholdMin);
  //TODO add other properties that should force redraw only when changed.
  std::stringstream sstream;
  sstream << seed;
  return sstream.str();
}

RebinningIterationAction vtkRebinningCutter::decideIterationAction(const int timestep)
{
  RebinningIterationAction action;
  if (NULL == m_cachedVTKDataSet || m_Rebin)
  {
    action = RecalculateAll;
  }
  else
  {
    if ((timestep != m_timestep) || m_cachedRedrawArguments != createRedrawHash())
    {
      action = RecalculateVisualDataSetOnly;
    }
    else
    {
      action = UseCache;
    }
  }
  return action;
}

int vtkRebinningCutter::RequestData(vtkInformation *request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  using namespace Mantid::Geometry;
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;
  using Mantid::VATES::Dimension_sptr;
  using Mantid::VATES::DimensionVec;

  vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
  vtkDataSet * inputDataset = vtkDataSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));

  DimensionVec dimensionsVec(4);
  dimensionsVec[0] = m_appliedXDimension;
  dimensionsVec[1] = m_appliedYDimension;
  dimensionsVec[2] = m_appliedZDimension;
  dimensionsVec[3] = m_appliedTDimension;

  //Create the composite holder.
  CompositeImplicitFunction* compFunction = new CompositeImplicitFunction;
  compFunction->addFunction(constructBox(m_appliedXDimension, m_appliedYDimension, m_appliedZDimension));

  // Construct reduction knowledge.
  m_presenter.constructReductionKnowledge(
      dimensionsVec,
      m_appliedXDimension,
      m_appliedYDimension,
      m_appliedZDimension,
      m_appliedTDimension,
      compFunction,
      inputDataset);

  ParaViewProgressAction updatehandler(this);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));

  //Acutally perform rebinning or specified action.
  int timestep = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    timestep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
  }
  RebinningIterationAction action = decideIterationAction(timestep);

  MDWorkspace_sptr spRebinnedWs = m_presenter.applyRebinningAction(action, updatehandler);

  vtkUnstructuredGrid* outData;
  if (UseCache == action)
  {
    //Use existing vtkDataSet
    vtkDataSetFactory_sptr spvtkDataSetFactory(new vtkProxyFactory(m_cachedVTKDataSet));
    outData = dynamic_cast<vtkUnstructuredGrid*> (m_presenter.createVisualDataSet(spvtkDataSetFactory));
    outData->Register(NULL);
  }
  else
  {
    //Build a vtkDataSet
    vtkDataSetFactory_sptr spvtkDataSetFactory = createDataSetFactory(spRebinnedWs);
    outData = dynamic_cast<vtkUnstructuredGrid*> (m_presenter.createVisualDataSet(spvtkDataSetFactory));
    m_cachedVTKDataSet = outData;
  }

  m_timestep = timestep; //Not settable directly via a setter.
  m_cachedRedrawArguments = createRedrawHash();
  m_Rebin = false;// So that we can compare again.

  output->ShallowCopy(outData);
  return 1;
}

void vtkRebinningCutter::UpdateAlgorithmProgress(int progressPercent)
{
  this->UpdateProgress(progressPercent);
  this->SetProgressText("Executing Mantid Rebinning Algorithm...");
}

int vtkRebinningCutter::RequestInformation(vtkInformation *request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  if (!m_isSetup)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;
    using Mantid::VATES::Dimension_sptr;
    using Mantid::VATES::DimensionVec;

    vtkInformation * inputInf = inputVector[0]->GetInformationObject(0);
    vtkDataSet * inputDataset = vtkDataSet::SafeDownCast(inputInf->Get(vtkDataObject::DATA_OBJECT()));

    DimensionVec dimensionsVec(4);
    dimensionsVec[0] = m_presenter.getXDimensionFromDS(inputDataset);
    dimensionsVec[1] = m_presenter.getYDimensionFromDS(inputDataset);
    dimensionsVec[2] = m_presenter.getZDimensionFromDS(inputDataset);
    dimensionsVec[3] = m_presenter.getTDimensionFromDS(inputDataset);

    m_appliedXDimension = dimensionsVec[0];
    m_appliedYDimension = dimensionsVec[1];
    m_appliedZDimension = dimensionsVec[2];
    m_appliedTDimension = dimensionsVec[3];

    // Construct reduction knowledge.
    m_presenter.constructReductionKnowledge(dimensionsVec, dimensionsVec[0], dimensionsVec[1],
        dimensionsVec[2], dimensionsVec[3], inputDataset);

    this->Modified();
    m_isSetup = true;
  }
  return 1;
}

int vtkRebinningCutter::RequestUpdateExtent(vtkInformation* info, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  return 1;
}
;

int vtkRebinningCutter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkRebinningCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkRebinningCutter::SetClipFunction(vtkImplicitFunction * func)
{
  if (func != m_clipFunction)
  {
    this->Modified();
    this->m_clipFunction = func;
  }
}

void vtkRebinningCutter::SetMaxThreshold(double maxThreshold)
{
  if (maxThreshold != m_thresholdMax)
  {
    this->Modified();
    this->m_thresholdMax = maxThreshold;
  }
}

void vtkRebinningCutter::SetMinThreshold(double minThreshold)
{
  if (minThreshold != m_thresholdMin)
  {
    this->Modified();
    this->m_thresholdMin = minThreshold;
  }
}

void vtkRebinningCutter::SetAppliedXDimensionXML(std::string xml)
{
  if (NULL != m_appliedXDimension.get())
  {
    if (m_appliedXDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      this->m_appliedXDimension = Mantid::VATES::createDimension(xml);
      this->m_Rebin = true;
    }
  }
}

void vtkRebinningCutter::SetAppliedYDimensionXML(std::string xml)
{
  if (NULL != m_appliedYDimension.get())
  {
    if (m_appliedYDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      this->m_appliedYDimension = Mantid::VATES::createDimension(xml);
      this->m_Rebin = true;
    }
  }
}

void vtkRebinningCutter::SetAppliedZDimensionXML(std::string xml)
{
  if (NULL != m_appliedZDimension.get())
  {
    if (m_appliedZDimension->toXMLString() != xml && !xml.empty())
    {
      this->Modified();
      this->m_appliedZDimension = Mantid::VATES::createDimension(xml);
      this->m_Rebin = true;
    }
  }
}

void vtkRebinningCutter::SetAppliedTDimensionXML(std::string xml)
{
  this->Modified();
  //TODO
}

const char* vtkRebinningCutter::GetInputGeometryXML()
{
  return this->m_presenter.getWorkspaceGeometry().c_str();
}

unsigned long vtkRebinningCutter::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->m_clipFunction != NULL)
  {
    time = this->m_clipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionX(vtkDataSet* in_ds) const
{
  return m_presenter.getXDimensionFromDS(in_ds);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionY(vtkDataSet* in_ds) const
{
  return m_presenter.getYDimensionFromDS(in_ds);
  //return Mantid::VATES::createDimension(m_presenter.getYDimensionFromDS(in_ds)->toXMLString(), 100);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensionZ(vtkDataSet* in_ds) const
{
  return m_presenter.getZDimensionFromDS(in_ds);
}

Mantid::VATES::Dimension_sptr vtkRebinningCutter::getDimensiont(vtkDataSet* in_ds) const
{
  return m_presenter.getTDimensionFromDS(in_ds);
}

boost::shared_ptr<Mantid::API::ImplicitFunction> vtkRebinningCutter::constructBox(Dimension_sptr spDimX,
    Dimension_sptr spDimY, Dimension_sptr spDimZ) const
{
  using namespace Mantid::MDAlgorithms;
  //Have to use dimension knowledge to construct box.
  double originX = (spDimX->getMaximum() + spDimX->getMinimum()) / 2;
  double originY = (spDimY->getMaximum() + spDimY->getMinimum()) / 2;
  double originZ = (spDimZ->getMaximum() + spDimZ->getMinimum()) / 2;
  double width = spDimX->getMaximum() - spDimX->getMinimum();
  double height = spDimY->getMaximum() - spDimY->getMinimum();
  double depth = spDimZ->getMaximum() - spDimZ->getMinimum();

  //Create domain parameters.
  OriginParameter originParam = OriginParameter(originX, originY, originZ);
  WidthParameter widthParam = WidthParameter(width);
  HeightParameter heightParam = HeightParameter(height);
  DepthParameter depthParam = DepthParameter(depth);

  //Create the box. This is specific to this type of presenter and this type of filter. Other rebinning filters may use planes etc.
  BoxImplicitFunction* boxFunction = new BoxImplicitFunction(widthParam, heightParam, depthParam,
      originParam);

  return boost::shared_ptr<Mantid::API::ImplicitFunction>(boxFunction);
}

vtkDataSetFactory_sptr vtkRebinningCutter::createDataSetFactory(
    Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const
{

  using Mantid::MDDataObjects::MDImage;

  //Get the time dimension
  boost::shared_ptr<const Mantid::Geometry::IMDDimension> timeDimension = spRebinnedWs->gettDimension();

  //Create a mapper to transform real time into steps.
  TimeToTimeStep timeMapper(timeDimension->getMinimum(), timeDimension->getMaximum(),
      timeDimension->getNBins());

  //Create a factory for generating a thresholding unstructured grid.
  vtkDataSetFactory* pvtkDataSetFactory = new vtkThresholdingUnstructuredGridFactory<MDImage,
      TimeToTimeStep> (spRebinnedWs->get_spMDImage(), XMLDefinitions::signalName(), m_timestep,
      timeMapper, m_thresholdMin, m_thresholdMax);

  //Return the generated factory.
  return vtkDataSetFactory_sptr(pvtkDataSetFactory);
}

