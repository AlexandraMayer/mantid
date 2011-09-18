#ifndef MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
#define MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
/*WIKI* 


This algorithm performs a Monte Carlo simulation to calculate the attenuation factor of a given sample for an arbitrary arrangement of sample + container shapes. The algorithm proceeds as follows for each spectra in turn:
* A random point within the sample+container set up is selected and chosen as a scattering point; 
* This point then defines an incoming vector from the source position and an outgoing vector to the final detector;
* The total attenuation factor for this path is then calculated as the product of the factor for each defined material of the sample/container that the track passes through.

== Known limitations ==
* Only elastic scattering is implemented at the moment.

* The object's bounding box is used as a starting point for selecting a random point. If the shape of the object is peculiar enough not to occupy much of the bounding box then the generation of the initial scatter point will fail.
 

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.

At a minimum, the input workspace must have a sample shape defined.


*WIKI*/

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/RandomNumberGenerator.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid
{
  namespace Geometry
  {
    class IDetector;
    class Object;
  }

  namespace Algorithms
  {
    /** 
      Calculates attenuation due to absorption and scattering in a sample +
      its environment using a weighted Monte Carlo.
 
      @author Martyn Gigg, Tessella plc
      @date 22/11/2010

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
    class DLLExport MonteCarloAbsorption : public API::Algorithm
    {
      
    public:
      /// Constructor
      MonteCarloAbsorption();
      /// Destructor
      ~MonteCarloAbsorption();

      /// Algorithm's name
      virtual const std::string name() const { return "MonteCarloAbsorption"; }
      /// Algorithm's version
      virtual int version() const { return 1; }
      /// Algorithm's category
      virtual const std::string category() const { return "Absorption Corrections"; }

    private:
      /// Initialize the algorithm
      void init();
      /// Execution code
      void exec();
      
      /// Do the simulation for the given detector and wavelength
      void doSimulation(const Geometry::IDetector * const detector, 
			const double lambda, double & attenFactor, double & error);
      /// Randomly select the location initial point within the beam from a square
      /// distribution
      Kernel::V3D sampleBeamProfile() const;
      /// Select a random location within the sample + container environment
      Kernel::V3D selectScatterPoint() const;
      /// Calculate the attenuation factor for the given single scatter setup
      double attenuationFactor(const Kernel::V3D & startPos, 
			       const Kernel::V3D & scatterPoint,
			       const Kernel::V3D & finalPos, const double lambda);
      /// Calculate the attenuation for a given length, material and wavelength
      double attenuation(const double length, const Geometry::Material& material,
			 const double lambda) const;

      /// Check the input and throw if invalid
      void retrieveInput();
      /// Initialise the caches used here including setting up the random 
      /// number generator
      void initCaches();

    private:
      /// The input workspace
      API::MatrixWorkspace_sptr m_inputWS;
      /// The sample's shape
      const Geometry::Object *m_sampleShape;
      /// The sample's material 
      const Geometry::Material *m_sampleMaterial;
      /// The container(s)
      const API::SampleEnvironment *m_container;
      /// The number of wavelength points to use for each simulation
      int m_numberOfPoints;
      /// The step size
      int m_xStepSize;
      /// The number of events per point
      int m_numberOfEvents;
      
      /** @name Cached values */
      //@{
      /// The sample position
      Kernel::V3D m_samplePos;
      /// The source position
      Kernel::V3D m_sourcePos;
      /// Bounding box length
      double m_bbox_length;
      /// Half the bounding box length
      double m_bbox_halflength;
      /// Bounding box width
      double m_bbox_width;
      /// Half the bounding box width
      double m_bbox_halfwidth;
      /// Bounding box height
      double m_bbox_height;
      /// Bounding box height
      double m_bbox_halfheight;
      //@}      
      
      /// A pointer to the random number generator
      Kernel::RandomNumberGenerator *m_randGen;
    };

  }
}


#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
