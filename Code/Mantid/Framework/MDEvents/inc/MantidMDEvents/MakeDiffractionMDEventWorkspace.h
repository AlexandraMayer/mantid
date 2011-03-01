#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace MDEvents
{

  /** MakeDiffractionMDEventWorkspace : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-03-01 13:14:48.236513
   */
  class DLLExport MakeDiffractionMDEventWorkspace  : public API::Algorithm
  {
  public:
    MakeDiffractionMDEventWorkspace();
    ~MakeDiffractionMDEventWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "MakeDiffractionMDEventWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}
    
  private:
    void init();
    void exec();


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
