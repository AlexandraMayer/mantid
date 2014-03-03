/*WIKI*
 Performs polarisation correction on the input workspace


 *WIKI*/

#include "MantidAlgorithms/PolarisationCorrection.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"
#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace
{

  const std::string pNRLabel()
  {
    return "PNR";
  }

  const std::string pALabel()
  {
    return "PA";
  }

  const std::string crhoLabel()
  {
    return "crho";
  }

  const std::string cppLabel()
  {
    return "cPp";
  }

  const std::string cAlphaLabel()
  {
    return "cAlpha";
  }

  const std::string cApLabel()
  {
    return "cAp";
  }

  std::vector<std::string> modes()
  {
    std::vector<std::string> modes;
    modes.push_back(pALabel());
    modes.push_back(pNRLabel());
    return modes;
  }

  Instrument_const_sptr fetchInstrument(WorkspaceGroup const * const groupWS)
  {
    if (groupWS->size() == 0)
    {
      throw std::invalid_argument("Input group workspace has no children.");
    }
    Workspace_sptr firstWS = groupWS->getItem(0);
    MatrixWorkspace_sptr matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(firstWS);
    return matrixWS->getInstrument();
  }

  typedef std::vector<double> VecDouble;
}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PolarisationCorrection)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    PolarisationCorrection::PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    PolarisationCorrection::~PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string PolarisationCorrection::name() const
    {
      return "PolarisationCorrection";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int PolarisationCorrection::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string PolarisationCorrection::category() const
    {
      return "ISIS//Reflectometry";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void PolarisationCorrection::initDocs()
    {
      this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
      this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void PolarisationCorrection::init()
    {
      declareProperty(
          new WorkspaceProperty<Mantid::API::WorkspaceGroup>("InputWorkspace", "", Direction::Input),
          "An input workspace to process.");

      auto propOptions = modes();
      declareProperty("PolarisationAnalysis", "PA", boost::make_shared<StringListValidator>(propOptions),
          "What Polarisation mode will be used?\n"
              "PNR: Polarised Neutron Reflectivity mode\n"
              "PA: Full Polarisation Analysis PNR-PA");

      VecDouble emptyVec;
      auto mandatoryArray = boost::make_shared<MandatoryValidator<VecDouble> >();

      declareProperty(new ArrayProperty<double>(crhoLabel(), mandatoryArray, Direction::Input),
          "TODO-Description");
      declareProperty(new ArrayProperty<double>(cppLabel(), mandatoryArray, Direction::Input),
          "TODO - Description");
      declareProperty(new ArrayProperty<double>(cAlphaLabel(), mandatoryArray, Direction::Input),
          "TODO - Description");
      declareProperty(new ArrayProperty<double>(cApLabel(), mandatoryArray, Direction::Input),
          "TODO - Description");

      declareProperty(
          new WorkspaceProperty<Mantid::API::WorkspaceGroup>("OutputWorkspace", "", Direction::Output),
          "An output workspace.");
    }

    MatrixWorkspace_sptr PolarisationCorrection::execPolynomialCorrection(MatrixWorkspace_sptr& input,
        const VecDouble& coefficients)
    {
      auto polyCorr = this->createChildAlgorithm("PolynomialCorrection");
      polyCorr->initialize();
      polyCorr->setProperty("InputWorkspace", input);
      polyCorr->setProperty("Coefficients", coefficients);
      polyCorr->execute();
      MatrixWorkspace_sptr corrected = polyCorr->getProperty("OutputWorkspace");
      return corrected;
    }

    MatrixWorkspace_sptr PolarisationCorrection::multiply(MatrixWorkspace_sptr& lhs,
        MatrixWorkspace_sptr& rhs)
    {
      auto multiply = this->createChildAlgorithm("Multiply");
      multiply->initialize();
      multiply->setProperty("LHSWorkspace", lhs);
      multiply->setProperty("RHSWorkspace", rhs);
      multiply->execute();
      MatrixWorkspace_sptr outWS = multiply->getProperty("OutputWorskapce");
      return outWS;
    }

    MatrixWorkspace_sptr PolarisationCorrection::multiply(MatrixWorkspace_sptr& lhs, const double& rhs)
    {
      auto multiply = this->createChildAlgorithm("Multiply");
      multiply->initialize();
      multiply->setProperty("LHSWorkspace", lhs);
      multiply->setProperty("RHSWorkspace", rhs);
      multiply->execute();
      MatrixWorkspace_sptr outWS = multiply->getProperty("OutputWorskapce");
      return outWS;
    }

    MatrixWorkspace_sptr PolarisationCorrection::add(MatrixWorkspace_sptr& lhs, const double& rhs)
    {
      auto plus = this->createChildAlgorithm("Plus");
      plus->initialize();
      plus->setProperty("LHSWorkspace", lhs);
      plus->setProperty("RHSWorkspace", rhs);
      plus->execute();
      MatrixWorkspace_sptr outWS = plus->getProperty("OutputWorskapce");
      return outWS;
    }

    WorkspaceGroup_sptr PolarisationCorrection::execPA(WorkspaceGroup_sptr inWS)
    {

      size_t itemIndex = 0;
      MatrixWorkspace_sptr Ipp = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Iaa = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Ipa = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Iap = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));

      MatrixWorkspace_sptr ones = WorkspaceFactory::Instance().create(Iaa);
      ones = this->add(ones, 1.0);

      const VecDouble c_rho = getProperty(crhoLabel());
      const VecDouble c_alpha = getProperty(cAlphaLabel());
      const VecDouble c_pp = getProperty(cppLabel());
      const VecDouble c_ap = getProperty(cApLabel());

      const auto rho = this->execPolynomialCorrection(ones, c_rho);
      const auto pp = this->execPolynomialCorrection(ones, c_pp);
      const auto alpha = this->execPolynomialCorrection(ones, c_alpha);
      const auto ap = this->execPolynomialCorrection(ones, c_ap);

      const auto A0 = Iaa * pp + ap * Ipa * rho * pp + ap * Iap * pp * alpha
          + Ipp * ap * alpha * rho * pp;
      const auto A1 = pp * Iaa;
      const auto A2 = pp * Iap;
      const auto A3 = ap * Iaa;
      const auto A4 = ap * Ipa;
      const auto A5 = ap * alpha * Ipp;
      const auto A6 = ap * alpha * Iap;
      const auto A7 = pp * rho * Ipp;
      const auto A8 = pp * rho * Ipa;

      const auto D = pp * ap * (rho + alpha + 1.0 + rho * alpha);

      const auto nIpp = (A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8 + Ipp + Iaa - Ipa - Iap) / D;
      const auto nIaa = (A0 + A1 - A2 + A3 - A4 - A5 + A6 - A7 + A8 + Ipp + Iaa - Ipa - Iap) / D;
      const auto nIpa = (A0 - A1 + A2 + A3 - A4 - A5 + A6 + A7 - A8 - Ipp - Iaa + Ipa + Iap) / D;
      const auto nIap = (A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8 - Ipp - Iaa + Ipa + Iap) / D;

      WorkspaceGroup_sptr dataOut = boost::make_shared<WorkspaceGroup>();
      dataOut->addWorkspace(nIpp);
      dataOut->addWorkspace(nIaa);
      dataOut->addWorkspace(nIpa);
      dataOut->addWorkspace(nIap);

      return DataOut;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void PolarisationCorrection::exec()
    {
      WorkspaceGroup_sptr inWS = getProperty("InputWorkspace");
      const std::string analysisMode = getProperty("PolarisationAnalysis");
      const size_t nWorkspaces = inWS->size();

      Instrument_const_sptr instrument = fetchInstrument(inWS.get());

      WorkspaceGroup_sptr outWS;
      if (analysisMode == pALabel())
      {
        if (nWorkspaces != 4)
        {
          throw std::invalid_argument("For PA analysis, input group must have 4 periods.");
        }
        outWS = execPA(inWS);
      }
      else if (analysisMode == pNRLabel())
      {
        if (nWorkspaces != 2)
        {
          throw std::invalid_argument("For PNR analysis, input group must have 2 periods.");
        }

      }
    }

  } // namespace Algorithms
} // namespace Mantid
