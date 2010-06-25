#ifndef MANTID_ALGORITHMS_SOLIDANGLECORRECTION_H_
#define MANTID_ALGORITHMS_SOLIDANGLECORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/**

    Performs a solid angle correction on a 2D SANS data set to correct
    for the absence of curvature of the detector.

    Brulet et al, J. Appl. Cryst. (2007) 40, 165-177.
    See equation 22.

    Required Properties:
    <UL>
    <LI> InputWorkspace    - The data in units of wavelength. </LI>
    <LI> OutputWorkspace   - The workspace in which to store the result histogram. </LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SolidAngleCorrection : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SolidAngleCorrection() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SolidAngleCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SolidAngleCorrection"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RADIALAVERAGE_H_*/
