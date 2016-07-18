#ifndef MUONFITPROPERTYBROWSER_H_
#define MUONFITPROPERTYBROWSER_H_

#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/IMuonFitFunctionControl.h"

/* Forward declarations */

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;
class QVBoxLayout;
class QSplitter;

namespace Mantid {
namespace API {
class IFitFunction;
class IPeakFunction;
class CompositeFunction;
}
}

namespace MantidQt {
namespace MantidWidgets {
class PropertyHandler;

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFitPropertyBrowser
    : public MantidQt::MantidWidgets::FitPropertyBrowser,
      public MantidQt::MantidWidgets::IMuonFitFunctionControl {
  Q_OBJECT

public:
  /// Constructor.
  MuonFitPropertyBrowser(QWidget *parent = NULL, QObject *mantidui = NULL);
  /// Initialise the layout.
  void init() override;
  /// Set the input workspace name
  void setWorkspaceName(const QString &wsName) override;
  /// Called when the fit is finished
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  /// Add an extra widget into the browser
  void addExtraWidget(QWidget *widget);
  /// Set function externally
  void setFunction(const Mantid::API::IFunction_sptr func) override;
  /// Run a non-sequential fit
  void runFit() override;
  /// Run a sequential fit
  void runSequentialFit() override;
  /// Get the fitting function
  Mantid::API::IFunction_sptr getFunction() const override {
    return getFittingFunction();
  }
  /// Set parameter value externally
  void setParameterValue(const QString &funcIndex, const QString &paramName,
                         double value) override;
  /// Set list of workspaces to fit
  void setWorkspaceNames(const QStringList &wsNames) override;
  /// Get output name
  std::string outputName() const override;
  /// Prefix for simultaneous fit results
  static const std::string SIMULTANEOUS_PREFIX;
  /// Set label for simultaneous fit results
  void setSimultaneousLabel(const std::string &label) override {
    m_simultaneousLabel = label;
  }
  /// Get names of workspaces that are set to be fitted
  std::vector<std::string> getWorkspaceNamesToFit() const override {
    return m_workspacesToFit;
  }
  /// User changed dataset index to fit
  void userChangedDataset(int index) override {
    emit userChangedDatasetIndex(index);
  }

public slots:
  /// Perform the fit algorithm
  void fit() override;
  /// Open sequential fit dialog
  void sequentialFit() override;

signals:
  /// Emitted when sequential fit is requested by user
  void sequentialFitRequested();
  /// Emitted when function should be updated
  void functionUpdateRequested() override;
  /// Emitted when a fit or sequential fit is requested
  void functionUpdateAndFitRequested(bool sequential) override;
  /// Emitted when number of workspaces to fit is changed
  void workspacesToFitChanged(int n) override;
  /// Emitted when dataset index to fit is changed
  void userChangedDatasetIndex(int index) override;

protected:
  void showEvent(QShowEvent *e) override;

private slots:
  void doubleChanged(QtProperty *prop) override;

private:
  /// Get the registered function names
  void populateFunctionNames() override;
  /// Check if the workspace can be used in the fit
  bool isWorkspaceValid(Mantid::API::Workspace_sptr) const override;
  /// After a simultaneous fit, add information to results table and group
  /// workspaces
  void finishAfterSimultaneousFit(const Mantid::API::IAlgorithm *fitAlg,
                                  const int nWorkspaces) const;
  /// Layout for extra widgets
  QVBoxLayout *m_additionalLayout;
  /// Splitter for additional widgets
  QSplitter *m_widgetSplitter;
  /// Names of workspaces to fit
  std::vector<std::string> m_workspacesToFit;
  /// Label to use for simultaneous fits
  std::string m_simultaneousLabel;
};

} // MantidQt
} // API

#endif /*MUONFITPROPERTYBROWSER_H_*/
