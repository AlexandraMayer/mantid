#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox() :
    IMDBox<MDE, nd>()
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** ctor
   * @param splitter :: BoxSplitController that controls how boxes split
   */
  TMDE(MDBox)::MDBox(BoxSplitController_sptr splitter)
  {
    m_splitController = splitter;
  }

  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDBox)::getNPoints() const
  {
    return data.size();
  }

  //-----------------------------------------------------------------------------------------------
  /** Set the extents of this box.
   * @param dim :: index of dimension
   * @param min :: min edge of the dimension
   * @param max :: max edge of the dimension
   */
  TMDE(
  void MDBox)::setExtents(size_t dim, CoordType min, CoordType max)
  {
    if (dim >= nd)
      throw std::invalid_argument("Invalid dimension passed to MDBox::setExtents");
    this->extents[dim].min = min;
    this->extents[dim].max = min;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   */
  TMDE(
  std::vector< MDE > & MDBox)::getEvents()
  {
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    out->insert(out->begin(), data.begin(), data.end());
    return out;
  }




  //-----------------------------------------------------------------------------------------------
  /** Add a MDEvent to the box.
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    this->data.push_back(event);

    // Keep the running total of signal and error
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();
  }


  //-----------------------------------------------------------------------------------------------
  /** Return true if the box would need to split into a MDGridBox to handle the new events
   * @param num :: number of events that would be added
   */
  TMDE(
  bool MDBox)::willSplit(size_t num) const
  {
    if (!m_splitController)
      return false;
    return m_splitController->willSplit(this->data.size(), num);
  }


  //-----------------------------------------------------------------------------------------------
  /** Add several events
   * @param events :: vector of events to be copied.
   */
  TMDE(
  void MDBox)::addEvents(const std::vector<MDE> & events)
  {
    this->data.insert(this->data.end(), events.begin(), events.end());
  }








  template DLLExport class MDBox<MDEvent<1>, 1>;
  template DLLExport class MDBox<MDEvent<2>, 2>;
  template DLLExport class MDBox<MDEvent<3>, 3>;
  template DLLExport class MDBox<MDEvent<4>, 4>;
  template DLLExport class MDBox<MDEvent<5>, 5>;
  template DLLExport class MDBox<MDEvent<6>, 6>;
  template DLLExport class MDBox<MDEvent<7>, 7>;
  template DLLExport class MDBox<MDEvent<8>, 8>;
  template DLLExport class MDBox<MDEvent<9>, 9>;


}//namespace MDEvents

}//namespace Mantid

