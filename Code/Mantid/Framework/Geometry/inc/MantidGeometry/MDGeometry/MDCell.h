#ifndef MDCell_H_
#define MDCell_H_

/** Abstract type to represent a group of multidimensional pixel/points. 
*   This type may not be a suitable pure virtual type in future iterations. Future implementations should be non-abstract.

    @author Owen Arnold, RAL ISIS
    @date 15/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include <boost/shared_ptr.hpp>
#include "MantidGeometry/MDGeometry/MDPoint.h"
namespace Mantid
{
  namespace Geometry
  {
    class MANTID_GEOMETRY_DLL MDCell : public SignalAggregate
    {
    private:
      double m_cachedSignal;
      double m_cachedError;
      std::vector<Coordinate> m_vertexes;
      std::vector<boost::shared_ptr<MDPoint> > m_contributingPoints;
      inline void calculateCachedValues();
    public:
      /// Default constructor
      MDCell(){};
      /// Construct from exising points.
      MDCell(std::vector<boost::shared_ptr<MDPoint> > pContributingPoints, std::vector<Coordinate> vertexes);
      /// Construct image-only mode. No contributing points maintained.
      MDCell(const double& signal,const double& error, const std::vector<Coordinate>& vertexes);
      std::vector<Coordinate> getVertexes() const;
      signal_t getSignal() const;
      signal_t getError() const;
      std::vector<boost::shared_ptr<MDPoint> > getContributingPoints() const;
      ~MDCell();
    };
  }
}

#endif 
