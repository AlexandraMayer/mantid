#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <map>
#include <memory>
#include <set>
#include <vector>

class QAbstractItemModel;

namespace MantidQt {
namespace MantidWidgets {

class DataProcessorCommand;
class DataProcessorWhiteList;

typedef std::map<int, std::map<int, std::vector<std::string>>> TreeData;

/** @class DataProcessorTreeManager

DataProcessorTreeManager is an abstract base class defining some methods meant
to be used by the Generic Data Processor presenter, which will delegate some
functionality to concrete tree manager implementations, depending on whether or
not a post-processing algorithm has been defined.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DataProcessorTreeManager {
public:
  virtual ~DataProcessorTreeManager(){};

  /// Actions/commands

  /// Publish actions/commands
  virtual std::vector<std::unique_ptr<DataProcessorCommand>>
  publishCommands() = 0;
  /// Append a row
  virtual void appendRow() = 0;
  /// Append a group
  virtual void appendGroup() = 0;
  /// Delete a row
  virtual void deleteRow() = 0;
  /// Delete a group
  virtual void deleteGroup() = 0;
  /// Group rows
  virtual void groupRows() = 0;
  /// Expand selection
  virtual std::set<int> expandSelection() = 0;
  /// Clear selected
  virtual void clearSelected() = 0;
  /// Copy selected
  virtual std::string copySelected() = 0;
  /// Paste selected
  virtual void pasteSelected(const std::string &text) = 0;
  /// Blank table
  virtual void newTable(const DataProcessorWhiteList &whitelist) = 0;
  /// Blank table
  virtual void newTable(Mantid::API::ITableWorkspace_sptr table,
                        const DataProcessorWhiteList &whitelist) = 0;

  /// Read/write data

  /// Return selected data
  virtual TreeData selectedData(bool prompt = false) = 0;
  /// Transfer new data to model
  virtual void
  transfer(const std::vector<std::map<std::string, std::string>> &runs,
           const DataProcessorWhiteList &whitelist) = 0;
  /// Update row with new data
  virtual void update(int parent, int child,
                      const std::vector<std::string> &data) = 0;

  /// Validate a table workspace
  virtual bool isValidModel(Mantid::API::Workspace_sptr ws,
                            size_t whitelistColumns) const = 0;

  /// Return member variables

  /// Return the model
  virtual boost::shared_ptr<QAbstractItemModel> getModel() = 0;
  /// Return the table ws
  virtual Mantid::API::ITableWorkspace_sptr getTableWorkspace() = 0;

protected:
  /// Add a command to the list of available commands
  void addCommand(std::vector<std::unique_ptr<DataProcessorCommand>> &commands,
                  std::unique_ptr<DataProcessorCommand> command) {
    commands.push_back(std::move(command));
  }
};
}
}
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEMANAGER_H */
