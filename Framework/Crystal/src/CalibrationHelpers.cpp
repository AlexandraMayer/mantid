#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

/**
* Updates the ParameterMap for NewInstrument to reflect the position of the
*source.
*
* @param newInstrument The instrument whose parameter map will be changed to
*reflect the new source position
* @param L0 The distance from source to sample (should be positive)
* @param newSampPos The relative shift for the new sample position
* @param pmapOld The Parameter map from the original instrument (not
*NewInstrument). "Clones" relevant information into the newInstrument's
*parameter map.
*/
void CalibrationHelpers::fixUpSourceParameterMap(
    boost::shared_ptr<const Instrument> newInstrument, double const L0,
    const V3D newSampPos) {
  boost::shared_ptr<ParameterMap> pmap = newInstrument->getParameterMap();
  IComponent_const_sptr source = newInstrument->getSource();

  IComponent_const_sptr sample = newInstrument->getSample();
  V3D SamplePos = sample->getPos();
  if (SamplePos != newSampPos) {
    V3D newSampRelPos = newSampPos - SamplePos;
    pmap->addPositionCoordinate(sample.get(), std::string("x"),
                                newSampRelPos.X());
    pmap->addPositionCoordinate(sample.get(), std::string("y"),
                                newSampRelPos.Y());
    pmap->addPositionCoordinate(sample.get(), std::string("z"),
                                newSampRelPos.Z());
  }
  V3D sourceRelPos = source->getRelativePos();
  V3D sourcePos = source->getPos();
  V3D parentSourcePos = sourcePos - sourceRelPos;
  V3D source2sampleDir = SamplePos - source->getPos();

  double scalee = L0 / source2sampleDir.norm();
  V3D newsourcePos = sample->getPos() - source2sampleDir * scalee;
  V3D newsourceRelPos = newsourcePos - parentSourcePos;

  pmap->addPositionCoordinate(source.get(), std::string("x"),
                              newsourceRelPos.X());
  pmap->addPositionCoordinate(source.get(), std::string("y"),
                              newsourceRelPos.Y());
  pmap->addPositionCoordinate(source.get(), std::string("z"),
                              newsourceRelPos.Z());
}

} // namespace Crystal
} // namespace Mantid
