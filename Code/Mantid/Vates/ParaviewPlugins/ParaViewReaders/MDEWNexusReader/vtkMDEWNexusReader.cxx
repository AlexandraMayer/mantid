#include "vtkMDEWNexusReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPVClipDataSet.h"
#include "vtkBox.h"

#include "MantidVatesAPI/vtkMDEWHexahedronFactory.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

vtkCxxRevisionMacro(vtkMDEWNexusReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkMDEWNexusReader);

using namespace Mantid::VATES;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension_sptr;


vtkMDEWNexusReader::vtkMDEWNexusReader() : 
  m_presenter(NULL),
  m_loadInMemory(false)
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

}

vtkMDEWNexusReader::~vtkMDEWNexusReader()
{
  delete m_presenter;
  this->SetFileName(0);
}

void vtkMDEWNexusReader::SetDepth(int depth)
{
  size_t temp = depth;
  if(m_depth != temp)
  {
   this->m_depth = temp;
   this->Modified();
  }
}

size_t vtkMDEWNexusReader::getRecursionDepth() const
{
  return this->m_depth;
}

bool vtkMDEWNexusReader::getLoadInMemory() const
{
  return m_loadInMemory;
}

double vtkMDEWNexusReader::getTime() const
{
  return m_time;
}

/**
  Sets algorithm in-memory property. If this is changed, the file is reloaded.
  @param inMemory : true if the entire file should be loaded into memory.
*/
void vtkMDEWNexusReader::SetInMemory(bool inMemory)
{
  if(m_loadInMemory != inMemory)
  {
    this->Modified(); 
  }
  m_loadInMemory = inMemory;
}


/**
  Gets the geometry xml from the workspace. Allows object panels to configure themeselves.
  @return geometry xml const * char reference.
*/
const char* vtkMDEWNexusReader::GetInputGeometryXML()
{
  if(m_presenter == NULL)
  {
    return "";
  }
  try
  {
    return m_presenter->getGeometryXML().c_str();
  }
  catch(std::runtime_error&)
  {
    return "";
  }
}

int vtkMDEWNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{

  using namespace Mantid::VATES;
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    m_time =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
  }

  FilterUpdateProgressAction<vtkMDEWNexusReader> updateHandler(this);
  vtkMDEWHexahedronFactory* hexahedronFactory = new vtkMDEWHexahedronFactory(ThresholdRange_scptr(new IgnoreZerosThresholdRange()), "signal");
  hexahedronFactory->setTime(m_time);
  vtkDataSet* product = m_presenter->execute(hexahedronFactory, updateHandler);
  
  //-------------------------------------------------------- Corrects problem whereby boundaries not set propertly in PV.
  vtkBox* box = vtkBox::New();
  box->SetBounds(product->GetBounds());
  vtkPVClipDataSet* clipper = vtkPVClipDataSet::New();
  clipper->SetInput(product);
  clipper->SetClipFunction(box);
  clipper->SetInsideOut(true);
  clipper->Update();
  vtkDataSet* clipperOutput = clipper->GetOutput();
   //--------------------------------------------------------

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->ShallowCopy(clipperOutput);

  clipper->Delete();
  
  return 1;
}

int vtkMDEWNexusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if(m_presenter == NULL)
  {
    m_presenter = new MDEWEventNexusLoadingPresenter<vtkMDEWNexusReader>(this, FileName);
    m_presenter->executeLoadMetadata();
    setTimeRange(outputVector);
  }
  return 1;
}

void vtkMDEWNexusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkMDEWNexusReader::CanReadFile(const char* fname)
{
  MDEWEventNexusLoadingPresenter<vtkMDEWNexusReader> temp(this, fname);
  return temp.canReadFile();
}

unsigned long vtkMDEWNexusReader::GetMTime()
{
  return Superclass::GetMTime();
}

/**
  Update/Set the progress.
  @parameter progress : progress increment.
*/
void vtkMDEWNexusReader::updateAlgorithmProgress(double progress)
{
  progressMutex.lock();
  this->SetProgressText("Loading MDEW file...");
  this->UpdateProgress(progress);
  progressMutex.unlock();
}

/** Helper function to setup the time range.
@param outputVector : vector onto which the time range will be set.
*/
void vtkMDEWNexusReader::setTimeRange(vtkInformationVector* outputVector)
{
  if(m_presenter->hasTDimensionAvailable())
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    std::vector<double> timeStepValues = m_presenter->getTimeStepValues();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0],
      static_cast<int> (timeStepValues.size()));
    double timeRange[2];
    timeRange[0] = timeStepValues.front();
    timeRange[1] = timeStepValues.back();

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
}