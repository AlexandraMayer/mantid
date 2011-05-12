#include "MantidQtCustomInterfaces/SANSPlotSpecial.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QLineEdit>

#include "qwt_plot_curve.h"

namespace MantidQt
{
namespace CustomInterfaces
{

SANSPlotSpecial::SANSPlotSpecial(QWidget *parent) : 
  QFrame(parent), m_rangeSelector(NULL), m_transforms(), m_current(""),
  m_dataCurve(new QwtPlotCurve()), m_linearCurve(new QwtPlotCurve())
{
  m_uiForm.setupUi(this);
  initLayout();
}

SANSPlotSpecial::~SANSPlotSpecial()
{
  //
}

void SANSPlotSpecial::rangeChanged(double low, double high)
{
  Mantid::API::IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Linear");
  fit->initialize();
  fit->setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace", m_workspaceIQT);
  fit->setPropertyValue("OutputWorkspace", "__sans_isis_display_linear");
  fit->setProperty<double>("StartX", low);
  fit->setProperty<double>("EndX", high);
  fit->execute();

  m_workspaceLinear = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve("__sans_isis_display_linear"));
  m_linearCurve = plotMiniplot(m_linearCurve, m_workspaceLinear);

  QPen fitPen(Qt::red, Qt::SolidLine);
  m_linearCurve->setPen(fitPen);
  m_uiForm.plotWindow->replot();

  double intercept = fit->getProperty("FitIntercept");
  double gradient = fit->getProperty("FitSlope");
  double chisqrd = fit->getProperty("Chi2");

  m_uiForm.lbInterceptValue->setText(QString::number(intercept));
  m_uiForm.lbGradientValue->setText(QString::number(gradient));
  m_uiForm.lbChiSqrdValue->setText(QString::number(chisqrd));
}

void SANSPlotSpecial::plot()
{
  // validate input
  if ( ! validatePlotOptions() )
  {
    return;
  }
  // Run iq transform algorithm
  m_workspaceIQT = runIQTransform();
  // plot data to the plotWindow
  m_dataCurve = plotMiniplot(m_dataCurve, m_workspaceIQT);
  // update fields of table of "derived" values?
}

void SANSPlotSpecial::help()
{
  //
}

void SANSPlotSpecial::updateAxisLabels(const QString & value)
{
  if ( m_current != "" )
  {
    foreach ( QWidget* item, m_transforms[m_current]->xWidgets() )
    {
      m_uiForm.layoutXAxis->removeWidget(item);
      delete item;
    }
    foreach ( QWidget* item, m_transforms[m_current]->yWidgets() )
    {
      m_uiForm.layoutYAxis->removeWidget(item);
      delete item;
    }
    m_transforms[m_current]->init();
  }

  foreach ( QWidget* item, m_transforms[value]->xWidgets() )
  {
    m_uiForm.layoutXAxis->addWidget(item);
  }
  foreach ( QWidget* item, m_transforms[value]->yWidgets() )
  {
    m_uiForm.layoutYAxis->addWidget(item);
  }

  m_current = value;
}

void SANSPlotSpecial::initLayout()
{
  createTransforms();

  // Setup the cosmetics for the plotWindow
  m_uiForm.plotWindow->setAxisFont(QwtPlot::xBottom, this->font());
  m_uiForm.plotWindow->setAxisFont(QwtPlot::yLeft, this->font());
  m_uiForm.plotWindow->setCanvasBackground(Qt::white);

  // Setup RangeSelector widget for use on the plotWindow
  m_rangeSelector = new MantidWidgets::RangeSelector(m_uiForm.plotWindow);
  connect(m_rangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(rangeChanged(double, double)));

  // Other signal/slot connections
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.cbBackground, SIGNAL(currentIndexChanged(int)), m_uiForm.swBackground, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateAxisLabels(const QString &)));

  updateAxisLabels(m_uiForm.cbPlotType->currentText());
}

Mantid::API::MatrixWorkspace_sptr SANSPlotSpecial::runIQTransform()
{
  // Run the IQTransform algorithm for the current settings on the GUI
  Mantid::API::IAlgorithm_sptr iqt = Mantid::API::AlgorithmManager::Instance().create("IQTransform");
  iqt->initialize();
  iqt->setPropertyValue("InputWorkspace", m_uiForm.wsInput->currentText().toStdString());
  iqt->setPropertyValue("OutputWorkspace", "sans_isis_display_iqt");
  iqt->setPropertyValue("TransformType", m_uiForm.cbPlotType->currentText().toStdString());
  
  if ( m_uiForm.cbBackground->currentText() == "Value" )
  { 
    iqt->setProperty<double>("BackgroundValue", m_uiForm.dsBackground->value());
  }
  else
  { 
    iqt->setPropertyValue("BackgroundWorkspace", m_uiForm.wsBackground->currentText().toStdString());
  }

  if ( m_uiForm.cbPlotType->currentText() == "General" )
  {
    std::vector<double> constants = m_transforms["General"]->functionConstants();
    iqt->setProperty("GeneralFunctionConstants", constants);
  }

  iqt->execute();

  Mantid::API::MatrixWorkspace_sptr result =
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("sans_isis_display_iqt"));
  return result;
}

bool SANSPlotSpecial::validatePlotOptions()
{
  bool valid = true;
  m_uiForm.lbPlotOptionsError->setText("");
  QString error = "";

  if ( m_uiForm.wsInput->currentText() == "" )
  {
    error += "Please select an input workspace.\n";
    valid = false;
  }

  if ( m_uiForm.cbBackground->currentText() == "Workspace" &&
    m_uiForm.wsBackground->currentText() == "" )
  {
    error += "Please select a background workspace.\n";
    valid = false;
  }

  if ( m_uiForm.cbPlotType->currentText() == "General" )
  {
    std::vector<double> params = m_transforms["General"]->functionConstants();
    if ( params.size() != 10 )
    {
      error += "Constants for general function not provided.";
      valid = false;
    }
  }

  m_uiForm.lbPlotOptionsError->setText(error.trimmed());
  return valid;
}

void SANSPlotSpecial::createTransforms()
{
  m_transforms.clear();

  m_transforms["Guinier (spheres)"] = new Transform(Transform::GuinierSpheres);
  m_uiForm.cbPlotType->addItem("Guinier (spheres)");
  m_transforms["Guinier (rods)"] = new Transform(Transform::GuinierRods);
  m_uiForm.cbPlotType->addItem("Guinier (rods)");
  m_transforms["Guinier (sheets)"] = new Transform(Transform::GuinierSheets);
  m_uiForm.cbPlotType->addItem("Guinier (sheets)");
  m_transforms["Zimm"] = new Transform(Transform::Zimm);
  m_uiForm.cbPlotType->addItem("Zimm");
  m_transforms["Debye-Bueche"] = new Transform(Transform::DebyeBueche);
  m_uiForm.cbPlotType->addItem("Debye-Bueche");
  m_transforms["Holtzer"] = new Transform(Transform::Holtzer);
  m_uiForm.cbPlotType->addItem("Holtzer");
  m_transforms["Kratky"] = new Transform(Transform::Kratky);
  m_uiForm.cbPlotType->addItem("Kratky");
  m_transforms["Porod"] = new Transform(Transform::Porod);
  m_uiForm.cbPlotType->addItem("Porod");
  m_transforms["Log-Log"] = new Transform(Transform::LogLog);
  m_uiForm.cbPlotType->addItem("Log-Log");
  m_transforms["General"] = new Transform(Transform::General);
  m_uiForm.cbPlotType->addItem("General");
}

QwtPlotCurve* SANSPlotSpecial::plotMiniplot(QwtPlotCurve* curve, Mantid::API::MatrixWorkspace_sptr workspace)
{
  bool data = ( curve == m_dataCurve );

  if ( curve != NULL )
  {
    curve->attach(0);
    delete curve;
    curve = 0;
  }

  curve = new QwtPlotCurve();

  const QVector<double> dataX = QVector<double>::fromStdVector(workspace->readX(0));
  const QVector<double> dataY = QVector<double>::fromStdVector(workspace->readY(0));

  curve->setData(dataX, dataY);
  curve->attach(m_uiForm.plotWindow);

  m_uiForm.plotWindow->replot();
  
  if ( data )
  {
    double low = dataX.first();
    double high = dataX.last();
    m_rangeSelector->setRange(low, high);
  }

  return curve;
}

//--------------------------------------------------------------------
//------- Utility "Transform" Class ----------------------------------
//--------------------------------------------------------------------
SANSPlotSpecial::Transform::Transform(Transform::TransformType type, QWidget* parent) : m_type(type), 
  m_xWidgets(QList<QWidget*>()), m_yWidgets(QList<QWidget*>()), m_parent(parent)
{
  init();
}

SANSPlotSpecial::Transform::~Transform() {}

void SANSPlotSpecial::Transform::init()
{
  m_xWidgets.clear();
  m_yWidgets.clear();

  switch ( m_type )
  {
  case GuinierSpheres:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I)", this));
    break;
  case GuinierRods:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I (Q) )", this));
    break;
  case GuinierSheets:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("ln (I (Q ^ 2 ) )", this));
    break;
  case Zimm:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("1 / I", this));
    break;
  case DebyeBueche:
    m_xWidgets.append(new QLabel("Q^2", this));
    m_yWidgets.append(new QLabel("1 / sqrt (I)", this));
    break;
  case Holtzer:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q", this));
    break;
  case Kratky:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q^2", this));
    break;
  case Porod:
    m_xWidgets.append(new QLabel("Q", this));
    m_yWidgets.append(new QLabel("I * Q^4", this));
    break;
  case LogLog:
    m_xWidgets.append(new QLabel("ln (Q)", this));
    m_yWidgets.append(new QLabel("ln (I)", this));
    break;
  case General:
    m_xWidgets.append(new QLabel("Q^", this));
    m_xWidgets.append(new QLineEdit("C6", this));
    m_xWidgets.append(new QLabel("*I^", this));
    m_xWidgets.append(new QLineEdit("C7", this));
    m_xWidgets.append(new QLabel("*ln(Q^", this));
    m_xWidgets.append(new QLineEdit("C8", this));
    m_xWidgets.append(new QLabel("*I^", this));
    m_xWidgets.append(new QLineEdit("C9", this));
    m_xWidgets.append(new QLabel("*", this));
    m_xWidgets.append(new QLineEdit("C10", this));
    m_xWidgets.append(new QLabel(")", this));
    m_yWidgets.append(new QLabel("Q^", this));
    m_yWidgets.append(new QLineEdit("C1", this));
    m_yWidgets.append(new QLabel("*I^", this));
    m_yWidgets.append(new QLineEdit("C2", this));
    m_yWidgets.append(new QLabel("*ln(Q^", this));
    m_yWidgets.append(new QLineEdit("C3", this));
    m_yWidgets.append(new QLabel("*I^", this));
    m_yWidgets.append(new QLineEdit("C4", this));
    m_yWidgets.append(new QLabel("*", this));
    m_yWidgets.append(new QLineEdit("C5", this));
    m_yWidgets.append(new QLabel(")", this));
    tidyGeneral();
    break;
  }

}

std::vector<double> SANSPlotSpecial::Transform::functionConstants()
{
  std::vector<double> result;
  if ( m_type != General ) { return result; }

  foreach ( QWidget* item, m_yWidgets )
  {
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
      QString le = dynamic_cast<QLineEdit*>(item)->text();
      result.push_back(le.toDouble());
    }
  }

  foreach ( QWidget* item, m_xWidgets )
  {
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
      QString le = dynamic_cast<QLineEdit*>(item)->text();
      result.push_back(le.toDouble());
    }
  }
  return result;
}

void SANSPlotSpecial::Transform::tidyGeneral()
{
  foreach ( QWidget* item, m_xWidgets )
  {
    item->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
    }
  }

  foreach ( QWidget* item, m_yWidgets )
  {
    item->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    if ( item->isA("QLineEdit") )
    {
      item->setMaximumSize(25,20);
    }
  }
}

}
}
