#ifndef MANTID_ALGORITHM_FINDDEADDETECTORS_H_
#define MANTID_ALGORITHM_FINDDEADDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**
    Takes a workspace as input and identifies all of the spectra that have a
    value across all time bins less or equal to than the threshold 'dead' value.
    This is then used to mark all 'dead' detectors with a 'dead' marker value,
    while all spectra from live detectors are given a value of 'live' marker value.

    This is primarily used to ease identification using the instrument visualization tools.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> DeadThreshold - The threshold against which to judge if a spectrum belongs to a dead detector (default 0.0)</LI>
    <LI> LiveValue - The value to assign to an integrated spectrum flagged as 'live' (default 0.0)</LI>
    <LI> DeadValue - The value to assign to an integrated spectrum flagged as 'dead' (default 100.0)</LI>
    <LI> OutputFile - (Optional) A filename to which to write the list of dead detector UDETs </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 02/10/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport FindDeadDetectors : public API::Algorithm
    {
    public:
      /// Default constructor
      FindDeadDetectors() : API::Algorithm(),
        m_deadThreshold(0.0), m_liveValue(0.0), m_deadValue(100.0)
      {};
      /// Destructor
      virtual ~FindDeadDetectors() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FindDeadDetectors";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();

      API::MatrixWorkspace_sptr integrateWorkspace(std::string outputWorkspaceName);
      
      void checkAndLoadInputs();
      double m_deadThreshold;
      double m_liveValue;
      double m_deadValue;
      double m_startX;
      double m_endX;

      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };



  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FINDDEADDETECTORS_H_*/
