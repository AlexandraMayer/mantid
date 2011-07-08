#include "MantidTable.h"
#include "../ApplicationWindow.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"

#include <QMessageBox>
#include <iostream>

using namespace MantidQt::API;

//------------------------------------------------------------------------------------------------
/** Create MantidTable out of a ITableWorkspace
 *
 * @param env :: scripting environment (?)
 * @param ws :: ITableWorkspace to reproduce
 * @param label
 * @param parent :: parent window
 * @param name
 * @param f :: flags (?)
 * @return the MantidTable created
 */
MantidTable::MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, const QString& /*name*/, Qt::WFlags f):
Table(env,ws->rowCount(),ws->columnCount(),label,parent,"",f),
m_ws(ws),
m_wsName(ws->getName())
{
  // Set name and stuff
  parent->initTable(this, parent->generateUniqueName("Table-"));
  //  askOnCloseEvent(false);
  // Fill up the view
  this->fillTable();

  connect(this,SIGNAL(needToClose()),this,SLOT(closeTable()));
  connect(this,SIGNAL(needToUpdate()),this,SLOT(fillTable()));
  observeDelete();
  observeAfterReplace();
}

//------------------------------------------------------------------------------------------------
/** Refresh the table by filling it */
void MantidTable::fillTable()
{
  setNumCols(m_ws->columnCount());
  setNumRows(m_ws->rowCount());

  // Add all columns
  for(int i=0;i<m_ws->columnCount();i++)
  {
    Mantid::API::Column_sptr c = m_ws->getColumn(i);
    QString colName = QString::fromStdString(c->name());
    setColName(i,colName);
    // Make columns of ITableWorkspaces read only, if specified
    setReadOnlyColumn(i, c->getReadOnly() );
    // Special for errors?
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      setColPlotDesignation(i,Table::yErr);
    }
    // Print out the data in each row of this column
    for(int j=0; j<m_ws->rowCount(); j++)
    {
      std::ostringstream ostr;
      // This is the method on the Column object to convert to a string.
      c->print(ostr,j);
      setText(j,i,QString::fromStdString(ostr.str()));
      d_table->verticalHeader()->setLabel(j,QString::number(j));
    }
  }

}

//------------------------------------------------------------------------------------------------
void MantidTable::closeTable()
{
  askOnCloseEvent(false);
  close();
}

//------------------------------------------------------------------------------------------------
void MantidTable::deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::ITableWorkspace* ws_ptr = dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get());
  if (!ws_ptr) return;
  if (ws_ptr == m_ws.get() || wsName == m_wsName)
  {
    emit needToClose();
  }
}

//------------------------------------------------------------------------------------------------
void MantidTable::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::ITableWorkspace_sptr new_ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(ws);
  if (!new_ws) return;
  if (new_ws.get() == m_ws.get() || wsName == m_wsName)
  {
    m_ws = new_ws;
    emit needToUpdate();
  }
}


//------------------------------------------------------------------------------------------------
/** Called when a cell is edited */
void MantidTable::cellEdited(int row,int col)
{
  std::string text = d_table->text(row,col).remove(QRegExp("\\s")).toStdString();
  Mantid::API::Column_sptr c = m_ws->getColumn(col);

  // Have the column convert the text to a value internally
  int index = row;
  c->read(text, index);

  // Set the table view to be the same text after editing.
  // That way, if the string was stupid, it will be reset to the old value.
  std::ostringstream s;
  c->print(s, index);
  d_table->setText(row, col, QString(s.str().c_str()));
}


//------------------------------------------------------------------------------------------------
/** Call an algorithm in order to delete table rows
 *
 * @param startRow :: row index (starts at 1) to start to remove
 * @param endRow :: end row index, inclusive, to remove
 */
void MantidTable::deleteRows(int startRow, int endRow)
{
  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("DeleteTableRows");
  alg->setPropertyValue("TableWorkspace",m_ws->getName());
  QStringList rows;
  rows << QString::number(startRow - 1) << QString::number(endRow - 1);
  alg->setPropertyValue("Rows",rows.join("-").toStdString());
  try
  {
    alg->execute();
  }
  catch(...)
  {
    QMessageBox::critical(this,"MantidPlot - Error", "DeleteTableRow algorithm failed");
  }
}
