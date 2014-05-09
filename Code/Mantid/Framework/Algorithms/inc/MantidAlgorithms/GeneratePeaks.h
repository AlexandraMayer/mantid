#ifndef MANTID_ALGORITHMS_GENERATEPEAKS_H_
#define MANTID_ALGORITHMS_GENERATEPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IBackgroundFunction.h"

namespace Mantid
{
namespace Algorithms
{

  /** GeneratePeaks : Generate peaks from a table workspace containing peak parameters
    
    @date 2012-04-10

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport GeneratePeaks : public API::Algorithm
  {
  public:
    GeneratePeaks();
    virtual ~GeneratePeaks();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "GeneratePeaks";}
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Crystal";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    void processAlgProperties(std::string &peakfunctype, std::string &bkgdfunctype);

    void processParamTable();

    void processTableColumnNames();

    void generatePeaks();

    void generatePeaksNew(API::MatrixWorkspace_sptr dataWS);

    bool hasParameter(API::IFunction_sptr function, std::string paramname);

    API::MatrixWorkspace_sptr createOutputWorkspace();

    API::MatrixWorkspace_sptr createDataWorkspace(std::set<specid_t> spectra, std::vector<double> binparameters);

    void createFunction(std::string& peaktype, std::string& bkgdtype);

    /*
    API::IFunction_sptr createFunction(const std::string &peakFuncType, const std::vector<std::string> &colNames,
                                       const bool isRaw, const bool withBackground,
                                       DataObjects::TableWorkspace_const_sptr peakParmsWS,
                                       const std::size_t bkg_offset, const std::size_t rowNum,
                                       double &centre, double &fwhm);
                                       */

    void getSpectraSet(DataObjects::TableWorkspace_const_sptr peakParmsWS, std::set<specid_t>& spectra);

    void generatePeaks(API::MatrixWorkspace_sptr dataWS, DataObjects::TableWorkspace_const_sptr peakparameters,
       std::string peakfunction, bool m_newWSFromParent);

    double getTableValue(DataObjects::TableWorkspace_const_sptr tableWS, std::string colname, size_t index);

    /// Get the IPeakFunction part in the input function
    API::IPeakFunction_sptr getPeakFunction(API::IFunction_sptr infunction);

    /// Peak function
    API::IPeakFunction_sptr m_peakFunction;

    /// Background function
    API::IBackgroundFunction_sptr m_bkgdFunction;

    /// Spectrum map from full spectra workspace to partial spectra workspace
    std::map<specid_t, specid_t> mSpectrumMap;

    std::set<specid_t> spectra;

    /// Flag to use automatic background (???)
    bool m_useAutoBkgd;

    /// Parameter table workspace
    DataObjects::TableWorkspace_sptr m_funcParamWS;

    /// Input workspace (optional)
    API::MatrixWorkspace_const_sptr inputWS;

    /// Flag whether the new workspace is exactly as input
    bool m_newWSFromParent;

    /// Binning parameters
    std::vector<double> binParameters;

    /// Flag to generate background
    bool m_genBackground;

    bool m_useRawParameter;

    double m_maxChi2;

    double m_numPeakWidth;

    std::vector<std::string> m_funcParameterNames;

    int i_height, i_centre, i_width, i_a0, i_a1, i_a2;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_GENERATEPEAKS_H_ */
