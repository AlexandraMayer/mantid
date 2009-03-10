#ifndef MANTID_API_ALGORITHMMANAGER_H_
#define MANTID_API_ALGORITHMMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <deque>
#include <string>
#include "MantidAPI/DllExport.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmFactory.h"

namespace Mantid
{
namespace API
{
/** The AlgorithmManagerImpl class is responsible for controlling algorithm 
    instances. It incorporates the algorithm factory and initializes algorithms.

    @author Dickon Champion, ISIS, RAL
    @date 30/10/2007

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTID_API AlgorithmManagerImpl
{
public:
  // Methods to create algorithm instances
  IAlgorithm_sptr create(const std::string& algName, const int& version = -1);
  IAlgorithm_sptr createUnmanaged(const std::string& algName, const int& version = -1) const;

  /// deletes all registered algorithms
  void clear();

  /** Gives the number of managed algorithms
   *  @return The number of registered algorithms
   */
  int size() const
  {
    return regAlg.size();
  }

  const std::vector<std::pair<std::string, std::string> > getNamesAndCategories() const;
  
  /// Returns a reference to the container of managed algorithms
  const std::deque<IAlgorithm_sptr>& algorithms() const
  {
    return regAlg;
  }
  
  IAlgorithm_sptr getAlgorithm(AlgorithmID id) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmManagerImpl>;

  ///Class cannot be instantiated by normal means
  AlgorithmManagerImpl();
  ///destructor
  ~AlgorithmManagerImpl();
  ///Copy constructor
  AlgorithmManagerImpl(const AlgorithmManagerImpl&);

  /// Standard Assignment operator    
  AlgorithmManagerImpl& operator =(const AlgorithmManagerImpl&);

  /// Reference to the logger class
  Kernel::Logger& g_log;

  int max_no_algs; ///< The maximum size of the algorithm store
  std::deque<IAlgorithm_sptr> regAlg; ///<  pointers to registered algorithms [policy???]
};

///Forward declaration of a specialisation of SingletonHolder for AlgorithmManagerImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl>;
#endif /* _WIN32 */
typedef Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl> AlgorithmManager;

} // namespace API
} //Namespace Mantid

#endif /* MANTID_API_ALGORITHMMANAGER_H_ */
