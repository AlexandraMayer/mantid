#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Templated class for the multi-dimensional event workspace.
   *
   * Template parameters are the same as the MDPoint<> template params, and
   * determine the basic MDPoint type used.
   *
   * @tparam nd :: the number of dimensions that each MDPoint will be tracking.
   *               an usigned int > 0.
   *
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDEventWorkspace  : public API::IMDEventWorkspace
  {
  public:
    MDEventWorkspace();
    virtual ~MDEventWorkspace();

    /// Perform initialization after dimensions (and others) have been set.
    virtual void initialize();

    virtual const std::string id() const;

    /** Returns the number of dimensions in this workspace */
    virtual size_t getNumDims() const;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const;

    /** Returns the number of bytes of memory
     * used by the workspace. */
    virtual size_t getMemorySize() const;

    void setBoxController(BoxController_sptr controller);

    /// Returns the BoxController used in this workspace
    BoxController_sptr getBoxController()
    {
      return m_BoxController;
    }

    virtual std::vector<std::string> getBoxControllerStats() const;

    void splitBox();

    void splitAllIfNeeded(Kernel::ThreadScheduler * ts);

    void refreshCache();

    /** Sample function returning (a copy of) the n-th event in the workspace.
     * This may not be needed.
     *  */
    MDE getEvent(size_t n);

    void addEvent(const MDE & event);

    void addEvents(const std::vector<MDE> & events);

    void addManyEvents(const std::vector<MDE> & events, Mantid::Kernel::ProgressBase * prog);

    Mantid::API::IMDWorkspace_sptr centerpointBinToMDHistoWorkspace(
        std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions,
        Mantid::API::ImplicitFunction *_implicitFunction,
        Mantid::Kernel::ProgressBase * prog) const;

    /// Return true if the underlying box is a MDGridBox.
    bool isGridBox()
    {
      return (dynamic_cast<MDGridBox<MDE,nd> *>(data) != NULL);
    }

    /** Returns a pointer to the box (MDBox or MDGridBox) contained within, */
    IMDBox<MDE,nd> * getBox()
    {
      return data;
    }

    /** Returns a pointer to the box (MDBox or MDGridBox) contained within, const version.  */
    const IMDBox<MDE,nd> * getBox() const
    {
      return data;
    }


  protected:

    /** MDBox containing all of the events in the workspace. */
    IMDBox<MDE, nd> * data;

    /// Box controller in use
    BoxController_sptr m_BoxController;

  public:
    /// Typedef for a shared pointer of this kind of event workspace
    typedef boost::shared_ptr<MDEventWorkspace<MDE, nd> > sptr;


  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
