#ifndef INSTRUMENTWINDOWMASKTAB_H_
#define INSTRUMENTWINDOWMASKTAB_H_

#include "InstrumentWindowTab.h"
#include "MantidGLWidget.h"

#include <QFrame>
#include <QMap>

#include <boost/shared_ptr.hpp>

class Instrument3DWidget;
class CollapsiblePanel;
class OneCurvePlot;
class Shape2D;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QAction;

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;

namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
  }
}

/**
  * Implements the Mask tab in InstrumentWindow
  */
class InstrumentWindowMaskTab: public InstrumentWindowTab
{
  Q_OBJECT
public:
  enum Activity {Move = 0, Select = 1, DrawEllipse};
  InstrumentWindowMaskTab(InstrumentWindow* instrWindow);
  void initSurface();
signals:
  void executeAlgorithm(const QString&, const QString&);
protected slots:
  void setActivity();
  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();
  void clearShapes();
  void applyMask();
  void storeMask();
  void clearMask();
  void saveInvertedMaskToWorkspace();
  void saveInvertedMaskToFile();
  void saveMaskToWorkspace();
  void saveMaskToFile();

  void doubleChanged(QtProperty*);
protected:
  void showEvent (QShowEvent *);

  void clearProperties();
  void setProperties();
  boost::shared_ptr<Mantid::API::MatrixWorkspace> createMaskWorkspace(bool invertMask, bool temp = false);
  void saveMaskingToWorkspace(bool invertMask = false);
  void saveMaskingToFile(bool invertMask = false);
  std::string generateMaskWorkspaceName(bool temp = false) const;
  void enableApply(bool on);
  void enableClear(bool on);
  void setSelectActivity();

  /// Is it used?
  Activity m_activity;
  /// True if there is a mask not applied to the data workspace
  bool m_hasMaskToApply;

  // buttons
  QPushButton* m_move;
  QPushButton* m_pointer;
  QPushButton* m_ellipse;
  QPushButton* m_rectangle;
  QPushButton* m_ring_ellipse;
  QPushButton* m_ring_rectangle;

  QPushButton* m_apply;
  QPushButton* m_clear_all;
  QPushButton* m_saveButton;

  QAction* m_save_as_workspace_include;
  QAction* m_save_as_workspace_exclude;
  QAction* m_save_as_file_include;
  QAction* m_save_as_file_exclude;

  // properties
  bool m_userEditing;
  QtGroupPropertyManager  *m_groupManager;
  QtStringPropertyManager *m_stringManager;
  QtDoublePropertyManager *m_doubleManager;

  QtTreePropertyBrowser* m_browser;

  QtProperty *m_left;
  QtProperty *m_top;
  QtProperty *m_right;
  QtProperty *m_bottom;

  QMap<QtProperty *,QString> m_doublePropertyMap;
  QMap<QString,QtProperty *> m_pointPropertyMap;
  QMap<QtProperty *,QString> m_pointComponentsMap;
};


#endif /*INSTRUMENTWINDOWMASKTAB_H_*/
