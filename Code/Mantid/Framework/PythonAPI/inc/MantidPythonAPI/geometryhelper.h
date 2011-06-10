#ifndef GEOMETRYHELPER_H_
#define GEOMETRYHELPER_H_

/**
 * Based on classes written by Laurent Chapon. Will help
 * export matrices from UnitCell, Goniometer, OrientedSample, ...
 * 
 */

#include <MantidGeometry/Crystal/UnitCell.h>
#include "MantidPythonAPI/MantidVecHelper.h"

namespace Mantid 
{
namespace PythonAPI
{
using namespace Geometry;

class UnitCellWrapper: public UnitCell
{
   public:
		/// Constructor
		UnitCellWrapper(PyObject *self);
		/// Destructor
		~UnitCellWrapper();
		/// Return the UnitCell metric tensor
		static PyObject * getG(UnitCell& self);
		/// Return the UnitCell reciprocal metric tensor
		static PyObject * getGstar(UnitCell& self);
		/// Return the UnitCell B matrix
		static PyObject * getB(UnitCell& self);
    /// Recalculate unit cell parameters from G* - 3x3 numpy array 
    static void recalculateFromGstar(UnitCell& self,PyObject* p);
	private:
		/// Stored Python object
		PyObject *m_self;
};
}
}
#endif /* GEOMETRYHELPER_H_ */
