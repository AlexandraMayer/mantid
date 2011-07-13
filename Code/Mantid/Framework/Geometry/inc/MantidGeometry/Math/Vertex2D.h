#ifndef MANTID_GEOMETRY_VERTEX2D_H_
#define MANTID_GEOMETRY_VERTEX2D_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V2D.h"

namespace Mantid
{
  namespace Geometry
  {

    /** 
    Implements a vertex in two-dimensional space

    @author Martyn Gigg
    @date 2011-07-12

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_GEOMETRY_DLL Vertex2D : public Kernel::V2D
    {
    public:
      /// Default constructor (a point at the origin)
      Vertex2D();
      /// Constructor with X and Y values
      Vertex2D(const double x, const double y);
      /// Constructor with a point
      Vertex2D(const Kernel::V2D & point);
      /// Insert a vertex so that it is next
      Vertex2D * insert(Vertex2D *vertex);
      /// Remove this node from the chain
      Vertex2D * remove();
      /**
       * Returns the next in the chain
       * @returns The next vertex
       */
      inline Vertex2D * next() const { return m_next; }
     /**
       * Returns the previous in the chain
       * @returns The previous vertex
       */
      inline Vertex2D * previous() const { return m_prev; }

    private:
      /// Initialize the neighbour pointers
      void initNeighbours();

      /// Pointer to the "next" in the chain
      Vertex2D * m_next;
      /// Pointer to the "previous" in the chain
      Vertex2D * m_prev;
    };


  } // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_VERTEX2D_H_ */
