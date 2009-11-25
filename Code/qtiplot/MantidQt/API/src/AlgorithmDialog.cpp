//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/FileProperty.h"

#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QHBoxLayout>
#include <QSignalMapper>
using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
AlgorithmDialog::AlgorithmDialog(QWidget* parent) :  
  QDialog(parent), m_algorithm(NULL), m_algName(""), m_algProperties(), 
  m_propertyValueMap(), m_tied_properties(), m_forScript(false), m_python_arguments(), 
  m_enabledNames(), m_strMessage(""), m_msgAvailable(false), m_isInitialized(false), 
  m_validators(), m_inputws_opts(), m_outputws_fields(), m_wsbtn_tracker(), 
  m_signal_mapper(new QSignalMapper())
{
  connect(m_signal_mapper, SIGNAL(mapped(QWidget*)), this, SLOT(replaceWSClicked(QWidget*)));
}

/**
 * Destructor
 */
AlgorithmDialog::~AlgorithmDialog()
{
}

/**
 * Create the layout for this dialog.
 */
void AlgorithmDialog::initializeLayout()
{
  if( isInitialized() ) return;

  //Set a common title
  setWindowTitle(QString::fromStdString(getAlgorithm()->name()) + " input dialog");
  //Set the icon
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  
  // Fill the map of properties<->validator markers 
  createValidatorLabels();

  // These containers are for ensuring the 'replace input workspace; button works correctly
  // Store all combo boxes that relate to an input workspace
  m_inputws_opts.clear();
  // Store all line edit fields that relate to an output workspace name
  m_outputws_fields.clear();
  // Keep track of the input workspace that has been used to fill the output workspace. Each button click
  // cycles through all of the input workspaces
  m_wsbtn_tracker.clear();

  // This derived class function creates the layout of the widget. It can also add default input if the
  // dialog has been written this way
  this->initLayout();
  // Check if there is any default input 
  this->parse();

  // Try to set these values. This will validate the defaults and mark those that are invalid, if any.
  setPropertyValues();

  m_isInitialized = true;
}

/**
 * Has this dialog been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool AlgorithmDialog::isInitialized() const
{ 
  return m_isInitialized; 
}


//------------------------------------------------------
// Protected member functions
//------------------------------------------------------
/**
 * Parse input from widgets on the dialog. This function does nothing in the 
 * base class
 */
void AlgorithmDialog::parseInput()
{
}

/**
 * Get the algorithm pointer
 * @returns A pointer to the algorithm that is associated with the dialog
 */
Mantid::API::IAlgorithm* AlgorithmDialog::getAlgorithm() const
{
  return m_algorithm;
}

/**
 * Get a named property for this algorithm
 * @param propName The name of the property
 */
Mantid::Kernel::Property* AlgorithmDialog::getAlgorithmProperty(const QString & propName) const
{
  if( m_algProperties.contains(propName) ) return m_algProperties.value(propName);
  else return NULL;
}

/**
 * Get a property validator label
 */
QLabel* AlgorithmDialog::getValidatorMarker(const QString & propname) const
{
  if( m_validators.contains(propname) ) return m_validators.value(propname);
  return m_validators.value(propname);
}

/**
 * Adds a property (name,value) pair to the stored map
 */
void AlgorithmDialog::storePropertyValue(const QString & name, const QString & value)
{
  if( name.isEmpty() ) return;
  
  m_propertyValueMap.insert(name, value);
}

/**
 * Return the message string
 * @returns the message string
 */
const QString & AlgorithmDialog::getOptionalMessage() const
{
  return m_strMessage;
}

/**
 * Was this dialog raised from a script? This is important when deciding what to
 * do with properties that have old input
 * @returns A boolean inidcating whether we are being called from a script
 */
bool AlgorithmDialog::isForScript() const
{
  return m_forScript;
}

/*
 * Is there a message string available
 * @returns A boolean indicating whether the message string is empty
 */
bool AlgorithmDialog::isMessageAvailable() const
{
  return !m_strMessage.isEmpty();
}

/**
 * Check if the control should be enabled for this property
 * @param propName The name of the property
 */
bool AlgorithmDialog::isWidgetEnabled(const QString & propName) const
{
  // If this dialog is not for a script then always enable
  if( !isForScript() || propName.isEmpty() )
  {
    return true;
  }

  if( isInEnabledList(propName) ) return true;

  // Otherwise it must be disabled but only if it is valid
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( property->isValid().empty() )
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
 * Tie together an input widget and a property
 * @param widget The widget that will collect the input
 * @param property The name of the property to tie the given widget to
 * @param An optional pointer to a QLayout class that is reponsible for managing the passed widget.
 * If given, a validator label will be added for the given input widget
 * @returns A NULL pointer if a valid label was successfully add to a passed parent_layout otherwise it
 * returns a pointer to the QLabel instance marking the validity
 */
QWidget* AlgorithmDialog::tie(QWidget* widget, const QString & property, QLayout *parent_layout)
{
  if( m_tied_properties.contains(property) )
  {
    m_tied_properties.remove(property);
  }
  Mantid::Kernel::Property * prop=getAlgorithmProperty(property);
  if(prop) 
  { //Set a few things on the widget
    QString docstring = QString::fromStdString(prop->documentation());
    widget->setToolTip(docstring);
  }
  widget->setEnabled(isWidgetEnabled(property));
  setValue(widget, property);
  m_tied_properties.insert(property, widget);
  QLabel *validlbl = getValidatorMarker(property);
  if( !parent_layout ) return validlbl;
  
  int item_index = parent_layout->indexOf(widget);
  if( QBoxLayout *box = qobject_cast<QBoxLayout*>(parent_layout) )
  {
    box->insertWidget(item_index + 1, validlbl);
  }
  else if( QGridLayout *grid = qobject_cast<QGridLayout*>(parent_layout) )
  {
    int row(0), col(0), span(0);
    grid->getItemPosition(item_index, &row, &col, &span, &span);
    grid->addWidget(validlbl, row, col + 1);
  }
  else 
  {
  }

  return NULL;
}

/**
 * Open a file selection box. The type of dialog, i.e. load/save will depend on the
 * property type
 * @param The property name that this is associated with. 
 */
QString AlgorithmDialog::openFileDialog(const QString & propName)
{
  if( propName.isEmpty() ) return "";
  Mantid::Kernel::FileProperty* prop = 
    dynamic_cast< Mantid::Kernel::FileProperty* >( getAlgorithmProperty(propName) );
  if( !prop ) return "";

  //The allowed values in this context are file extensions
  std::set<std::string> exts = prop->allowedValues();
  QString filter;
  if( !exts.empty() )
  {
    filter = "Files (";
		
    std::set<std::string>::const_iterator iend = exts.end();
    for( std::set<std::string>::const_iterator itr = exts.begin(); itr != iend; ++itr)
    {
  	  filter.append("*." + QString::fromStdString(*itr) + " ");
    }
		
    filter.trimmed();
    filter.append(QString::fromStdString(")"));
  }
  else
  {
    filter = "All Files (*.*)";
  }
  
  /* MG 20/07/09: Static functions such as these that use native Windows and MAC dialogs 
     in those environments are alot faster. This is unforunately at the expense of 
     shell-like pattern matching, i.e. [0-9].      
  */
  QString filename;
  if( prop->isLoadProperty() )
  {
    filename = QFileDialog::getOpenFileName(this, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
    
  }
  else
  {
    filename = QFileDialog::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }

  if( !filename.isEmpty() ) 
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}

/**
 * Takes a combobox and adds the allowed values of the given property to its list. 
 * It also sets the displayed value to the correct one based on either the history
 * or a script input value
 * @param propName The name of the property
 * @param optionsBox A pointer to a QComoboBox object
 * @returns A newed QComboBox
 */
void AlgorithmDialog::fillAndSetComboBox(const QString & propName, QComboBox* optionsBox) const
{
  if( !optionsBox ) return;
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( !property ) return;
  
  std::set<std::string> items = property->allowedValues();
  std::set<std::string>::const_iterator vend = items.end();
  for(std::set<std::string>::const_iterator vitr = items.begin(); vitr != vend; 
      ++vitr)
  {
    optionsBox->addItem(QString::fromStdString(*vitr));
  }

  // Display the appropriate value
  QString displayed("");
  if( !isForScript() )
  {
    displayed = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  if( displayed.isEmpty() )
  {
    displayed = QString::fromStdString(property->value());
  }

  int index = optionsBox->findText(displayed);
  if( index >= 0 )
  {
    optionsBox->setCurrentIndex(index);
  }
}

/**
 * Takes the given property and QCheckBox pointer and sets the state based on either
 * the history or property value
 * @param propName The name of the property
 * @param 
 * @returns A newed QCheckBox
 */
void AlgorithmDialog::setCheckBoxState(const QString & propName, QCheckBox* checkBox) const
{
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
  if( !property ) return;
  
  //Check boxes are special in that if they have a default value we need to display it
  QString displayed("");
  if( !isForScript() )
  {
    displayed = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  if( displayed.isEmpty() )
  {
    displayed = QString::fromStdString(property->value());
  }

  if( displayed == "0" )
  {
    checkBox->setCheckState(Qt::Unchecked);
  }
  else
  {
    checkBox->setCheckState(Qt::Checked);
  }

}

/**
 * Set the input for a text box based on either the history or a script value
 * @param propName The name of the property
 * @param field The QLineEdit field
 */
void AlgorithmDialog::fillLineEdit(const QString & propName, QLineEdit* textField)
{
  if( !isForScript() )
  {
    textField->setText(AlgorithmInputHistory::Instance().previousInput(m_algName, propName));
  }
  else
  {
    Mantid::Kernel::Property *property = getAlgorithmProperty(propName);
    if( property && property->isValid().empty() && 
	( m_python_arguments.contains(propName) || !property->isDefault() ) ) 
    {
      textField->setText(QString::fromStdString(property->value()));
    }
  }
}

QHBoxLayout *
AlgorithmDialog::createDefaultButtonLayout(const QString & helpText,
					   const QString & loadText,
					   const QString & cancelText)
{
  QPushButton *okButton = new QPushButton(loadText);
  connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
  okButton->setDefault(true);

  QPushButton *exitButton = new QPushButton(cancelText);
  connect(exitButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *buttonRowLayout = new QHBoxLayout;
  buttonRowLayout->addWidget(createHelpButton(helpText));
  buttonRowLayout->addStretch();
  buttonRowLayout->addWidget(okButton);
  buttonRowLayout->addWidget(exitButton);
    
  return buttonRowLayout;
}

/**
 * Create a help button that, when clicked, will open a browser to the Mantid wiki page
 * for that algorithm
 */
QPushButton* AlgorithmDialog::createHelpButton(const QString & helpText) const
{
  QPushButton *help = new QPushButton(helpText);
  help->setMaximumWidth(25);
  connect(help, SIGNAL(clicked()), this, SLOT(helpClicked()));
  return help;
}

/**
 * Create a button that when clicked will put the name of the input workspace into the
 * output box.
 * @param inputBox The input combo box that contains the input workspace names
 * @param outputEdit The output text box that should contain the output name
 * @returns A new QPushButton linked to the appropriate widgets.
 */
QPushButton* AlgorithmDialog::createReplaceWSButton(QLineEdit *outputEdit)
{
  QPushButton *btn = new QPushButton(QIcon(":/data_replace.png"), "");
  // MG: There is no way with the QIcon class to actually ask what size it is so I had to hard
  // code this number here to get it to a sensible size
  btn->setMaximumWidth(32);
  m_wsbtn_tracker[btn ] = 1;
  btn->setToolTip("Replace input workspace");
  m_outputws_fields.push_back(outputEdit);
  connect(btn, SIGNAL(clicked()), m_signal_mapper, SLOT(map()));
  m_signal_mapper->setMapping(btn, outputEdit);  
  return btn;
}

/** 
 * Flag an input workspace combobox with its property name
 * @param optionsBox A QComboBox containing the options
 */
void AlgorithmDialog::flagInputWS(QComboBox *optionsBox)
{
  m_inputws_opts.push_back(optionsBox);
}

//-----------------------------------------------------------
// Protected slots
//-----------------------------------------------------------
/**
 * A slot that can be used to connect a button that accepts the dialog if
 * all of the properties are valid
 */
void AlgorithmDialog::accept()
{
  // Get property values
  parse();
  
  //Try and set and validate the properties and 
  if( setPropertyValues() )
  {
    //Store input for next time
    saveInput();
    QDialog::accept();
  }
  else
  {
    QMessageBox::critical(this, "", 
			  "One or more properties are invalid. The invalid properties are\n"
        "marked with a *, hold your mouse over the * for more information." );
  } 
}

/**
 * A slot to handle the help button click
 */
void AlgorithmDialog::helpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") + m_algName));
}

/**
 * A slot to handle the replace workspace button click
 * @param outputEdit The line edit that is associated, via the signalmapper, with this click
 */
void AlgorithmDialog::replaceWSClicked(QWidget *outputEdit)
{
  QPushButton *btn = qobject_cast<QPushButton*>(m_signal_mapper->mapping(outputEdit));
  if( !btn ) return;
  int input =  m_wsbtn_tracker.value(btn);

  QString wsname = m_inputws_opts.value(input - 1)->currentText();
  //Adjust tracker
  input = (input % m_inputws_opts.size() ) + 1;
  m_wsbtn_tracker[btn] = input;

  // Check if any of the other line edits have this name
  QVector<QLineEdit*>::const_iterator iend = m_outputws_fields.constEnd();
  for( QVector<QLineEdit*>::const_iterator itr = m_outputws_fields.constBegin();
       itr != iend; ++itr )
  {
    //Check that we are not the field we are actually comparing against
    if( (*itr) == outputEdit ) continue;
    if( (*itr)->text() == wsname )
    {
      wsname += "-1";
      break;
    }
  }
  QLineEdit *edit = qobject_cast<QLineEdit*>(outputEdit);
  if( edit )
  {
    edit->setText(wsname);
  }
}

//------------------------------------------------------
// Private member functions
//------------------------------------------------------
/**
 * Set the algorithm pointer
 * @param alg A pointer to the algorithm
 */
void AlgorithmDialog::setAlgorithm(Mantid::API::IAlgorithm* alg)
{
  m_algorithm = alg;
  m_algName = QString::fromStdString(alg->name());
  m_algProperties.clear();
  std::vector<Mantid::Kernel::Property*>::const_iterator iend = alg->getProperties().end();
  for( std::vector<Mantid::Kernel::Property*>::const_iterator itr = alg->getProperties().begin(); itr != iend;
       ++itr )
  {
    m_algProperties.insert(QString::fromStdString((*itr)->name()), *itr); 
  }
}

/**
 * Parse out information from the dialog
 */
void AlgorithmDialog::parse()
{
  QHashIterator<QString, QWidget*> itr(m_tied_properties);
  while( itr.hasNext() )
  {
    itr.next();
    //Need to do different things depending on the type of the widget. getValue sorts this out
    storePropertyValue(itr.key(), getValue(itr.value()));
  }

  //Now call parseInput, which can be overridden in an inheriting class
  parseInput();
}


/**
 * Set the properties that have been parsed from the dialog.
 * @returns A boolean that indicates if the validation was successful.
 */
bool AlgorithmDialog::setPropertyValues()
{
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  bool allValid(true);
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    Mantid::Kernel::Property *prop = pitr.value();
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    QLabel *validator = getValidatorMarker(pitr.key());

    std::string error = "";
    if ( !value.isEmpty() )
    {//if there something in the box then use it
      try
      {
        error = prop->setValue(value.toStdString());
      }
      catch(std::exception & err_details)
      {
        error = err_details.what();
      }
    }
    else
    {//else use the default which may or may not be a valid property value
      try
      {
        error = prop->setValue(prop->getDefault());
      }
      catch(std::exception& err_details)
      {
        error = err_details.what();
      }
    }

    if( error.empty() )
    {//no error
      if( validator ) validator->hide();
      //Store value for future input if it is not default
    }
    else
    {//the property could not be set
      allValid = false;
      if( validator && validator->parent() )
      {
        //a description of the problem will be visible to users if they their mouse pointer lingers over validator star mark
        validator->setToolTip(  QString::fromStdString(error) );
        validator->show();
      }
    }
  }
  return allValid;
}

/**
 * Save the property values to the input history
 */
void AlgorithmDialog::saveInput()
{
  AlgorithmInputHistory::Instance().clearAlgorithmInput(m_algName);
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QString pName = pitr.key();
    QString value = m_propertyValueMap.value(pName);
    AlgorithmInputHistory::Instance().storeNewValue(m_algName, QPair<QString, QString>(pName, value));
  }
}

/**
  * Set a list of values for the properties
  * @param presetValues A string containing a list of "name=value" pairs with each separated by an '|' character
  */
void AlgorithmDialog::setPresetValues(const QString & presetValues)
{
  if( presetValues.isEmpty() ) return;
  QStringList presets = presetValues.split('|', QString::SkipEmptyParts);
  QStringListIterator itr(presets);
  m_python_arguments.clear();
  while( itr.hasNext() )
  {
    QString namevalue = itr.next();
    QString name = namevalue.section('=', 0, 0);
    m_python_arguments.append(name);
    // Simplified removes trims from start and end and replaces all n counts of whitespace with a single whitespace
    QString value = namevalue.section('=', 1, 1).simplified();
    storePropertyValue(name, value.trimmed());
  }
  setPropertyValues();
}

/** 
 * Set comma-separated list of enabled parameter names
 * @param enabledNames A comma-separated list of parameter names to keep enabled
 */
void AlgorithmDialog::setEnabledNames(const QString & enabledNames)
{
  if( enabledNames.isEmpty() ) return;
  
  m_enabledNames = enabledNames.split(',', QString::SkipEmptyParts);
}

bool AlgorithmDialog::isInEnabledList(const QString& propName) const
{
  return m_enabledNames.contains(propName);
}

/**
 * Set if we are for a script or not
 * @param forScript A boolean inidcating whether we are being called from a script
 */
void AlgorithmDialog::isForScript(bool forScript)
{
  m_forScript = forScript;
}

/**
 * Set an optional message to be displayed at the top of the widget
 * @param message The message string
 */
void AlgorithmDialog::setOptionalMessage(const QString & message)
{
  m_strMessage = message;
  if( message.isEmpty() ) m_msgAvailable = false;
}

/**
 * This sets up the labels that are to be used to mark whether a property is valid. It has
 * a default implmentation but can be overridden if some other marker is required
 */ 
void AlgorithmDialog::createValidatorLabels()
{
  QHash<QString, Mantid::Kernel::Property*>::const_iterator pend = m_algProperties.end();
  for( QHash<QString, Mantid::Kernel::Property*>::const_iterator pitr = m_algProperties.begin();
       pitr != pend; ++pitr )
  {
    QLabel *validLbl = new QLabel("*");
    QPalette pal = validLbl->palette();
    pal.setColor(QPalette::WindowText, Qt::darkRed);
    validLbl->setPalette(pal);
    m_validators[pitr.key()] = validLbl;
  }
}

/**
 * Get a value from a widget. The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit and QCheckBox
 * @param widget A pointer to the widget
 */
QString AlgorithmDialog::getValue(QWidget *widget)
{
  if( QComboBox *opts = qobject_cast<QComboBox*>(widget) )
  {
    return opts->currentText();
  }
  else if( QLineEdit *textfield = qobject_cast<QLineEdit*>(widget) )
  {
    return textfield->text();
  }
  else if( QCheckBox *checker = qobject_cast<QCheckBox*>(widget) )
  {
    if( checker->checkState() == Qt::Checked )
    {
      return QString("1");
    }
    else
    {
      return QString("0");
    }
  }
  else
  {
    QMessageBox::warning(this, windowTitle(), 
			 QString("Cannot parse input from ") + widget->metaObject()->className() + 
			 ". Update AlgorithmDialog::getValue() to cope with this widget.");
    return "";
  }
}

/**
 * Set a value for a widget. The function needs to know about the types of widgets
 * that are being used. Currently it knows about QComboBox, QLineEdit and QCheckBox
 * @param widget A pointer to the widget
 * @param property The property name
 */
void AlgorithmDialog::setValue(QWidget *widget, const QString & propName)
{
  // Get the value from either the previous input store or from Python argument
  QString value("");
  Mantid::Kernel::Property *property = getAlgorithmProperty(propName);

  if( !isForScript() )
  {
    value = AlgorithmInputHistory::Instance().previousInput(m_algName, propName);
  }
  else
  {
    if( !property ) return;
    value = m_propertyValueMap.value(propName);
  }

  // Do the right thing for the widget type
  if( QComboBox *opts = qobject_cast<QComboBox*>(widget) )
  {
    if( property && value.isEmpty() )
    {
      value = QString::fromStdString(property->value());
    }
    int index = opts->findText(value);
    if( index >= 0 )
    {
      opts->setCurrentIndex(index);
    }
  }
  else if( QLineEdit *textfield = qobject_cast<QLineEdit*>(widget) )
  {
    if( !isForScript() )
    {
      textfield->setText(value);
    }
    else
    {
      //Need to check if this is the default value as we don't fill them in if they are
      if( m_python_arguments.contains(propName) || !property->isDefault() )
      {
	      textfield->setText(value);
      }
    }
  }
  else if( QCheckBox *checker = qobject_cast<QCheckBox*>(widget) )
  {
    if( value == "0" )
    {
      checker->setCheckState(Qt::Unchecked);
    }
    else
    {
      checker->setCheckState(Qt::Checked);
    }
    
  }
  else
  {
    QMessageBox::warning(this, windowTitle(), 
			 QString("Cannot set value for ") + widget->metaObject()->className() + 
			 ". Update AlgorithmDialog::setValue() to cope with this widget.");
  }
}
