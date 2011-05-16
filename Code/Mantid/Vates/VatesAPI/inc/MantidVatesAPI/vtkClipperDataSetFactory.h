#ifndef MANTID_VATES_VTKCLIPPERDATASETFACTORY_H
#define MANTID_VATES_VTKCLIPPERDATASETFACTORY_H

#include "MantidAPI/ImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/Clipper.h"
#include "MantidAPI/IMDWorkspace.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace VATES
{

/** Factory method implementation. This is an effective vtkAlgorithm in that it uses clipping functions to slice a vtkDataSet based on a provided implicit function.

 @author Owen Arnold, Tessella plc
 @date 07/02/2010

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


class DLLExport vtkClipperDataSetFactory : public vtkDataSetFactory
{
public:
  /// Constructor
  vtkClipperDataSetFactory(boost::shared_ptr<Mantid::API::ImplicitFunction> implicitFunction, vtkDataSet* dataSet, Clipper* clipper);

  /// Destructor
  virtual ~vtkClipperDataSetFactory();

  /// Factory Method.
  virtual vtkDataSet* create() const;

  /// Initialize the factory.
  virtual void initialize(Mantid::API::IMDWorkspace_sptr workspace);

  vtkDataSet* createMeshOnly() const;

  vtkFloatArray* createScalarArray() const;

  void validate() const
  {
  }

  virtual std::string getFactoryTypeName() const
  {
    return "vtkClipperDataSetFactory";
  }

private:

  /// Disabled copy constructor.
  vtkClipperDataSetFactory(const vtkClipperDataSetFactory&);

  /// Disabled assignment operator.
  vtkClipperDataSetFactory& operator=(const vtkClipperDataSetFactory&);

  /// Function describing clipping.
  boost::shared_ptr<Mantid::API::ImplicitFunction> m_implicitFunction;

  /// Dataset on which to apply clipping.
  vtkDataSet* m_dataset;

  boost::scoped_ptr<Clipper> m_clipper;
};

}
}

#endif
