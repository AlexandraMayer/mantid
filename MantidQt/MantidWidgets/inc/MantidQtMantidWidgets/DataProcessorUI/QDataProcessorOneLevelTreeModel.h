#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <QAbstractItemModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** QDataProcessorOneLevelTreeModel : Provides a QAbstractItemModel for a
DataProcessorUI with no post-processing defined. The first argument to the
constructor is a Mantid ITableWorkspace containing the values to use in the
reduction. Each row in the table corresponds to an independent reduction. The
second argument is a WhiteList containing the header of the model, i.e. the name
of the columns that will be displayed in the tree. The table workspace must have
the same number of columns as the number of items in the WhiteList.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS QDataProcessorOneLevelTreeModel
    : public QAbstractItemModel {
  Q_OBJECT
public:
  QDataProcessorOneLevelTreeModel(
      Mantid::API::ITableWorkspace_sptr tableWorkspace,
      const DataProcessorWhiteList &whitelist);
  ~QDataProcessorOneLevelTreeModel() override;

  // Functions to read data from the model

  // Get flags for a cell
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // Get data for a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // Get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // Row count
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  // Column count
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  // Get the index for a given column, row and parent
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;

  // Get the parent
  QModelIndex parent(const QModelIndex &index) const override;

  // Functions to edit the model

  // Change or add data to the model
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  // Add new rows to the model
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // Remove rows from the model
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;

private:
  /// Collection of data for viewing.
  Mantid::API::ITableWorkspace_sptr m_tWS;
  /// Map of column indexes to names and viceversa
  DataProcessorWhiteList m_whitelist;
};

/// Typedef for a shared pointer to \c QDataProcessorOneLevelTreeModel
typedef boost::shared_ptr<QDataProcessorOneLevelTreeModel>
    QDataProcessorOneLevelTreeModel_sptr;

} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_ */
