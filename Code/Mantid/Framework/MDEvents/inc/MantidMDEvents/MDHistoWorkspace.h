#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACE_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/IMDHistoWorkspace.h"

using Mantid::DataObjects::WorkspaceSingleValue;


namespace Mantid
{
namespace MDEvents
{

  /** MDHistoWorkspace:
  *
  * An implementation of IMDWorkspace that contains a (normally dense) histogram
  * representation in up to 4 dimensions.
  *
  * This will be the result of a slice or rebin of another workspace, e.g. a
  * MDEventWorkspace.
  *
  * This will be used by ParaView e.g. for visualization.
  *
  * @author Janik Zikovsky
  * @date 2011-03-24 11:21:06.280523
  */
  class DLLExport MDHistoWorkspace : public API::IMDHistoWorkspace
  {
  public:
    MDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX,
        Mantid::Geometry::MDHistoDimension_sptr dimY=Mantid::Geometry::MDHistoDimension_sptr(),
        Mantid::Geometry::MDHistoDimension_sptr dimZ=Mantid::Geometry::MDHistoDimension_sptr(),
        Mantid::Geometry::MDHistoDimension_sptr dimT=Mantid::Geometry::MDHistoDimension_sptr());

    MDHistoWorkspace(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions);

    MDHistoWorkspace(const MDHistoWorkspace & other);

    virtual ~MDHistoWorkspace();

    void init(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions);

    virtual const std::string id() const
    { return "MDHistoWorkspace"; }

    virtual size_t getMemorySize() const;

    /// Get the number of points (bins in this case) associated with the workspace;
    uint64_t getNPoints() const
    {
      return m_length;
    }

    /// Creates a new iterator pointing to the first cell in the workspace
    virtual Mantid::API::IMDIterator* createIterator(Mantid::Geometry::MDImplicitFunction * function = NULL) const;

    void checkWorkspaceSize(const MDHistoWorkspace & other, std::string operation);

    // --------------------------------------------------------------------------------------------
    MDHistoWorkspace & operator+=(const MDHistoWorkspace & b);
    void add(const MDHistoWorkspace & b);
    void add(const signal_t signal, const signal_t error);

    MDHistoWorkspace & operator-=(const MDHistoWorkspace & b);
    void subtract(const MDHistoWorkspace & b);
    void subtract(const signal_t signal, const signal_t error);

    MDHistoWorkspace & operator*=(const MDHistoWorkspace & b);
    void multiply(const MDHistoWorkspace & b);
    void multiply(const signal_t signal, const signal_t error);

    MDHistoWorkspace & operator/=(const MDHistoWorkspace & b);
    void divide(const MDHistoWorkspace & b);
    void divide(const signal_t signal, const signal_t error);

    void log(double filler = 0.0);
    void log10(double filler = 0.0);
    void exp();
    void power(double exponent);

    // --------------------------------------------------------------------------------------------
    MDHistoWorkspace & operator&=(const MDHistoWorkspace & b);
    MDHistoWorkspace & operator|=(const MDHistoWorkspace & b);
    MDHistoWorkspace & operator^=(const MDHistoWorkspace & b);
    void operatorNot();

    void lessThan(const MDHistoWorkspace & b);
    void lessThan(const signal_t signal);
    void greaterThan(const MDHistoWorkspace & b);
    void greaterThan(const signal_t signal);
    void equalTo(const MDHistoWorkspace & b, const signal_t tolerance=1e-5);
    void equalTo(const signal_t signal, const signal_t tolerance=1e-5);

    void setUsingMask(const MDHistoWorkspace & mask, const MDHistoWorkspace & values);
    void setUsingMask(const MDHistoWorkspace & mask, const signal_t signal, const signal_t error);



    // --------------------------------------------------------------------------------------------
    /** @return a const reference to the indexMultiplier array.
     * To find the index into the linear array, dim0 + indexMultiplier[0]*dim1 + ...
     */
    const size_t * getIndexMultiplier() const
    {
      return indexMultiplier;
    }

    /** @return the direct pointer to the signal array. For speed */
    signal_t * getSignalArray()
    {
      return m_signals;
    }

    /** @return the inverse of volume of EACH cell in the workspace. For normalizing. */
    coord_t getInverseVolume() const
    {
      return m_inverseVolume;
    }

    /** @return the direct pointer to the error squared array. For speed */
    signal_t * getErrorSquaredArray()
    {
      return m_errorsSquared;
    }

    void setTo(signal_t signal, signal_t errorSquared);

    void applyImplicitFunction(Mantid::Geometry::MDImplicitFunction * function, signal_t signal, signal_t errorSquared);

    coord_t * getVertexesArray(size_t linearIndex, size_t & numVertices) const;

    Mantid::Kernel::VMD getCenter(size_t linearIndex) const;

    /// Returns the (normalized) signal at a given coordinates
    signal_t getSignalAtCoord(const coord_t * coords) const;

    /// Sets the signal at the specified index.
    void setSignalAt(size_t index, signal_t value)
    {
      m_signals[index] = value;
    }

    /// Sets the error (squared) at the specified index.
    void setErrorSquaredAt(size_t index, signal_t value)
    {
      m_errorsSquared[index] = value;
    }


    /// Get the error (squared) of the signal at the specified index.
    signal_t getErrorAt(size_t index) const
    {
      return m_errorsSquared[index];
    }

    /// Get the error at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getErrorAt(size_t index1, size_t index2) const
    {
      return m_errorsSquared[index1 + indexMultiplier[0]*index2];
    }

    /// Get the error at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getErrorAt(size_t index1, size_t index2, size_t index3) const
    {
      return m_errorsSquared[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3];
    }

    /// Get the error at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getErrorAt(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      return m_errorsSquared[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3 + indexMultiplier[2]*index4];
    }



    /// Get the signal at the specified index.
    signal_t getSignalAt(size_t index) const
    {
      return m_signals[index];
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getSignalAt(size_t index1, size_t index2) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2];
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getSignalAt(size_t index1, size_t index2, size_t index3) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3];
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
    signal_t getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3 + indexMultiplier[2]*index4];
    }




    /// Get the signal at the specified index, normalized by cell volume
    signal_t getSignalNormalizedAt(size_t index) const
    {
      return m_signals[index] * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getSignalNormalizedAt(size_t index1, size_t index2) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2] * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3] * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      return m_signals[index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3 + indexMultiplier[2]*index4] * m_inverseVolume;
    }



    /// Get the error of the signal at the specified index, normalized by cell volume
    signal_t getErrorNormalizedAt(size_t index) const
    {
      return m_errorsSquared[index] * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getErrorNormalizedAt(size_t index1, size_t index2) const
    {
      return getErrorAt(index1,index2) * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3) const
    {
      return getErrorAt(index1,index2,index3) * m_inverseVolume;
    }

    /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
    signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      return getErrorAt(index1,index2,index3,index4) * m_inverseVolume;
    }

    //---------------------------------------------------------------------------------------------
    /** @return a reference to the error (squared) at the linear index
     * @param index :: linear index (see getLinearIndex).  */
    signal_t & errorSquaredAt(size_t index)
    {
      if (index < m_length)
        return m_errorsSquared[index];
      else
        throw std::invalid_argument("MDHistoWorkspace::array index out of range");
    }

    /** @return a reference to the signal at the linear index
     * @param index :: linear index (see getLinearIndex).  */
    signal_t & signalAt(size_t index)
    {
      if (index < m_length)
        return m_signals[index];
      else
        throw std::invalid_argument("MDHistoWorkspace::array index out of range");
    }

    //---------------------------------------------------------------------------------------------
    size_t getLinearIndex(size_t index1, size_t index2) const
    {
      if (this->getNumDims() != 2)
        throw std::runtime_error("Workspace does not have 2 dimensions!");
      return index1 + indexMultiplier[0]*index2;
    }

    size_t getLinearIndex(size_t index1, size_t index2, size_t index3) const
    {
      if (this->getNumDims() != 3)
        throw std::runtime_error("Workspace does not have 3 dimensions!");
      return index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3;
    }

    size_t getLinearIndex(size_t index1, size_t index2, size_t index3, size_t index4) const
    {
      if (this->getNumDims() != 4)
        throw std::runtime_error("Workspace does not have 4 dimensions!");
      return index1 + indexMultiplier[0]*index2 + indexMultiplier[1]*index3 + indexMultiplier[2]*index4;
    }



    /** Array subscript operator
     * @param index :: linear index into array
     * @return the signal (not normalized) at that index.
     */
    signal_t& operator [](const size_t & index)
    {
      if (index < m_length)
        return m_signals[index];
      else
        throw std::invalid_argument("MDHistoWorkspace::array index out of range");
    }



    /// Return a vector containing a copy of the signal data in the workspace. TODO: Make this more efficient if needed.
    virtual std::vector<signal_t> getSignalDataVector() const;
    virtual std::vector<signal_t> getErrorDataVector() const;




  private:

    void initVertexesArray();

    /// Number of dimensions in this workspace
    size_t numDimensions;

    /// Linear array of signals for each bin
    signal_t * m_signals;

    /// Linear array of errors for each bin
    signal_t * m_errorsSquared;

    /// Length of the m_signals / m_errorsSquared arrays.
    size_t m_length;

    /// To find the index into the linear array, dim0 + indexMultiplier[0]*dim1 + ...
    size_t * indexMultiplier;

    /// Inverse of the volume of EACH cell
    coord_t m_inverseVolume;

    /// Pre-calculated vertexes array for the 0th box
    coord_t * m_vertexesArray;

    /// Vector of the length of the box in each dimension
    coord_t * m_boxLength;

    /// Vector of the origin in each dimension
    coord_t * m_origin;

    /// For converting to/from linear index to tdimensions
    size_t * m_indexMaker;
    /// Max index into each dimension
    size_t * m_indexMax;

  };

  /// A shared pointer to a MDHistoWorkspace
  typedef boost::shared_ptr<MDHistoWorkspace> MDHistoWorkspace_sptr;

  /// A shared pointer to a const MDHistoWorkspace
  typedef boost::shared_ptr<const MDHistoWorkspace> MDHistoWorkspace_const_sptr;

} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDHISTOWORKSPACE_H_ */
