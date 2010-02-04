//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FlatPlateAbsorption.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatPlateAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

FlatPlateAbsorption::FlatPlateAbsorption() : AbsorptionCorrection(),
  m_slabHeight(0.0), m_slabWidth(0.0), m_slabThickness(0.0)
{
}

void FlatPlateAbsorption::defineProperties()
{
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
  mustBePositive->setLower(0.0);
  declareProperty("SampleHeight", -1.0, mustBePositive, "The height of the plate in cm");
  declareProperty("SampleWidth", -1.0, mustBePositive->clone(), "The width of the plate in cm");
  declareProperty("SampleThickness", -1.0, mustBePositive->clone(), "The thickness of the plate in cm");
  
  BoundedValidator<double> *moreThanZero = new BoundedValidator<double> ();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, 
    "The size of one side of an integration element cube in mm");
}

/// Fetch the properties and set the appropriate member variables
void FlatPlateAbsorption::retrieveProperties()
{
  m_slabHeight = getProperty("SampleHeight"); // in cm
  m_slabWidth = getProperty("SampleWidth"); // in cm
  m_slabThickness = getProperty("SampleThickness"); // in cm
  m_slabHeight *= 0.01; // now in m
  m_slabWidth *= 0.01; // now in m
  m_slabThickness *= 0.01; // now in m
  
  double cubeSide = getProperty("ElementSize"); // in mm
  cubeSide *= 0.001; // now in m
  m_numXSlices = static_cast<int>(m_slabWidth/cubeSide);
  m_numYSlices = static_cast<int>(m_slabHeight/cubeSide);
  m_numZSlices = static_cast<int>(m_slabThickness/cubeSide);
  m_XSliceThickness = m_slabWidth/m_numXSlices;
  m_YSliceThickness = m_slabHeight/m_numYSlices;
  m_ZSliceThickness = m_slabThickness/m_numZSlices;

  m_numVolumeElements = m_numXSlices * m_numYSlices * m_numZSlices;
  m_sampleVolume = m_slabHeight * m_slabWidth * m_slabThickness;
}

std::string FlatPlateAbsorption::sampleXML()
{
  double szX = m_slabWidth/2, szY = m_slabHeight/2, szZ = m_slabThickness/2;
  std::ostringstream xmlShapeStream;
  xmlShapeStream 
      << " <cuboid id=\"sample-shape\"> " 
      << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> " 
      << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> " 
      << "<left-back-bottom-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> " 
      << "<right-front-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> " 
      << "</cuboid>";
  
  return xmlShapeStream.str();
}

/// Calculate the distances for L1 and element size for each element in the sample
void FlatPlateAbsorption::initialiseCachedDistances()
{
  m_L1s.resize(m_numVolumeElements);
  m_elementVolumes.resize(m_numVolumeElements);
  m_elementPositions.resize(m_numVolumeElements);
  
  int counter = 0;
  
  for (int i = 0; i < m_numZSlices; ++i)
  {
    const double z = (i + 0.5) * m_ZSliceThickness - 0.5 * m_slabThickness;
    
    for (int j = 0; j < m_numYSlices; ++j)
    {
      const double y = (j + 0.5) * m_YSliceThickness - 0.5 * m_slabHeight;
      
      for (int k = 0; k < m_numXSlices; ++k)
      {
        const double x = (k + 0.5) * m_XSliceThickness - 0.5 * m_slabWidth;
        // Set the current position in the sample in Cartesian coordinates.
        m_elementPositions[counter](x,y,z);
        assert(m_sampleObject->isValid(m_elementPositions[counter]));
        // Create track for distance in sample before scattering point
        // Remember beam along Z direction
        Track incoming(m_elementPositions[counter], V3D(0.0, 0.0, -1.0));
        m_sampleObject->interceptSurface(incoming);
        m_L1s[counter] = incoming.begin()->Dist;

        // Also calculate element volume here
        m_elementVolumes[counter] = m_XSliceThickness*m_YSliceThickness*m_ZSliceThickness;

        counter++;
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
