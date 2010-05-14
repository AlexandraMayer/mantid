#ifndef MANTID_API_IINSTRUMENT_H_
#define MANTID_API_IINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace Mantid
{
namespace API
{
/** IInstrument Class. The abstract instrument class it is the base for 
    Instrument and ParInstrument classes.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
class DLLExport IInstrument : public virtual Geometry::ICompAssembly
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "IInstrument"; }

  ///Virtual destructor
  virtual ~IInstrument() {}

  /// Returns a pointer to the geometrical object representing the source
  virtual Geometry::IObjComponent_sptr getSource() const = 0;
  /// Returns a pointer to the geometrical object representing the sample
  virtual Geometry::IObjComponent_sptr getSample() const = 0;
  /// Returns a pointer to the geometrical object for the detector with the given ID
  virtual Geometry::IDetector_sptr getDetector(const int &detector_id) const = 0;

  /// Returns a pointer to the geometrical object representing the monitor with the given ID
  virtual Geometry::IDetector_sptr getMonitor(const int &detector_id) const = 0;

  virtual std::string getName() const = 0;

  /// Returns a shared pointer to a component
  virtual boost::shared_ptr<Geometry::IComponent> getComponentByID(Geometry::ComponentID id) = 0;

  /// Returns a pointer to the component with the given name
  boost::shared_ptr<Geometry::IComponent> getComponentByName(const std::string & cname);

  /// return reference to detector cache 
  virtual std::map<int, Geometry::IDetector_sptr> getDetectors() const = 0;

  /// The type used to deliver the set of plottable components
  typedef std::vector<Geometry::IObjComponent_const_sptr> plottables;
  /// A constant shared pointer to a vector of plotables
  typedef const boost::shared_ptr<const plottables> plottables_const_sptr;
  /// Get pointers to plottable components
  virtual plottables_const_sptr getPlottable() const = 0;

  /// returns a list containing  detector ids of monitors
  virtual  const std::vector<int> getMonitors()const=0;
  /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possiblities are "Z+, Z-, X+, ..."
  virtual std::string getDefaultAxis() const=0;
};

/// Shared pointer to IInstrument
typedef boost::shared_ptr<IInstrument> IInstrument_sptr;
/// Shared pointer to IInstrument (const version)
typedef boost::shared_ptr<const IInstrument> IInstrument_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_PARINSTRUMENT_H_*/
