#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidQtSliceViewer/PeakOverlayView.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    // Forward dec.
    class IPeak;
  }
}

namespace MantidQt
{
  namespace SliceViewer
  {
    /** Abstract view factory. For creating types of IPeakOverlay.
    
    @date 2012-08-24

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport PeakOverlayViewFactory
    {
    public:
      /// Create a peak view from the peak object.
      virtual boost::shared_ptr<PeakOverlayView> createView(const Mantid::Kernel::V3D&) const = 0;
      /// Setter for the radius to use for all peaks.
      virtual void setRadius(const double& radius) = 0;
      /// Destructor
      virtual ~PeakOverlayViewFactory()
      {
      }
      /// Get the plot x-axis label
      virtual std::string getPlotXLabel() const = 0;
      /// Get the plot y-axis label
      virtual std::string getPlotYLabel() const = 0;
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_ */