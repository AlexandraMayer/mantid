#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <QComboBox>
#include <QDockWidget>
#include <QPoint>
#include <QTreeWidget>
#include <QVector>

class MantidUI;
class ApplicationWindow;
class MantidTreeWidget;
class QLabel;
class QMenu;
class QPushButton;
class QTreeWidget;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;

class MantidDockWidget: public QDockWidget
{
  Q_OBJECT
public:
  MantidDockWidget(MantidUI *mui, ApplicationWindow *parent);
  QString getSelectedWorkspaceName() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const;

public slots:
  void clickedWorkspace(QTreeWidgetItem*, int);
  void deleteWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem* item);
  void saveToProgram(const QString & name);

protected slots:
  void popupMenu(const QPoint & pos);
  void workspaceSelected();

private slots:
  void addTreeEntry(const QString &, Mantid::API::Workspace_sptr);
  void replaceTreeEntry(const QString &, Mantid::API::Workspace_sptr);
  void unrollWorkspaceGroup(const QString &,Mantid::API::Workspace_sptr);
  void removeWorkspaceEntry(const QString &);
  void treeSelectionChanged();
  void groupingButtonClick();
  void plotSpectra();
  void plotSpectraDistribution();
  void drawColorFillPlot();
  void showDetectorTable();

private:
  void createWorkspaceMenuActions();
  QString findParentName(const QString & ws_name, Mantid::API::Workspace_sptr workspace);
  void setItemIcon(QTreeWidgetItem* ws_item,  Mantid::API::Workspace_sptr workspace);
  QTreeWidgetItem *createEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace);
  void updateWorkspaceEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace);
  void updateWorkspaceGroupEntry(const QString & ws_name, Mantid::API::WorkspaceGroup_sptr workspace);
  void populateMDWorkspaceData(Mantid::API::IMDWorkspace_sptr workspace, QTreeWidgetItem* ws_item);
  void populateMDEventWorkspaceData(Mantid::API::IMDEventWorkspace_sptr workspace, QTreeWidgetItem* ws_item);
  void populateExperimentInfoData(Mantid::API::ExperimentInfo_sptr workspace, QTreeWidgetItem* ws_item);
  void populateMatrixWorkspaceData(Mantid::API::MatrixWorkspace_sptr workspace, QTreeWidgetItem* ws_item);
  void populateWorkspaceGroupData(Mantid::API::WorkspaceGroup_sptr workspace, QTreeWidgetItem* ws_item);
  void populateTableWorkspaceData(Mantid::API::ITableWorkspace_sptr workspace, QTreeWidgetItem* ws_item);
  void addMatrixWorspaceMenuItems(QMenu *menu, Mantid::API::MatrixWorkspace_const_sptr matrixWS) const;
  void addMDEventWorspaceMenuItems(QMenu *menu, Mantid::API::IMDEventWorkspace_const_sptr mdeventWS) const;
  void addPeaksWorspaceMenuItems(QMenu *menu, Mantid::API::IPeaksWorkspace_const_sptr WS) const;
  void addWorkspaceGroupMenuItems(QMenu *menu) const;
  void addTableWorkspaceMenuItems(QMenu * menu) const;
  bool isInvisibleWorkspaceOptionSet();
  
protected:
  MantidTreeWidget * m_tree;
  friend class MantidUI;

private:
  QString selectedWsName;
  
  MantidUI * const m_mantidUI;
  QSet<QString> m_known_groups;

  QPushButton *m_loadButton;
  QMenu *m_loadMenu, *m_saveToProgram;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;
  QSignalMapper *m_loadMapper, *m_programMapper;

  //Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecDistr, *m_showDetectors, *m_showBoxData, *m_colorFill, *m_showLogs, *m_showHist,
    *m_saveNexus, *m_rename, *m_delete, *m_program; 

  static Mantid::Kernel::Logger& logObject;
};


class MantidTreeWidget:public QTreeWidget
{
  Q_OBJECT

public:
  MantidTreeWidget(QWidget *w, MantidUI *mui);
  void mousePressEvent (QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mouseDoubleClickEvent(QMouseEvent *e);

  QStringList getSelectedWorkspaceNames() const;
  QMultiMap<QString,int> chooseSpectrumFromSelected() const;
  
private:
  QPoint m_dragStartPosition;
  MantidUI *m_mantidUI;
  static Mantid::Kernel::Logger& logObject;
};

class FindAlgComboBox:public QComboBox
{
    Q_OBJECT
signals:
    void enterPressed();
protected:
    void keyPressEvent(QKeyEvent *e);
};

class AlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void update();
    void findAlgTextChanged(const QString& text);
    void treeSelectionChanged();
    void selectionChanged(const QString& algName);
    void updateProgress(void* alg, const double p, const QString& msg, double estimatedTime, int progressPrecision);
    void algorithmStarted(void* alg);
    void algorithmFinished(void* alg);
protected:
    void showProgressBar();
    void hideProgressBar();

    QTreeWidget *m_tree;
    FindAlgComboBox* m_findAlg;
    QPushButton *m_runningButton;
    QProgressBar* m_progressBar;
    QHBoxLayout * m_runningLayout;
    bool m_treeChanged;
    bool m_findAlgChanged;
    QVector<void*> m_algID;
    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
};


class AlgorithmTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    AlgorithmTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui){}
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
private:
    QPoint m_dragStartPosition;
    MantidUI *m_mantidUI;
};

#endif
