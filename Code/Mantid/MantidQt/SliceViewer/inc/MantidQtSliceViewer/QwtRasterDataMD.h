#ifndef QwtRasterDataMD_H_
#define QwtRasterDataMD_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <qwt_double_interval.h>
#include <qwt_raster_data.h>
#include <vector>

namespace MantidQt
{
namespace SliceViewer
{

/** Implemenation of QwtRasterData that can display the data
 * from a slice of an IMDWorkspace.
 *
 * This can be used by QwtPlotSpectrogram's to plot 2D data.
 * It is used by the SliceViewer GUI.
 *
 * @author Janik Zikovsky
 * @date Sep 29, 2011
 */

class QWT_EXPORT QwtRasterDataMD : public QwtRasterData
{
public:
  QwtRasterDataMD();
  virtual ~QwtRasterDataMD();
  QwtRasterData* copy() const;

  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  QwtDoubleInterval range() const;
  void setRange(const QwtDoubleInterval & range);

  void setSliceParams(size_t dimX, size_t dimY, std::vector<Mantid::coord_t> & slicePoint);

  double value(double x, double y) const;

  QSize rasterHint(const QwtDoubleRect &) const;

  void setFastMode(bool fast);

protected:
  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Number of dimensions in the workspace
  size_t m_nd;

  /// Dimension index used as the X axis
  size_t m_dimX;

  /// Dimension index used as the Y axis
  size_t m_dimY;

  /// nd-sized array indicating where the slice is being done in the OTHER dimensions
  Mantid::coord_t * m_slicePoint;

  /// Range of colors to plot
  QwtDoubleInterval m_range;

  /// Not a number
  double nan;

  /// When true, renders the view as quickly as the workspace resolution allows
  /// when false, renders one point per pixel
  bool m_fast;
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMD_H_ */
