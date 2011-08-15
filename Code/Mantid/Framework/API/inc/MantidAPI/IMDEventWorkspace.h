#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ProgressBase.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace API
{

  /** Abstract base class for multi-dimension event workspaces (MDEventWorkspace).
   * This class will handle as much of the common operations as possible;
   * but since MDEventWorkspace is a templated class, that makes some aspects
   * impossible.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  class MANTID_API_DLL IMDEventWorkspace  : public API::Workspace
  {
  public:
    IMDEventWorkspace();
    IMDEventWorkspace(const IMDEventWorkspace & other);

    /// Perform initialization after dimensions (and others) have been set.
    virtual void initialize() = 0;

    /** Returns the number of dimensions in this workspace */
    virtual size_t getNumDims() const = 0;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const = 0;

    /// Add a new dimension
    virtual void addDimension(Mantid::Geometry::IMDDimension_sptr dimInfo);

    /// Get that dimension
    virtual Mantid::Geometry::IMDDimension_sptr getDimension(size_t dim) const;

    /// Get the minimum extents that hold the data
    virtual std::vector<Mantid::Geometry::MDDimensionExtents> getMinimumExtents(size_t depth=2) = 0;

    /// Returns some information about the box controller, to be displayed in the GUI, for example
    virtual std::vector<std::string> getBoxControllerStats() const = 0;

    /// @return true if the workspace is file-backed
    virtual bool isFileBacked() const = 0;

    /// Get the dimension index, searching by name
    virtual size_t getDimensionIndexByName(const std::string & name) const;


    ExperimentInfo_sptr getExperimentInfo(const uint16_t runIndex);

    ExperimentInfo_const_sptr getExperimentInfo(const uint16_t runIndex) const;

    uint16_t addExperimentInfo(ExperimentInfo_sptr ei);

    void setExperimentInfo(const uint16_t runIndex, ExperimentInfo_sptr ei);

    uint16_t getNumExperimentInfo() const;

  protected:
    /// Vector with each dimension (length must match nd)
    std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions;

    /// Vector for each ExperimentInfo class
    std::vector<ExperimentInfo_sptr> m_expInfos;

  };

  /// Shared pointer to a generic IMDEventWorkspace
  typedef boost::shared_ptr<IMDEventWorkspace> IMDEventWorkspace_sptr;

  /// Shared pointer to a generic const IMDEventWorkspace
  typedef boost::shared_ptr<const IMDEventWorkspace> IMDEventWorkspace_const_sptr;


}//namespace MDEvents

}//namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
