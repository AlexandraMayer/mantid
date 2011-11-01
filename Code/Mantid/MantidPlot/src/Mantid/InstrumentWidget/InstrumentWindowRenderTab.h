#ifndef INSTRUMENTWINDOWRENDERTAB_H_
#define INSTRUMENTWINDOWRENDERTAB_H_
#include "MantidQtAPI/GraphOptions.h"

#include <QFrame>

class InstrumentWindow;
class MantidGLWidget;
class BinDialog;
//class QwtScaleWidget;
class ColorMapWidget;
class MantidColorMap;

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QAction;

/**
  * Implements the Render tab in InstrumentWindow
  */
class InstrumentWindowRenderTab: public QFrame
{
  Q_OBJECT
public:
  InstrumentWindowRenderTab(InstrumentWindow* instrWindow);
  ~InstrumentWindowRenderTab();
  void setupColorBarScaling(const MantidColorMap&,double);
  void loadSettings(const QString& section);
  void saveSettings(const QString& section);
  void setMinValue(double value, bool apply = true);
  void setMaxValue(double value, bool apply = true);
  GraphOptions::ScaleType getScaleType()const;
  void setScaleType(GraphOptions::ScaleType type);
  void setAxis(const QString& axisName);
  bool areAxesOn()const;
  void init();
  void updateSurfaceTypeControl(int);
public slots:
  void showAxes(bool on);
private slots:
  void changeColormap(const QString & filename = "");
  void showResetView(int);
  void showFlipControl(int);
  void flipUnwrappedView(bool);
private:
  void showEvent (QShowEvent *);

  QFrame * setupAxisFrame();

  InstrumentWindow* m_instrWindow;
  MantidGLWidget *m_InstrumentDisplay;
  QComboBox* m_renderMode;
  QPushButton *mSaveImage;
  ColorMapWidget* m_colorMapWidget;
  QFrame* m_resetViewFrame;
  QComboBox *mAxisCombo;
  QCheckBox *m_flipCheckBox;

  QAction *m_colorMap;
  QAction *m_backgroundColor;
  QAction *m_displayAxes;
  QAction *m_wireframe;
  QAction *m_lighting;
  
};


#endif /*INSTRUMENTWINDOWRENDERTAB_H_*/
