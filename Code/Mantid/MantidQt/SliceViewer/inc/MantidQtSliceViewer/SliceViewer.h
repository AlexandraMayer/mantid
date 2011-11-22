#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include "ColorBarWidget.h"
#include "DimensionSliceWidget.h"
#include "DllOption.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidQtAPI/MantidColorMap.h"
#include "MantidQtSliceViewer/LineOverlay.h"
#include "QwtRasterDataMD.h"
#include "ui_SliceViewer.h"
#include <QtCore/QtCore>
#include <QtGui/qdialog.h>
#include <QtGui/QWidget>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_raster_data.h>
#include <qwt_scale_widget.h>
#include <vector>
#include "MantidKernel/VMD.h"

namespace MantidQt
{
namespace SliceViewer
{

/** GUI for viewing a 2D slice out of a multi-dimensional workspace.
 * You can select which dimension to plot as X,Y, and the cut point
 * along the other dimension(s).
 *
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceViewer : public QWidget
{
  Q_OBJECT

public:
  SliceViewer(QWidget *parent = 0);
  ~SliceViewer();

  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
  void showControls(bool visible);
  void zoomBy(double factor);
  void loadColorMap(QString filename = QString() );
  LineOverlay * getLineOverlay() { return m_lineOverlay; }
  Mantid::Kernel::VMD getSlicePoint() const { return m_slicePoint; }
  size_t getDimX() const { return m_dimX; }
  size_t getDimY() const { return m_dimY; }

signals:
  /// Signal emitted when the X/Y index of the shown dimensions is changed
  void changedShownDim(size_t dimX, size_t dimY);
  /// Signal emitted when the slice point moves
  void changedSlicePoint(Mantid::Kernel::VMD slicePoint);
  /// Signal emitted when the LineViewer should be shown/hidden.
  void showLineViewer(bool);

public slots:
  void changedShownDim(int index, int dim, int oldDim);
  void resetZoom();
  void showInfoAt(double, double);
  void colorRangeFullSlot();
  void colorRangeSliceSlot();
  void colorRangeChanged();
  void btnDoLineToggled(bool);
  void zoomInSlot();
  void zoomOutSlot();
  void updateDisplaySlot(int index, double value);
  void loadColorMapSlot();


private:
  void loadSettings();
  void saveSettings();
  void initMenus();
  void initZoomer();

  void updateDisplay(bool resetAxes = false);
  void updateDimensionSliceWidgets();
  void resetAxis(int axis, Mantid::Geometry::IMDDimension_const_sptr dim);
  QwtDoubleInterval getRange(Mantid::API::IMDIterator * it);

  void findRangeFull();
  void findRangeSlice();


private:
  // -------------------------- Widgets ----------------------------

  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Main plot object
  QwtPlot * m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram * m_spect;

  /// Layout containing the spectrogram
  QHBoxLayout * m_spectLayout;

  /// Color bar indicating the color scale
  ColorBarWidget * m_colorBar;

  /// Vector of the widgets for slicing dimensions
  std::vector<DimensionSliceWidget *> m_dimWidgets;

  /// The LineOverlay widget for drawing line cross-sections (hidden at startup)
  LineOverlay * m_lineOverlay;



  // -------------------------- Data Members ----------------------------

  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Set to true once the first workspace has been loaded in it
  bool m_firstWorkspaceOpen;

  /// File of the last loaded color map.
  QString m_currentColorMapFile;

  /// Vector of the dimensions to show.
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_dimensions;

  /// Data presenter
  QwtRasterDataMD * m_data;

  /// The X and Y dimensions being plotted
  Mantid::Geometry::IMDDimension_const_sptr m_X;
  Mantid::Geometry::IMDDimension_const_sptr m_Y;
  size_t m_dimX;
  size_t m_dimY;

  /// The point of slicing in the other dimensions
  Mantid::Kernel::VMD m_slicePoint;

  /// The range of values to fit in the color map.
  QwtDoubleInterval m_colorRange;

  /// The calculated range of values in the FULL data set
  QwtDoubleInterval m_colorRangeFull;

  /// The calculated range of values ONLY in the currently viewed part of the slice
  QwtDoubleInterval m_colorRangeSlice;

  /// Use the log of the value for the color scale
  bool m_logColor;

  /// Menus
  QMenu * m_menuColorOptions;
  QMenu * m_menuView;

  /// Cached double for infinity
  double m_inf;
};

} // namespace SliceViewer
} // namespace Mantid

#endif // SLICEVIEWER_H
