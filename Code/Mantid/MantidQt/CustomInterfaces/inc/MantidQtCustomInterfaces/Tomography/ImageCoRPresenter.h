#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORPRESENTER_H_

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRView.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"

#include <boost/scoped_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the image center of rotation (and other parameters)
selection widget. In principle, in a strict MVP setup, signals from
the model should always be handled through this presenter and never go
directly to the view, and viceversa.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport ImageCoRPresenter : public IImageCoRPresenter {

public:
  /// Default constructor - normally used from the concrete view
  ImageCoRPresenter(IImageCoRView *view);
  virtual ~ImageCoRPresenter();

  void notify(IImageCoRPresenter::Notification notif);

protected:
  void initialize();

  /// clean shut down of model, view, etc.
  void cleanup();

  void processInit();
  void processBrowseImg();
  void processNewStack();
  void processSelectCoR();
  void processSelectROI();
  void processSelectNormalization();
  void processFinishedCoR();
  void processFinishedROI();
  void processFinishedNormalization();
  void processResetCoR();
  void processResetROI();
  void processResetNormalization();
  void processShutDown();

private:
  Mantid::API::WorkspaceGroup_sptr loadFITSStack(const std::string &path);
  Mantid::API::WorkspaceGroup_sptr loadFITSImage(const std::string &path);

  /// Associated view for this presenter (MVP pattern)
  IImageCoRView *const m_view;

  /// Associated model for this presenter (MVP pattern). This is just
  /// a set of coordinates
  const boost::scoped_ptr<ImageStackPreParams> m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORPRESENTER_H_
