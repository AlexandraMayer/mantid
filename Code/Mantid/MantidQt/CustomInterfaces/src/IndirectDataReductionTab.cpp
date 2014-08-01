#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"

#include "MantidKernel/Logger.h"

namespace
{
  Mantid::Kernel::Logger g_log("IndirectDataReductionTab");
}

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectDataReductionTab::IndirectDataReductionTab(Ui::IndirectDataReduction& uiForm, QObject* parent) : QObject(parent),
      m_plots(), m_curves(), m_rangeSelectors(),
      m_properties(),
      m_dblManager(new QtDoublePropertyManager()), m_blnManager(new QtBoolPropertyManager()), m_grpManager(new QtGroupPropertyManager()),
      m_dblEdFac(new DoubleEditorFactory()),
      m_uiForm(uiForm)
  {
    m_parentWidget = dynamic_cast<QWidget *>(parent);

    m_algRunner = new MantidQt::API::AlgorithmRunner(m_parentWidget);
    m_valInt = new QIntValidator(m_parentWidget);
    m_valDbl = new QDoubleValidator(m_parentWidget);
    m_valPosDbl = new QDoubleValidator(m_parentWidget);

    const double tolerance = 0.00001;
    m_valPosDbl->setBottom(tolerance);

    QObject::connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(algorithmFinished(bool)));
    connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectDataReductionTab::~IndirectDataReductionTab()
  {
  }
  
  void IndirectDataReductionTab::runTab()
  {
    if(validate())
    {
      run();
    }
    else
    {
      g_log.warning("Failed to validate indirect tab input!");
    }
  }

  void IndirectDataReductionTab::setupTab()
  {
    setup();
  }

  void IndirectDataReductionTab::validateTab()
  {
    validate();
  }

  /**
   * Run the load algorithm with the supplied filename
   * 
   * @param filename :: The name of the file to load
   * @param outputName :: The name of the output workspace
   * @return If the algorithm was successful
   */
  bool IndirectDataReductionTab::loadFile(const QString& filename, const QString& outputName)
  {
    using namespace Mantid::API;

    Algorithm_sptr load = AlgorithmManager::Instance().createUnmanaged("Load", -1);
    load->initialize();
    load->setProperty("Filename", filename.toStdString());
    load->setProperty("OutputWorkspace", outputName.toStdString());
    load->execute();
    
    //If reloading fails we're out of options
    return load->isExecuted();
  }

  /**
   * Gets the range of the curve plotted in the mini plot
   *
   * @param curveID :: The string index of the curve in the m_curves map
   * @return A pair containing the maximum and minimum points of the curve
   */
  std::pair<double,double> IndirectDataReductionTab::getCurveRange(const QString& curveID)
  {
    size_t npts = m_curves[curveID]->data().size();

    if( npts < 2 )
      throw std::invalid_argument("Too few points on data curve to determine range.");

    return std::make_pair(m_curves[curveID]->data().x(0), m_curves[curveID]->data().x(npts-1));
  }

  /**
   * Plot a workspace to the miniplot given a workspace name and
   * a specturm index.
   *
   * This method uses the analysis data service to retrieve the workspace.
   * 
   * @param workspace :: The name of the workspace
   * @param index :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectDataReductionTab::plotMiniPlot(const QString& workspace, size_t index,
      const QString& plotID, const QString& curveID)
  {
    using namespace Mantid::API;
    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(workspace.toStdString());
    plotMiniPlot(ws, index, plotID, curveID);
  }

  /**
   * Plot a workspace to the miniplot given a workspace pointer and
   * a specturm index.
   * 
   * @param workspace :: Pointer to the workspace
   * @param wsIndex :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectDataReductionTab::plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex,
      const QString& plotID, const QString& curveID)
  {
    using Mantid::MantidVec;

    QString cID = curveID;
    if(cID == "")
      cID = plotID;

    //check if we can plot
    if( wsIndex >= workspace->getNumberHistograms() || workspace->readX(0).size() < 2 )
      return;

    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(wsIndex), false);

    if ( m_curves[cID] != NULL )
    {
      m_curves[cID]->attach(0);
      delete m_curves[cID];
      m_curves[cID] = NULL;
    }

    size_t nhist = workspace->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      emit showMessageBox("Error: Workspace index out of range.");
    }
    else
    {
      m_curves[cID] = new QwtPlotCurve();
      m_curves[cID]->setData(wsData);
      m_curves[cID]->attach(m_plots[plotID]);

      m_plots[plotID]->replot();
    }
  }

  /**
   * Sets the edge bounds of plot to prevent the user inputting invalid values
   * 
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param min :: The lower bound property in the property browser
   * @param max :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectDataReductionTab::setPlotRange(const QString& rsID, QtProperty* min, QtProperty* max,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setMinimum(min, bounds.first);
    m_dblManager->setMaximum(min, bounds.second);
    m_dblManager->setMinimum(max, bounds.first);
    m_dblManager->setMaximum(max, bounds.second);
    m_rangeSelectors[rsID]->setRange(bounds.first, bounds.second);
  }

  /**
   * Set the position of the guides on the mini plot
   * 
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param lower :: The lower bound property in the property browser
   * @param upper :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectDataReductionTab::setMiniPlotGuides(const QString& rsID, QtProperty* lower, QtProperty* upper,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setValue(lower, bounds.first);
    m_dblManager->setValue(upper, bounds.second);
    m_rangeSelectors[rsID]->setMinimum(bounds.first);
    m_rangeSelectors[rsID]->setMaximum(bounds.second);
  }

  /**
   * Runs an algorithm async
   *
   * @param algorithm :: The algorithm to be run
   */
  void IndirectDataReductionTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm)
  {
    algorithm->setRethrows(true);
    m_algRunner->startAlgorithm(algorithm);
  }

  /**
   * Handles getting the results of an algorithm running async
   *
   * @param error :: True if execution failed, false otherwise
   */
  void IndirectDataReductionTab::algorithmFinished(bool error)
  {
    if(error)
    {
      emit showMessageBox("Error running SofQWMoments. \nSee results log for details.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
