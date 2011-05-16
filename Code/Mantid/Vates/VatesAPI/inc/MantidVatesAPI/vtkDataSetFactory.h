

#ifndef MANTID_VATES_VTKDATASETFACTORY_H_
#define MANTID_VATES_VTKDATASETFACTORY_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include "vtkDataSet.h"

class vtkFloatArray;
namespace Mantid
{
  namespace API
  {
    class IMDWorkspace;
  }

namespace VATES
{

  /* Helper struct allows recognition of points that we should not bother to draw.
  */
  struct UnstructuredPoint
  {
    bool isSparse;
    vtkIdType pointId;
  };

/** Abstract type to generate a vtk dataset on demand from a MDWorkspace.
 Uses Chain Of Responsibility pattern to self-manage and ensure that the workspace rendering is delegated to another factory
 if the present concrete type can't handle it.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport vtkDataSetFactory
{

public:

  /// Constructor
  vtkDataSetFactory();

  /// Destructor
  virtual ~vtkDataSetFactory()=0;

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkDataSet* create() const=0;
  
  /// Create as a mesh only.
  virtual vtkDataSet* createMeshOnly() const=0;

  /// Create the scalar array only.
  virtual vtkFloatArray* createScalarArray() const=0;

  /// Initalize with a target workspace.
  virtual void initialize(boost::shared_ptr<Mantid::API::IMDWorkspace>)=0;

  /// Add a chain-of-responsibility successor to this factory. Handle case where the factory cannot render the MDWorkspace owing to its dimensionality.
  virtual void SetSuccessor(vtkDataSetFactory* pSuccessor);

  /// Determine whether a successor factory has been provided.
  virtual bool hasSuccessor() const;

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const =0;

protected:

  /// Typedef for internal unique shared pointer for successor types.
  typedef boost::shared_ptr<vtkDataSetFactory> SuccessorType;

  vtkDataSetFactory::SuccessorType m_successor;

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const = 0;

};

typedef boost::shared_ptr<vtkDataSetFactory> vtkDataSetFactory_sptr;

}
}


#endif
