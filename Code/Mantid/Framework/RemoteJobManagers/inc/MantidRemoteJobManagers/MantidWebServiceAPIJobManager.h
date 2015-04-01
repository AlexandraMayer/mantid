#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGER_H
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGER_H

#include "MantidKernel/IRemoteJobManager.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIHelper.h"

namespace Mantid {
namespace RemoteJobManagers {
/**
MantidWebServiceAPIJobManager implements a remote job manager that
knows how to talk to the Mantid web service / job submission API
(http://www.mantidproject.org/Remote_Job_Submission_API). This is
being used for example for the Fermi cluster at SNS.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MantidWebServiceAPIJobManager
    : public Mantid::Kernel::IRemoteJobManager {
public:
  void authenticate(std::string &username, std::string &password);

  std::string
  submitRemoteJob(const std::string &transactionID, const std::string &runnable,
                  const std::string &param, const std::string &taskName = "",
                  const int numNodes = 1, const int coresPerNode = 1);

  void downloadRemoteFile(const std::string &transactionID,
                          const std::string &remoteFileName,
                          const std::string &localFileName);

  std::vector<Mantid::Kernel::IRemoteJobManager::RemoteJobInfo>
  queryAllRemoteJobs() const;

  std::vector<std::string>
  queryRemoteFile(const std::string &transactionID) const;

  Mantid::Kernel::IRemoteJobManager::RemoteJobInfo
  queryRemoteJob(const std::string &jobID) const;

  std::string startRemoteTransaction();

  void stopRemoteTransaction(const std::string &transactionID);

  void abortRemoteJob(const std::string &jobID);

  void uploadRemoteFile(const std::string &transactionID,
                        const std::string &remoteFileName,
                        const std::string &localFileName);

private:
  MantidWebServiceAPIHelper m_helper;
};

} // namespace RemoteJobManagers
} // namespace Mantid

#endif // MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGER_H
