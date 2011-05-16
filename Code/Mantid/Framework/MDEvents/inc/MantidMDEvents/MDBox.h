#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidMDEvents/MDDimensionStats.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Templated class for a multi-dimensional event "box".
   *
   * A box is a container of MDEvent's within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * This class is a simple list of points with no more internal structure.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDBox : public IMDBox<MDE, nd>
  {
  public:
    MDBox();

    MDBox(BoxController_sptr splitter, const size_t depth = 0);

    virtual ~MDBox() {}

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    std::vector< MDE > & getEvents();

    std::vector< MDE > * getEventsCopy();

    void addEvent(const MDE & point);

    size_t addEvents(const std::vector<MDE> & events)
    {
      return this->addEvents(events, 0, events.size());
    }

    size_t addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void calculateDimensionStats(MDDimensionStats * stats) const;

    void integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const;

  protected:

    /** Vector of MDEvent's, in no particular order.
     * */
    std::vector< MDE > data;

    /// Mutex for modifying the event list
    Mantid::Kernel::Mutex dataMutex;


  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr< MDBox<MDE, nd> > sptr;

    /// Typedef for a vector of the conatined events
    typedef std::vector< MDE > vec_t;

  };







}//namespace MDEvents

}//namespace Mantid

#endif /* MDBOX_H_ */
