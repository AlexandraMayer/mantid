#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_H_
    
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/VMD.h"

namespace Mantid
{
namespace Geometry
{

  /** MDHistoDimension :
   *
   * A very simple implementation of a IMDDimension, describing the
   * limits and number of bins in a dimension.
   * 
   * @author Janik Zikovsky
   * @date 2011-03-24 14:16:39.231608
   */
  class MANTID_GEOMETRY_DLL MDHistoDimension : public IMDDimension
  {
  public:

    /** Constructor for simple MDHistoDimension
    * @param name :: full name of the axis
    * @param ID :: identifier string
    * @param units :: string giving the units of this dimension
    * @param min :: minimum extent
    * @param max :: maximum extent
    * @param numBins :: number of bins (evenly spaced)
    */
    MDHistoDimension(std::string name, std::string ID, std::string units, double min, double max, size_t numBins)
    : m_name(name), m_dimensionId(ID), m_units(units),
      m_min(min), m_max(max), m_numBins(numBins),
      m_binWidth((max-min)/static_cast<double>(numBins))
    {
    }
    
    /// Destructor
    virtual ~MDHistoDimension()
    {}

    /// Return the name of the dimension as can be displayed along the axis
    virtual std::string getName() const
    {
      return m_name;
    }

    /// Return the units of the dimension as a string
    virtual std::string getUnits() const
    {
      return m_units;
    }

    /** Short name which identify the dimension among other dimension.
     * A dimension can be usually found by its ID and various
     * various method exist to manipulate set of dimensions by their names.
     */
    virtual std::string getDimensionId() const
    {
      return m_dimensionId;
    }

    /// Returns the maximum extent of this dimension
    virtual double getMaximum() const
    {
      return m_max;
    }

    /// Returns the minimum extent of this dimension
    virtual double getMinimum() const
    {
      return m_min;
    }

    /// number of bins dimension have (an integrated has one). A axis directed along dimension would have getNBins+1 axis points.
    virtual size_t getNBins() const
    {
      return m_numBins;
    }

    /// Dimensions must be xml serializable.
    virtual std::string toXMLString() const;

    ///  Get coordinate for index;
    virtual double getX(size_t index) const
    {
      return static_cast<double>(index) * m_binWidth + m_min;
    }

    /// Return the width of one bin.
    double getBinWidth() const
    {
      return m_binWidth;
    }

    /// @return the basis vector of this dimension as expressed in the dimensions of the original MDEventWorkspace.
    const Mantid::Kernel::VMD & getBasisVector() const
    { return m_basisVector; }

    /// Sets the basis vector of this dimension as expressed in the dimensions of the original MDEventWorkspace.
    void setBasisVector(const Mantid::Kernel::VMD & vector)
    { m_basisVector = vector; }


  private:
    /// Name
    std::string m_name;

    /// ID string
    std::string m_dimensionId;

    /// Dimension units
    std::string m_units;

    /// Extent of dimension
    double m_min;

    /// Extent of dimension
    double m_max;

    /// Number of bins
    size_t m_numBins;

    /// Calculated bin size
    double m_binWidth;

    /// Unit-length basis vector of this dimension as expressed in the dimensions of the original MDEventWorkspace.
    Mantid::Kernel::VMD m_basisVector;


  };


  /// Shared pointer to a MDHistoDimension
  typedef boost::shared_ptr<MDHistoDimension> MDHistoDimension_sptr;

  /// Shared pointer to a const MDHistoDimension
  typedef boost::shared_ptr<const MDHistoDimension> MDHistoDimension_const_sptr;

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_MDHISTODIMENSION_H_ */
