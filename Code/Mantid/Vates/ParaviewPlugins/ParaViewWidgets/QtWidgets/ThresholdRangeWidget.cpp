#include "ThresholdRangeWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QComboBox>
#include <QPalette>
#include <QFont>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

ThresholdRangeWidget::ThresholdRangeWidget(double min, double max) 
{
  QGridLayout* headerLayout = new QGridLayout();
  headerLayout->addWidget(new QLabel("Thresholds"), 0, 0, 1, 2, Qt::AlignCenter); 
  
  QGridLayout* layout = new QGridLayout;
  layout->addLayout(headerLayout, 0, 1);

  m_thresholdStrategyComboBox = new QComboBox;
  m_thresholdStrategyComboBox->addItem("Ignore Zeros");
  m_thresholdStrategyComboBox->addItem("No Threshold Range");
  m_thresholdStrategyComboBox->addItem("Median and Below");
  m_thresholdStrategyComboBox->addItem("User Defined");

  connect(m_thresholdStrategyComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(strategySelectedListener(const QString &)));
  layout->addWidget(m_thresholdStrategyComboBox, 1, 0, Qt::AlignLeft);

  m_minEditBox = new QLineEdit();
  std::string minValueString = boost::str(boost::format("%0.2f") % min);
  m_minEditBox->setText(minValueString.c_str());
  layout->addWidget(new QLabel("Min"), 1, 1, Qt::AlignLeft);
  layout->addWidget(m_minEditBox, 1, 2, Qt::AlignLeft);
  m_minEditBox->setDisabled(true); //Disabled by default.
  connect(m_minEditBox, SIGNAL(textEdited(const QString &)), this, SLOT(minThresholdListener(const QString &)));
 
  m_maxEditBox = new QLineEdit();
  std::string maxValueString = boost::str(boost::format("%0.2f") % max);
  m_maxEditBox->setText(maxValueString.c_str());
  m_maxEditBox->setDisabled(true); //Disabled by default
  layout->addWidget(new QLabel("Max"), 1, 3, Qt::AlignLeft);
  layout->addWidget(m_maxEditBox, 1, 4, Qt::AlignLeft);
  connect(m_maxEditBox, SIGNAL(textEdited(const QString &)), this, SLOT(maxThresholdListener(const QString &)));

  this->setLayout(layout);
}

void ThresholdRangeWidget::strategySelectedListener(const QString&)
{
  bool disableUserControls = true;
  if(m_thresholdStrategyComboBox->currentText() == "User Defined")
  {
    disableUserControls = false;
  }
  m_maxEditBox->setDisabled(disableUserControls);
  m_minEditBox->setDisabled(disableUserControls);

  emit chosenStrategyChanged();
}

void ThresholdRangeWidget::maxThresholdListener(const QString &)
{
  emit maxChanged();
}

void ThresholdRangeWidget::minThresholdListener(const QString &)
{
  emit minChanged();
}

void ThresholdRangeWidget::setMaximum(double value)
{
  std::string maxValueString = boost::str(boost::format("%0.2f") % value);
  m_maxEditBox->setText(maxValueString.c_str());
}

void ThresholdRangeWidget::setMinimum(double value)
{
  std::string minValueString = boost::str(boost::format("%0.2f") % value);
  m_minEditBox->setText(minValueString.c_str());
}

ThresholdRangeWidget::~ThresholdRangeWidget()
{
}

QString ThresholdRangeWidget::getMaxSignal() const
{
  return m_maxEditBox->text();
}

QString ThresholdRangeWidget::getMinSignal() const
{
  return m_minEditBox->text();
}

QString ThresholdRangeWidget::getChosenStrategy() const
{
  std::string minValueString = boost::str(boost::format("%i") % m_thresholdStrategyComboBox->currentIndex());
  return QString(minValueString.c_str());
}

