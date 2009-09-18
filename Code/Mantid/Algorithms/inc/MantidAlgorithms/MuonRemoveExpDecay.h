#ifndef MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_
#define MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**Takes a muon workspace as input and removes the exponential decay from a time channel.
	 This is done by multipling the data by exp(t/tmuon). 

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
    </UL>


    @author
    @date 11/07/2008

    Copyright &copy; 2008-9 STFC Rutherford Appleton Laboratory

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
    class DLLExport MuonRemoveExpDecay : public API::Algorithm
    {
    public:
      /// Default constructor
     MuonRemoveExpDecay(): API::Algorithm() {};
      /// Destructor
      virtual ~MuonRemoveExpDecay() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "RemoveExpDecay";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Muon";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();
      void removeDecay(const MantidVec& inX, const MantidVec& inY, MantidVec& outY);

      
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MUONREMOVEEXPDECAY_H_*/
