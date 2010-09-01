//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/indirectAnalysis.h"

#include "MantidKernel/ConfigService.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QValidator>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
  namespace CustomInterfaces
  {
    DECLARE_SUBWINDOW(indirectAnalysis);
  }
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
indirectAnalysis::indirectAnalysis(QWidget *parent) :
UserSubWindow(parent), m_valInt(0), m_valDbl(0)
{
}

/// Set up the dialog layout
void indirectAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // settings
  connect(m_uiForm.set_cbInst, SIGNAL(activated(int)), this, SLOT(instrumentChanged(int)));
  connect(m_uiForm.set_cbAnalyser, SIGNAL(activated(int)), this, SLOT(analyserSelected(int)));
  connect(m_uiForm.set_cbReflection, SIGNAL(activated(int)), this, SLOT(reflectionSelected(int)));

  // fury
  connect(m_uiForm.fury_pbRun, SIGNAL(clicked()), this, SLOT(furyRun()));
  // elwin
  connect(m_uiForm.elwin_pbRun, SIGNAL(clicked()), this, SLOT(elwinRun()));
  // slice
  connect(m_uiForm.slice_pbRun, SIGNAL(clicked()), this, SLOT(sliceRun()));
  // msd
  connect(m_uiForm.msd_pbRun, SIGNAL(clicked()), this, SLOT(msdRun()));
  // absorption
  connect(m_uiForm.abs_pbRun, SIGNAL(clicked()), this, SLOT(absorptionRun()));
  connect(m_uiForm.abs_cbShape, SIGNAL(activated(int)), this, SLOT(absorptionShape(int)));

  m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));

  // create validators
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  // apply validators - settings
  m_uiForm.set_leSpecMin->setValidator(m_valInt);
  m_uiForm.set_leSpecMax->setValidator(m_valInt);
  m_uiForm.set_leEFixed->setValidator(m_valDbl);
  // apply validators - fury
  m_uiForm.fury_leELow->setValidator(m_valDbl);
  m_uiForm.fury_leEWidth->setValidator(m_valDbl);
  m_uiForm.fury_leEHigh->setValidator(m_valDbl);
  // apply validators - elwin
  m_uiForm.elwin_leEStart->setValidator(m_valDbl);
  m_uiForm.elwin_leEEnd->setValidator(m_valDbl);
  // apply validators - slice
  m_uiForm.slice_leRange0->setValidator(m_valInt);
  m_uiForm.slice_leRange1->setValidator(m_valInt);
  m_uiForm.slice_leRange2->setValidator(m_valInt);
  m_uiForm.slice_leRange3->setValidator(m_valInt);
  // apply validators - msd
  m_uiForm.msd_leStartX->setValidator(m_valDbl);
  m_uiForm.msd_leEndX->setValidator(m_valDbl);
  // apply validators - absorption
  m_uiForm.abs_leAttenuation->setValidator(m_valDbl);
  m_uiForm.abs_leScatter->setValidator(m_valDbl);
  m_uiForm.abs_leDensity->setValidator(m_valDbl);
  m_uiForm.abs_leFlatHeight->setValidator(m_valDbl);
  m_uiForm.abs_leWidth->setValidator(m_valDbl);
  m_uiForm.abs_leThickness->setValidator(m_valDbl);
  m_uiForm.abs_leElementSize->setValidator(m_valDbl);
  m_uiForm.abs_leCylHeight->setValidator(m_valDbl);
  m_uiForm.abs_leRadius->setValidator(m_valDbl);
  m_uiForm.abs_leSlices->setValidator(m_valInt);
  m_uiForm.abs_leAnnuli->setValidator(m_valInt);
}

void indirectAnalysis::initLocalPython()
{
  clearSettings();

  QString pyInput = 
    "from IndirectEnergyConversion import getInstrumentDetails\n"
    "result = getInstrumentDetails('" + m_uiForm.set_cbInst->currentText() + "')\n"
    "print result\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();

  if ( pyOutput == "" )
  {
    showInformationBox("Could not gather required information from instrument definition.");
  }
  else
  {
    QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

    for (int i = 0; i< analysers.count(); i++ )
    {
      QString text; // holds Text field of combo box (name of analyser)
      QVariant data; // holds Data field of combo box (list of reflections)

      QStringList analyser = analysers[i].split("-", QString::SkipEmptyParts);

      text = analyser[0];

      if ( analyser.count() > 1 )
      {
        QStringList reflections = analyser[1].split(",", QString::SkipEmptyParts);
        data = QVariant(reflections);
        m_uiForm.set_cbAnalyser->addItem(text, data);
      }
      else
      {
        m_uiForm.set_cbAnalyser->addItem(text);
      }
    }

    analyserSelected(m_uiForm.set_cbAnalyser->currentIndex());

  }
}


// validation functions
bool indirectAnalysis::validateFury()
{
  bool valid = true;

  if ( ! m_uiForm.fury_iconFile->isValid() )
  {
    valid = false;
  }

  if ( ! m_uiForm.fury_resFile->isValid()  )
  {
    valid = false;
  }

  if ( m_uiForm.fury_leELow->text() == "" )
  {
    m_uiForm.fury_valELow->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valELow->setText(" ");
  }
  if ( m_uiForm.fury_leEWidth->text() == "" )
  {
    m_uiForm.fury_valEWidth->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valEWidth->setText(" ");
  }
  if ( m_uiForm.fury_leEHigh->text() == "" )
  {
    m_uiForm.fury_valEHigh->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.fury_valEHigh->setText(" ");
  }

  return valid;
}

bool indirectAnalysis::validateElwin()
{
  bool valid = true;

  if ( ! m_uiForm.elwin_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.elwin_leEStart->text() == "" )
  {
    m_uiForm.elwin_valRangeStart->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.elwin_valRangeStart->setText(" ");
  }
  if ( m_uiForm.elwin_leEEnd->text() == "" )
  {
    m_uiForm.elwin_valRangeEnd->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.elwin_valRangeEnd->setText(" ");
  }


  return valid;
}

bool indirectAnalysis::validateSlice()
{
  bool valid = true;
  // ...
  if ( ! m_uiForm.slice_inputFile->isValid() )
  {
    valid = false;
  }

  if ( ! m_uiForm.slice_calibFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.slice_leRange0->text() == "" )
  {
    m_uiForm.slice_valRange0->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange0->setText(" ");
  }

  if ( m_uiForm.slice_leRange1->text() == "" )
  {
    m_uiForm.slice_valRange1->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange1->setText(" ");
  }

  if ( m_uiForm.slice_leRange2->text() == "" )
  {
    m_uiForm.slice_valRange2->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange2->setText(" ");
  }

  if ( m_uiForm.slice_leRange3->text() == "" )
  {
    m_uiForm.slice_valRange3->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.slice_valRange3->setText(" ");
  }

  return valid;
}
bool indirectAnalysis::validateMsd()
{
  bool valid = true;

  if ( ! m_uiForm.msd_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.msd_leStartX->text() == "" )
  {
    m_uiForm.msd_valStartX->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.msd_valStartX->setText(" ");
  }

  if ( m_uiForm.msd_leEndX->text() == "" )
  {
    m_uiForm.msd_valEndX->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.msd_valEndX->setText(" ");
  }


  return valid;
}



bool indirectAnalysis::validateAbsorption()
{
  bool valid = true;

  if ( ! m_uiForm.abs_inputFile->isValid() )
  {
    valid = false;
  }

  if ( m_uiForm.abs_leAttenuation->text() == "" )
  {
    m_uiForm.abs_valAttenuation->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valAttenuation->setText(" ");
  }

  if ( m_uiForm.abs_leScatter->text() == "" )
  {
    m_uiForm.abs_valScatter->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valScatter->setText(" ");
  }

  if ( m_uiForm.abs_leDensity->text() == "" )
  {
    m_uiForm.abs_valDensity->setText("*");
    valid = false;
  }
  else
  {
    m_uiForm.abs_valDensity->setText(" ");
  }

  if ( m_uiForm.abs_cbShape->currentText() == "Flat Plate" )
  {
    // ... FLAT PLATE
    if ( m_uiForm.abs_leFlatHeight->text() == "" )
    {
      m_uiForm.abs_valFlatHeight->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valFlatHeight->setText(" ");
    }

    if ( m_uiForm.abs_leWidth->text() == "" )
    {
      m_uiForm.abs_valWidth->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valWidth->setText(" ");
    }

    if ( m_uiForm.abs_leThickness->text() == "" )
    {
      m_uiForm.abs_valThickness->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valThickness->setText(" ");
    }

    if ( m_uiForm.abs_leElementSize->text() == "" )
    {
      m_uiForm.abs_valElementSize->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valElementSize->setText(" ");
    }
  }
  else
  {
    // ... CYLINDER
    if ( m_uiForm.abs_leCylHeight->text() == "" )
    {
      m_uiForm.abs_valCylHeight->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valCylHeight->setText(" ");
    }

    if ( m_uiForm.abs_leRadius->text() == "" )
    {
      m_uiForm.abs_valRadius->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valRadius->setText(" ");
    }

    if ( m_uiForm.abs_leSlices->text() == "" )
    {
      m_uiForm.abs_valSlices->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valSlices->setText(" ");
    }

    if ( m_uiForm.abs_leAnnuli->text() == "" )
    {
      m_uiForm.abs_valAnnuli->setText("*");
      valid = false;
    }
    else
    {
      m_uiForm.abs_valAnnuli->setText(" ");
    }
  }

  return valid;
}
// cleanups
void indirectAnalysis::clearSettings()
{
  m_uiForm.set_cbAnalyser->clear();
}

// settings slots
void indirectAnalysis::instrumentChanged(int index)
{
  this->initLocalPython();
}

void indirectAnalysis::analyserSelected(int index)
{
  // populate Reflection combobox with correct values for Analyser selected.
  m_uiForm.set_cbReflection->clear();
  // clearReflectionInfo();

  QVariant currentData = m_uiForm.set_cbAnalyser->itemData(index);
  if ( currentData == QVariant::Invalid )
  {
    m_uiForm.set_lbReflection->setEnabled(false);
    m_uiForm.set_cbReflection->setEnabled(false);
  }
  else
  {
    m_uiForm.set_lbReflection->setEnabled(true);
    m_uiForm.set_cbReflection->setEnabled(true);
    QStringList reflections = currentData.toStringList();
    for ( int i = 0; i < reflections.count(); i++ )
    {
      m_uiForm.set_cbReflection->addItem(reflections[i]);
    }

    reflectionSelected(m_uiForm.set_cbReflection->currentIndex());
  }


}

void indirectAnalysis::reflectionSelected(int index)
{
  QString pyInput =
    "from IndirectEnergyConversion import getReflectionDetails\n"
    "instrument = '" + m_uiForm.set_cbInst->currentText() + "'\n"
    "analyser = '" + m_uiForm.set_cbAnalyser->currentText() + "'\n"
    "reflection = '" + m_uiForm.set_cbReflection->currentText() + "'\n"
    "print getReflectionDetails(instrument, analyser, reflection)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();

  QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

  m_uiForm.set_leSpecMin->setText(values[0]);
  m_uiForm.set_leSpecMax->setText(values[1]);
  m_uiForm.set_leEFixed->setText(values[2]);
}

/// fury slots
void indirectAnalysis::furyRun()
{
  if ( !validateFury() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import fury\n"
    "sample = r'" + m_uiForm.fury_iconFile->getFirstFilename() + "'\n"
    "resolution = r'" + m_uiForm.fury_resFile->getFirstFilename() + "'\n"
    "rebin = '" + m_uiForm.fury_leELow->text()+","+m_uiForm.fury_leEWidth->text()+","+ m_uiForm.fury_leEHigh->text()+"'\n";

  if ( m_uiForm.fury_ckSave->isChecked() )
  {
    pyInput += "save = True\n";
  }
  else
  {
    pyInput += "save = False\n";
  }
  pyInput +=
    "fury_ws = fury(sample, resolution, rebin, Save=save)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::elwinRun()
{
  if ( ! validateElwin() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import elwin\n"
    "input = [r'" + m_uiForm.elwin_inputFile->getFirstFilename() + "']\n"
    "eRange = [ " + m_uiForm.elwin_leEStart->text() +","+ m_uiForm.elwin_leEEnd->text() +"]\n"
    "eFixed = "+ m_uiForm.set_leEFixed->text() +"\n";
  if ( m_uiForm.elwin_ckSave->isChecked() )
  {
    pyInput += "save = True\n";
  }
  else
  {
    pyInput += "save = False\n";
  }
  pyInput +=
    "elwin_ws = elwin(input, eRange, eFixed, Save=save)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::sliceRun()
{
  if ( ! validateSlice() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import slice\n"
    "tofRange = [" + m_uiForm.slice_leRange0->text() + ","
    + m_uiForm.slice_leRange1->text() + ","
    + m_uiForm.slice_leRange2->text() + ","
    + m_uiForm.slice_leRange3->text() + "]\n"
    "rawfile = [r'" + m_uiForm.slice_inputFile->getFirstFilename() + "']\n"
    "calib = r'" + m_uiForm.slice_calibFile->getFirstFilename() + "'\n"
    "spec = ["+m_uiForm.set_leSpecMin->text() + "," + m_uiForm.set_leSpecMax->text() +"]\n";

  if ( m_uiForm.slice_ckSave->isChecked() )
    pyInput += "save = True\n";
  else
    pyInput += "save = False\n";

  pyInput +=
    "slice(rawfile, calib, tofXRange=tofRange, spectra=spec, Save=save)";

  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::msdRun()
{
  if ( ! validateMsd() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + m_uiForm.msd_leStartX->text() +"\n"
    "endX = " + m_uiForm.msd_leEndX->text() +"\n"
    "elwin = r'" + m_uiForm.msd_inputFile->getFirstFilename() + "'\n";

  if ( m_uiForm.msd_ckSave->isChecked() )
    pyInput += "save = True\n";
  else
    pyInput += "save = False\n";

  if ( m_uiForm.msd_ckPlot->isChecked() )
    pyInput += "plot = True\n";
  else
    pyInput += "plot = False\n";

  pyInput +=
    "msdfit(elwin, startX, endX, Save=save, Plot=plot)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();


}
void indirectAnalysis::absorptionRun()
{
  if ( ! validateAbsorption() )
  {
    showInformationBox("Please check your input.");
    return;
  }

  QString pyInput =
    "from IndirectDataAnalysis import absorption\n"
    "efixed = " + m_uiForm.set_leEFixed->text() + "\n"
    "file = r'" + m_uiForm.abs_inputFile->getFirstFilename() + "'\n"
    "mode = '" + m_uiForm.abs_cbShape->currentText() + "'\n"
    "sample = [ %0, %1, %2 ]\n"
    "can = [ %3, %4, %5, %6 ]\n";

  pyInput = pyInput.arg(m_uiForm.abs_leAttenuation->text());
  pyInput = pyInput.arg(m_uiForm.abs_leScatter->text());
  pyInput = pyInput.arg(m_uiForm.abs_leDensity->text());

  if ( m_uiForm.abs_cbShape->currentText() == "Flat Plate" )
  {
    pyInput = pyInput.arg(m_uiForm.abs_leFlatHeight->text());
    pyInput = pyInput.arg(m_uiForm.abs_leWidth->text());
    pyInput = pyInput.arg(m_uiForm.abs_leThickness->text());
    pyInput = pyInput.arg(m_uiForm.abs_leElementSize->text());
  }
  else
  {
    pyInput = pyInput.arg(m_uiForm.abs_leCylHeight->text());
    pyInput = pyInput.arg(m_uiForm.abs_leRadius->text());
    pyInput = pyInput.arg(m_uiForm.abs_leSlices->text());
    pyInput = pyInput.arg(m_uiForm.abs_leAnnuli->text());
  }
  if ( m_uiForm.abs_ckSave->isChecked() )
  {
    pyInput += "save = True\n";
  }
  else
  {
    pyInput += "save = False\n";
  }
  pyInput +=
    "absorption(file, mode, sample, can, efixed, Save=save)\n";
  QString pyOutput = runPythonCode(pyInput).trimmed();
}
void indirectAnalysis::absorptionShape(int index)
{
  m_uiForm.abs_swDetails->setCurrentIndex(index);
}