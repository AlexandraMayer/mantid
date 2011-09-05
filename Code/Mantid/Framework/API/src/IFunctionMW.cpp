//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunctionMW::g_log = Kernel::Logger::get("IFunctionMW");

  /** Set the workspace
    * @param ws :: A shared pointer to a workspace. Must be a MatrixWorkspace.
    * @param slicing :: A string identifying the data to be fitted. Format for IFunctionMW:
    *  "WorkspaceIndex=int,StartX=double,EndX=double". StartX and EndX are optional.
  */
  void IFunctionMW::setWorkspace(boost::shared_ptr<const Workspace> ws,const std::string& slicing,bool)
  {
    try
    {
      MatrixWorkspace_const_sptr mws = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
      if (!mws)
      {
        throw std::invalid_argument("Workspace has a wrong type (not a MatrixWorkspace)");
      }

      size_t index = mws->getNumberHistograms();
      size_t xMin = mws->blocksize();
      size_t xMax = mws->blocksize();
      double startX = 0;
      double endX = 0;
      size_t n = mws->blocksize(); // length of each Y vector

      Expression expr;
      expr.parse(slicing);
      if (expr.name() == ",")
      {
        for(size_t i = 0; i < expr.size(); ++i)
        {
          const Expression& e = expr[i];
          if (e.size() == 2 && e.name() == "=")
          {
            if (e[0].name() == "WorkspaceIndex")
            {
              index = boost::lexical_cast<int>(e[1].str());
            }
            else if (e[0].name() == "StartX")
            {
              startX = boost::lexical_cast<double>(e[1].str());
              xMin = 0;
            }
            else if (e[0].name() == "EndX")
            {
              endX = boost::lexical_cast<double>(e[1].str());
              xMax = 0;
            }
          }
        }
      }
      else if (expr.size() == 2 && expr.name() == "=" && expr[0].name() == "WorkspaceIndex")
      {
        index = boost::lexical_cast<int>(expr[1].name());
      }

      if (index >= mws->getNumberHistograms())
      {
        g_log.warning("WorkspaceIndex not set, defaulting to 0");
        index = 0;
      }
      else if (index >= static_cast<int>(mws->getNumberHistograms()))
      {
        throw std::range_error("WorkspaceIndex outside range");
      }
      const MantidVec& x = mws->readX(index);
      const MantidVec& y = mws->readY(index);
      const MantidVec& e = mws->readE(index);

      if (xMin >= mws->blocksize())
      {
        xMin = 0;
      }
      else
      {
        for(; xMin < n - 1; ++xMin)
        {
          if (x[xMin] > startX)
          {
            xMin--;
            break;
          }
        }
        if (xMin >= mws->blocksize()) xMin = 0;
      }

      if (xMax >= mws->blocksize())
      {
        xMax = n - 1;
      }
      else
      {
        for(; xMax < n - 1; ++xMax)
        {
          if (x[xMax] > endX)
          {
            xMax--;
            break;
          }
        }
        if (xMax >= mws->blocksize()) xMax = 0;
      }

      if (xMin > xMax)
      {
        size_t tmp = xMin;
        xMin = xMax;
        xMax = tmp;
      }
      m_dataSize = xMax - xMin + 1;
      m_data = &y[xMin];
      m_xValues.reset(new double[m_dataSize]);
      m_weights.reset(new double[m_dataSize]);
      bool isHist = x.size() > y.size();

      for (size_t i = 0; i < m_dataSize; ++i)
      {
        if (isHist)
        {
          m_xValues[i] = 0.5*(x[xMin + i] + x[xMin + i + 1]);
        }
        else
        {
          m_xValues[i] = x[xMin + i];
        }
        if (e[xMin + i] <= 0.0)
          m_weights[i] = 1.0;
        else
          m_weights[i] = 1./e[xMin + i];
      }

      if (mws->hasMaskedBins(index))
      {
        const MatrixWorkspace::MaskList& mlist = mws->maskedBins(index);
        MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
        for(;it!=mlist.end();it++)
        {
          m_weights[it->first - xMin] = 0.;
        }
      }

      setMatrixWorkspace(mws,index,xMin,xMax);
      
    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunctionMW::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }
  }

  /// Get the workspace
  boost::shared_ptr<const API::Workspace> IFunctionMW::getWorkspace()const
  {
    return m_workspace;
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  size_t IFunctionMW::dataSize()const
  {
    return m_dataSize;
  }

  /// Returns a pointer to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  const double* IFunctionMW::getData()const
  {
    return m_data;
  }

  const double* IFunctionMW::getWeights()const
  {
    return &m_weights[0];
  }

  /// Function you want to fit to. 
  /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunctionMW::function(double* out)const
  {
    if (m_dataSize == 0) return;
    functionMW(out,m_xValues.get(),m_dataSize);

  }

  /// Derivatives of function with respect to active parameters
  void IFunctionMW::functionDeriv(Jacobian* out)
  {
    if (out == NULL) 
    {
      functionDerivMW(out,m_xValues.get(),0);
      return;
    }
    if (m_dataSize == 0) return;
    functionDerivMW(out,m_xValues.get(),m_dataSize);
  }


/** Base class implementation of derivative IFunctionMW throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IFunctionMW::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
  throw Kernel::Exception::NotImplementedError("No derivative IFunctionMW provided");
}

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param xMin :: The lower bin index
 * @param xMax :: The upper bin index
 */
void IFunctionMW::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wi,size_t xMin,size_t xMax)
{
  m_workspaceIndex = wi;
  m_xMinIndex = xMin;
  m_xMaxIndex = xMax;

  if (m_workspace.get() == workspace.get()) return;
  m_workspace = workspace;

  try
  {

    // check if parameter are specified in instrument definition file

    const Geometry::ParameterMap& paramMap = m_workspace->instrumentParameters();

    Geometry::IDetector_const_sptr det = m_workspace->getDetector(wi);

    // if det is a detector groupworkspace then take as the detector
    // the detector returned by det->getID()
    if ( boost::dynamic_pointer_cast<const Geometry::DetectorGroup>(det) )
    {
      Instrument_const_sptr inst = m_workspace->getInstrument();
      det = inst->getDetector(det->getID());
    }

    for (size_t i = 0; i < nParams(); i++)
    {
      if ( !isExplicitlySet(i) )
      {
        Geometry::Parameter_sptr param = paramMap.getRecursive(&(*det), parameterName(i), "fitting");
        if (param != Geometry::Parameter_sptr())
        {
          // get FitParameter
          const Geometry::FitParameter& fitParam = param->value<Geometry::FitParameter>();

          // check first if this parameter is actually specified for this function
          if ( name().compare(fitParam.getFunction()) == 0 )
          {
            // update value          
            IFunctionWithLocation* testWithLocation = dynamic_cast<IFunctionWithLocation*>(this);
            if ( testWithLocation == NULL || 
              (fitParam.getLookUpTable().containData() == false && fitParam.getFormula().compare("") == 0) )
            {
              setParameter(i, fitParam.getValue());
            }
            else
            {
              double centreValue = testWithLocation->centre();
              Kernel::Unit_sptr centreUnit;  // unit of value used in formula or to look up value in lookup table
              if ( fitParam.getFormula().compare("") == 0 )
                centreUnit = fitParam.getLookUpTable().getXUnit();  // from table
              else
              {
                if ( !fitParam.getFormulaUnit().empty() )
                {
                  try
                  {
                    centreUnit = Kernel::UnitFactory::Instance().create(fitParam.getFormulaUnit());  // from formula
                  }
                  catch (...)
                  {
                    g_log.warning() << fitParam.getFormulaUnit() << " Is not an recognised formula unit for parameter " 
                                    << fitParam.getName() << "\n";
                  }
                }
              }

              // if unit specified convert centre value to unit required by formula or look-up-table
              if (centreUnit)
                centreValue = convertValue(centreValue, centreUnit, m_workspace, wi);

              double paramValue = fitParam.getValue(centreValue);

              // this returned param value by say a formula or a look-up-table may have
              // a unit of its own. If this is specified, try the following
              if ( fitParam.getFormula().compare("") == 0 )
              {
                // so from look up table
                Kernel::Unit_sptr resultUnit = fitParam.getLookUpTable().getYUnit();  // from table
                paramValue /= convertValue(1.0, resultUnit, m_workspace, wi);
              }
              else
              {
                // so from formula 

                std::string resultUnitStr = fitParam.getResultUnit();

                if ( !resultUnitStr.empty() )
                {
                  std::vector<std::string> allUnitStr = Kernel::UnitFactory::Instance().getKeys();
                  for (unsigned iUnit = 0; iUnit < allUnitStr.size(); iUnit++)
                  {
                    size_t found = resultUnitStr.find(allUnitStr[iUnit]);
                    if ( found != std::string::npos )
                    {
                      size_t len = allUnitStr[iUnit].size();
                      std::stringstream readDouble;
                      Kernel::Unit_sptr unt = Kernel::UnitFactory::Instance().create(allUnitStr[iUnit]);
                      readDouble << 1.0 / 
                        convertValue(1.0, unt, m_workspace, wi);
                      resultUnitStr.replace(found, len, readDouble.str());
                    }
                  }  // end for

                  try
                  {
                    mu::Parser p;
                    p.SetExpr(resultUnitStr);
                    paramValue *= p.Eval();
                  }
                  catch (mu::Parser::exception_type &e)
                  {
                    g_log.error() << "Cannot convert formula unit to workspace unit"
                      << " Formula unit which cannot be passed is " << resultUnitStr 
                      << ". Muparser error message is: " << e.GetMsg() << std::endl;
                  }
                } // end if
              } // end else
              

              setParameter(i, paramValue);
            }  // end of update parameter value

            // add tie if specified for this parameter in instrument definition file
            if ( fitParam.getTie().compare("") )
            {  
              std::ostringstream str;
              str << getParameter(i);
              tie(parameterName(i), str.str());
            }

            // add constraint if specified for this parameter in instrument definition file
            if ( fitParam.getConstraint().compare("") )
            {  
              IConstraint* constraint = ConstraintFactory::Instance().createInitialized(this, fitParam.getConstraint());
              if ( fitParam.getConstraintPenaltyFactor().compare("") )
              {
                double penalty;
                try
                {
                  penalty = atof(fitParam.getConstraintPenaltyFactor().c_str());
                  constraint->setPenaltyFactor(penalty);
                }
                catch (...)
                {
                  g_log.warning() << "Can't set penalty factor for constraint\n";
                }
              }
              addConstraint(constraint);
            }
          }
        }
      }
    }
  }
  catch(...)
  {
  }
}

/** Convert a value from unit defined in workspace (ws) to outUnit
 *
 *  @param value ::   assumed to be in unit of workspace
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 *  @return converted value
 */
double IFunctionMW::convertValue(double value, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const MatrixWorkspace> ws,
                               size_t wsIndex)const
{
  double retVal = value;
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      retVal = factor * std::pow(retVal,power);
    }
    else
    {
      double l1,l2,twoTheta;

      // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
      Instrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if (sample == NULL)
      {
        g_log.error() << "No sample defined instrument. Cannot convert units for function\n"
                      << "Ignore convertion."; 
        return value; 
      }
      l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_const_sptr det = ws->getDetector(wsIndex);
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        twoTheta = ws->detectorTwoTheta(det);
      }
      else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
      {
        l2 = det->getDistance(*(instrument->getSource()));
        l2 = l2 - l1;
        twoTheta = 0.0;
      }

      std::vector<double> endPoint;
      endPoint.push_back(retVal);
      std::vector<double> emptyVec;
      wsUnit->toTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      outUnit->fromTOF(endPoint,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      retVal = endPoint[0];
    }
  }  
  return retVal;
}

/** Convert values from unit defined in workspace (ws) to outUnit
 *
 *  @param values ::   As input: assumed to be in unit of workspace. 
 *                  As output: in unit of outUnit
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 */
void IFunctionMW::convertValue(std::vector<double>& values, Kernel::Unit_sptr& outUnit, 
                               boost::shared_ptr<const MatrixWorkspace> ws,
                               size_t wsIndex) const
{
  Kernel::Unit_sptr wsUnit = ws->getAxis(0)->unit();

  // if unit required by formula or look-up-table different from ws-unit then 
  if ( outUnit->unitID().compare(wsUnit->unitID()) != 0 )
  {
    // first check if it is possible to do a quick convertion convert
    double factor,power;
    if (wsUnit->quickConversion(*outUnit,factor,power) )
    {
      for (size_t i = 0; i < values.size(); i++)
        values[i] = factor * std::pow(values[i],power);
    }
    else
    {
      double l1,l2,twoTheta;

      // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
      Instrument_const_sptr instrument = ws->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if (sample == NULL)
      {
        g_log.error() << "No sample defined instrument. Cannot convert units for function\n"
                      << "Ignore convertion."; 
        return; 
      }
      l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_const_sptr det = ws->getDetector(wsIndex);
      if ( ! det->isMonitor() )
      {
        l2 = det->getDistance(*sample);
        twoTheta = ws->detectorTwoTheta(det);
      }
      else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
      {
        l2 = det->getDistance(*(instrument->getSource()));
        l2 = l2 - l1;
        twoTheta = 0.0;
      }

      std::vector<double> emptyVec;
      wsUnit->toTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
      outUnit->fromTOF(values,emptyVec,l1,l2,twoTheta,0,0.0,0.0);
    }
  }  
}

/**
 * Calculate the Jacobian with respect to parameters actually declared in the IFunctionMW
 * @param out :: The output Jacobian
 * @param xValues :: The x-values
 * @param nData :: The number of data points (and x-values).
 */
void IFunctionMW::calJacobianForCovariance(Jacobian* out, const double* xValues, const size_t nData)
{
  this->functionDerivMW(out,xValues,nData);
}

/// Called after setMatrixWorkspace if setWorkspace hadn't been called before
void IFunctionMW::setUpNewStuff(boost::shared_array<double> xs,boost::shared_array<double> weights)
{
  m_dataSize = m_xMaxIndex - m_xMinIndex;
  m_data = &m_workspace->readY(m_workspaceIndex)[m_xMinIndex];
  //m_xValues = &m_workspace->readX(m_workspaceIndex)[m_xMinIndex];
  if (weights && xs)
  {
    m_xValues = xs;
    m_weights = weights;
  }
  else
  {
    m_xValues.reset(new double[m_dataSize]);
    m_weights.reset(new double[m_dataSize]);
    const MantidVec& x = m_workspace->readX(m_workspaceIndex);
    const MantidVec& e = m_workspace->readE(m_workspaceIndex);
    bool isHist = m_workspace->isHistogramData();

    for (size_t i = 0; i < m_dataSize; ++i)
    {
      if (isHist)
      {
        m_xValues[i] = 0.5*(x[m_xMinIndex + i] + x[m_xMinIndex + i + 1]);
      }
      else
      {
        m_xValues[i] = x[m_xMinIndex + i];
      }
      if (e[m_xMinIndex + i] <= 0.0)
        m_weights[i] = 1.0;
      else
        m_weights[i] = 1./e[m_xMinIndex + i];
    }

    if (m_workspace->hasMaskedBins(m_workspaceIndex))
    {
      const MatrixWorkspace::MaskList& mlist = m_workspace->maskedBins(m_workspaceIndex);
      MatrixWorkspace::MaskList::const_iterator it = mlist.begin();
      for(;it!=mlist.end();it++)
      {
        m_weights[it->first - m_xMinIndex] = 0.;
      }
    }
  }

}

namespace
{
  /**
   * A simple implementation of Jacobian.
   */
  class SimpleJacobian: public Jacobian
  {
  public:
    /// Constructor
    SimpleJacobian(size_t nData,size_t nParams):m_nData(nData),m_nParams(nParams),m_data(nData*nParams){}
    /// Setter
    virtual void set(size_t iY, size_t iP, double value)
    {
      m_data[iY * m_nParams + iP] = value;
    }
    /// Getter
    virtual double get(size_t iY, size_t iP)
    {
      return m_data[iY * m_nParams + iP];
    }
  private:
    size_t m_nData; ///< size of the data / first dimension
    size_t m_nParams; ///< number of parameters / second dimension
    std::vector<double> m_data; ///< data storage
  };
}

/** 
 * Creates a workspace containing values calculated with this function. It takes a workspace and ws index
 * of a spectrum which this function may have been fitted to. The output contains the original spectrum 
 * (wi = 0), the calculated values (ws = 1), and the difference between them (ws = 2).
 * @param inWS :: input workspace
 * @param wi :: workspace index
 * @param sd :: optional standard deviations of the parameters for calculating the error bars
 * @return created workspase
 */
boost::shared_ptr<API::MatrixWorkspace> IFunctionMW::createCalculatedWorkspace(
  boost::shared_ptr<const API::MatrixWorkspace> inWS, 
  size_t wi,
  const std::vector<double>& sd
  )
{
      const MantidVec& inputX = inWS->readX(wi);
      const MantidVec& inputY = inWS->readY(wi);
      const MantidVec& inputE = inWS->readE(wi);
      size_t nData = dataSize();

      size_t histN = inWS->isHistogramData() ? 1 : 0;
      API::MatrixWorkspace_sptr ws =
        Mantid::API::WorkspaceFactory::Instance().create(
            "Workspace2D",
            3,
            nData + histN,
            nData);
      ws->setTitle("");
      ws->setYUnitLabel(inWS->YUnitLabel());
      ws->setYUnit(inWS->YUnit());
      ws->getAxis(0)->unit() = inWS->getAxis(0)->unit();
      API::TextAxis* tAxis = new API::TextAxis(3);
      tAxis->setLabel(0,"Data");
      tAxis->setLabel(1,"Calc");
      tAxis->setLabel(2,"Diff");
      ws->replaceAxis(1,tAxis);

      assert(m_xMaxIndex-m_xMinIndex+1 == static_cast<int64_t>(nData));

      for(size_t i=0;i<3;i++)
      {
        ws->dataX(i).assign(inputX.begin()+m_xMinIndex,inputX.begin()+m_xMaxIndex+1+histN);
      }

      ws->dataY(0).assign(inputY.begin()+m_xMinIndex,inputY.begin()+m_xMaxIndex+1);
      ws->dataE(0).assign(inputE.begin()+m_xMinIndex,inputE.begin()+m_xMaxIndex+1);

      MantidVec& Ycal = ws->dataY(1);
      MantidVec& Ecal = ws->dataE(1);
      MantidVec& E = ws->dataY(2);

      double* lOut = new double[nData];  // to capture output from call to function()
      function( lOut );

      for(size_t i=0; i<nData; i++)
      {
        Ycal[i] = lOut[i]; 
        E[i] = m_data[i] - Ycal[i];
      }

      delete [] lOut; 

      if (sd.size() == static_cast<size_t>(this->nParams()))
      {
        SimpleJacobian J(nData,this->nParams());
        try
        {
          this->functionDeriv(&J);
        }
        catch(...)
        {
          this->calNumericalDeriv(&J,&m_xValues[0],nData);
        }
        for(size_t i=0; i<nData; i++)
        {
          double err = 0.0;
          for(size_t j=0;j< static_cast<size_t>(nParams());++j)
          {
            double d = J.get(i,j) * sd[j];
            err += d*d;
          }
          Ecal[i] = sqrt(err);
        }
      }

      return ws;
}

/** Calculate numerical derivatives.
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IFunctionMW::calNumericalDeriv(Jacobian* out, const double* xValues, const size_t& nData)
{
    double stepPercentage = 0.001; // step percentage
    double step; // real step
    double minDouble = std::numeric_limits<double>::min();
    double cutoff = 100.0*minDouble/stepPercentage;
    size_t nParam = nParams();

    // allocate memory if not already done
    if (!m_tmpFunctionOutputMinusStep && nData>0)
    {
      m_tmpFunctionOutputMinusStep.reset(new double[nData]);
      m_tmpFunctionOutputPlusStep.reset(new double[nData]);
    }

    functionMW(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

    for (size_t iP = 0; iP < static_cast<int>(nParam); iP++)
    {
      if ( isActive(iP) )
      {
        const double& val = getParameter(iP);
        if (val < cutoff)
        {
          step = cutoff;
        }
        else
        {
          step = val*stepPercentage;
        }

        //double paramMstep = val - step;
        //setParameter(iP, paramMstep);
        //function(m_tmpFunctionOutputMinusStep.get(), xValues, nData);

        double paramPstep = val + step;
        setParameter(iP, paramPstep);
        functionMW(m_tmpFunctionOutputPlusStep.get(), xValues, nData);

        step = paramPstep - val;
        setParameter(iP, val);

        for (size_t i = 0; i < nData; i++) {
         // out->set(i,iP, 
         //   (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/(2.0*step));
          out->set(i,iP, 
            (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step);
        }
      }
    }
}

} // namespace API
} // namespace Mantid
