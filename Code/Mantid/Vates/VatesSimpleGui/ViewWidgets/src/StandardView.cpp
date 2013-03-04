#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <vtkDataObject.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include <QHBoxLayout>
#include <QMessageBox>
#include <QString>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

/**
 * This function sets up the UI components, adds connections for the view's
 * buttons and creates the rendering view.
 * @param parent the parent widget for the standard view
 */
StandardView::StandardView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

  // Set the cut button to create a slice on the data
  QObject::connect(this->ui.cutButton, SIGNAL(clicked()), this,
                   SLOT(onCutButtonClicked()));

  // Set the rebin button to create the RebinCutter operator
  QObject::connect(this->ui.rebinButton, SIGNAL(clicked()), this,
                   SLOT(onRebinButtonClicked()));

  // Set the scale button to create the ScaleWorkspace operator
  QObject::connect(this->ui.scaleButton, SIGNAL(clicked()),
                   this, SLOT(onScaleButtonClicked()));

  this->view = this->createRenderView(this->ui.renderFrame);
}

StandardView::~StandardView()
{
}

void StandardView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->destroyFilter(builder, QString("Slice"));
  builder->destroy(this->view);
}

pqRenderView* StandardView::getView()
{
  return this->view.data();
}

void StandardView::render()
{
  this->origSrc = pqActiveObjects::instance().activeSource();
  if (NULL == this->origSrc)
  {
    return;
  }
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  if (this->isMDHistoWorkspace(this->origSrc))
  {
    this->ui.rebinButton->setEnabled(false);
  }
  if (this->isPeaksWorkspace(this->origSrc))
  {
    this->ui.rebinButton->setEnabled(false);
    this->ui.cutButton->setEnabled(false);
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->view);
  QString reptype = "Surface";
  if (this->isPeaksWorkspace(this->origSrc))
  {
    reptype = "Wireframe";
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype.toStdString().c_str());
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  this->onAutoScale();
  emit this->triggerAccept();
}

void StandardView::onCutButtonClicked()
{
  // Apply cut to currently viewed data
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", this->getPvActiveSrc());
}

void StandardView::onRebinButtonClicked()
{
  const QString filterName = "MantidRebinning";
  if (this->hasFilter(filterName))
  {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("Please click on the "+filterName+\
                                          " entry to modify the rebinning "\
                                          "parameters."));
    return;
  }
  if (this->origSrc)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    this->rebinCut = builder->createFilter("filters", "MDEWRebinningCutter",
                                           this->origSrc);
    this->ui.cutButton->setEnabled(false);
  }
}

void StandardView::onScaleButtonClicked()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->scaler = builder->createFilter("filters",
                                       "MantidParaViewScaleWorkspace",
                                       this->getPvActiveSrc());
}

void StandardView::renderAll()
{
  this->view->render();
}

void StandardView::resetDisplay()
{
  this->view->resetDisplay();
}

void StandardView::resetCamera()
{
  this->view->resetCamera();
}

/**
 * This function enables the cut button for the standard view.
 */
void StandardView::updateUI()
{
  this->ui.cutButton->setEnabled(true);
}

} // SimpleGui
} // Vates
} // Mantid
