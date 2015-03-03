#ifndef MANTID_DATAOBJECTS_MDSPLITBOX_H_
#define MANTID_DATAOBJECTS_MDSPLITBOX_H_

#include "MantidKernel/ThreadScheduler.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDLeanEvent.h"

namespace Mantid {
namespace DataObjects {

/** Similar to MDGridBox, this class is a split version of a MDBox where
 * a single left/right split occurs along a single dimension, at a variable
 *point.
 *
 * @author Janik Zikovsky
 * @date 2011-04-15 10:26:16.413856
 */
TMDE_CLASS
class DLLExport MDSplitBox : public MDBoxBase<MDE, nd> {
public:
  MDSplitBox(MDBox<MDE, nd> *box);

  MDSplitBox(MDBoxBase<MDE, nd> *box, size_t _dimSplit, coord_t _splitPoint);

  virtual ~MDSplitBox();

  void clear();

  uint64_t getNPoints() const;

  size_t getNumDims() const;

  size_t getNumMDBoxes() const;

  /// Fill a vector with all the boxes up to a certain depth
  void getBoxes(std::vector<MDBoxBase<MDE, nd> *> & /*boxes*/,
                size_t /*maxDepth*/, bool) {
    throw std::runtime_error("Not implemented.");
  }

  void addEvent(const MDE &point);

  void splitContents(size_t index, Mantid::Kernel::ThreadScheduler *ts);

  void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler *ts);

  void refreshCache(Kernel::ThreadScheduler *ts = NULL);

  virtual std::vector<MDE> *getEventsCopy() { return NULL; }

  virtual void centerpointBin(MDBin<MDE, nd> &bin, bool *fullyContained) const;

  void integrateSphere(Mantid::API::CoordTransform & /*radiusTransform*/,
                       const coord_t /*radiusSquared*/, signal_t & /*signal*/,
                       signal_t & /*errorSquared*/) const {
    throw std::runtime_error("Not implemented.");
  }

  void centroidSphere(Mantid::API::CoordTransform &, const coord_t, coord_t *,
                      signal_t &) const {
    throw std::runtime_error("Not implemented.");
  }

  void integrateCylinder(Mantid::API::CoordTransform &radiusTransform,
                         const coord_t radius, const coord_t length,
                         signal_t &signal, signal_t &errorSquared,
                         std::vector<signal_t> &signal_fit) const {
    throw std::runtime_error("Not implemented.");
  }

  // --------------------------------------------------------------------------------------------

  /// Return which dimension (index) was split
  size_t getSplitDimension() const { return dimSplit; }

  /// Return the X value in the split dimension that was the left/right
  /// splitting point
  coord_t getSplitPoint() const { return splitPoint; }

  /// Returns the MDBoxBase on the left side (x[dimSplit] < splitPoint)
  MDBoxBase<MDE, nd> *getLeft() { return left; }

  /// Returns the MDBoxBase on the right side (x[dimSplit] >= splitPoint)
  MDBoxBase<MDE, nd> *getRight() { return right; }

protected:
  /// Total number of points (events) in all sub-boxes
  size_t nPoints;

  /// Index of the dimension that this MDSplitBox splits. Between 0 and nd.
  size_t dimSplit;

  /// X-value that splits the dimension at index dimSplit.
  coord_t splitPoint;

  /// MDBoxBase on the left of the split (x[dimSplit] < splitPoint)
  MDBoxBase<MDE, nd> *left;

  /// MDBoxBase on the right of the split (x[dimSplit] >= splitPoint)
  MDBoxBase<MDE, nd> *right;

private:
  /// Used by constructor only.
  void initBoxes(MDBoxBase<MDE, nd> *box);
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_MDSPLITBOX_H_ */
