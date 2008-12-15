#ifndef MANTID_ALGORITHMS_CROSSCORRELATE_H_
#define MANTID_ALGORITHMS_CROSSCORRELATE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace Algorithms
{
/** Compute the cross correlation function for a range of spectra with respect to a reference spectrum.
 * This is use in powder diffraction experiments when trying to estimate the offset of one spectra
 * with respect to another one. The spectra are converted in d-spacing and then interpolate
 * on the X-axis of the reference. The cross correlation function is computed in the range [-N/2,N/2]
 * where N is the number of points.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    <LI> ReferenceSpectra - The spectra number against which cross-correlation function is computed.</LI>
    <LI> Spectra_min  - Lower bound of the spectra range for which cross-correlation is computed.</LI>
    <LI> Spectra_max - Upper bound of the spectra range for which cross-correlation is computed.</LI>
    </UL>

    @author Laurent C Chapon, ISIS Facility Rutherford Appleton Laboratory
    @date 15/12/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport CrossCorrelate : public API::Algorithm
{
public:
	#ifndef HAS_UNORDERED_MAP_H
	typedef std::map<int,int> spec2index_map;
	#else
	typedef std::tr1::unordered_map<int,int> spec2index_map;
	#endif
  /// (Empty) Constructor
  CrossCorrelate() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~CrossCorrelate() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CrossCorrelate"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
  /// Spectra to index map
  spec2index_map index_map;
  spec2index_map::iterator index_map_it;
};

// Functor for vector sum
struct sumV : public std::unary_function<double, void>
{
	sumV():sum(0){}
	void operator()(double data) {sum+=data;}
	double sum;
};
// Functor for computing variance
struct varV : public std::unary_function<double, void>
{
	varV(double mean_):var(0),mean(mean_){}
	void operator()(double data)
	{
		temp=(data-mean);
		var+=(temp*temp);
	}
	double var, mean, temp;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CROSSCORRELATE_H_*/
