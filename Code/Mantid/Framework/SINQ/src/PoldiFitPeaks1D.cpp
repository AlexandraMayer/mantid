#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidSINQ/PoldiFitPeaks1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidAPI/CompositeFunction.h"
#include <boost/make_shared.hpp>

#include <functional>
#include <boost/math/distributions/normal.hpp>


namespace Mantid
{
namespace Poldi
{

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace CurveFitting;

RefinedRange::RefinedRange(const PoldiPeak_sptr &peak, double fwhmMultiples) :
    m_peaks(1, peak)
{
    double width = peak->fwhm();
    double extent = std::max(0.002, width) * fwhmMultiples;

    m_xStart = peak->q() - extent;
    m_xEnd = peak->q() + extent;
}

RefinedRange::RefinedRange(double xStart, double xEnd, const std::vector<PoldiPeak_sptr> &peaks) :
    m_peaks(peaks),
    m_xStart(xStart),
    m_xEnd(xEnd)
{
}

RefinedRange::RefinedRange(const RefinedRange &other) :
    m_peaks(other.m_peaks),
    m_xStart(other.m_xStart),
    m_xEnd(other.m_xEnd)
{

}

bool RefinedRange::operator <(const RefinedRange &other) const
{
    return m_xStart < other.m_xStart;
}

bool RefinedRange::overlaps(const RefinedRange &other) const
{
    return    (other.m_xStart > m_xStart && other.m_xStart < m_xEnd)
           || (other.m_xEnd > m_xStart && other.m_xEnd < m_xEnd)
           || (other.m_xStart < m_xStart && other.m_xEnd > m_xEnd);
}

void RefinedRange::merge(const RefinedRange &other)
{
    m_peaks.insert(m_peaks.end(), other.m_peaks.begin(), other.m_peaks.end());

    m_xStart = std::min(m_xStart, other.m_xStart);
    m_xEnd = std::max(m_xEnd, other.m_xEnd);
}

bool operator <(const RefinedRange_sptr &lhs, const RefinedRange_sptr &rhs)
{
    return (*lhs) < (*rhs);
}



// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiFitPeaks1D)


PoldiFitPeaks1D::PoldiFitPeaks1D() :
    m_peaks(),
    m_profileTemplate(),
    m_peakResultOutput(),
    m_fitplots(new WorkspaceGroup),
    m_fwhmMultiples(1.0)
{

}

PoldiFitPeaks1D::~PoldiFitPeaks1D()
{
}


/// Algorithm's name for identification. @see Algorithm::name
const std::string PoldiFitPeaks1D::name() const { return "PoldiFitPeaks1D";}

/// Algorithm's version for identification. @see Algorithm::version
int PoldiFitPeaks1D::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiFitPeaks1D::category() const { return "SINQ\\Poldi"; }

void PoldiFitPeaks1D::init()
{
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input), "An input workspace containing a POLDI auto-correlation spectrum.");
    boost::shared_ptr<BoundedValidator<double> > minFwhmPerDirection = boost::make_shared<BoundedValidator<double> >();
    minFwhmPerDirection->setLower(2.0);
    declareProperty("FwhmMultiples", 2.0, minFwhmPerDirection, "Each peak will be fitted using x times FWHM data in each direction.", Direction::Input);

    std::vector<std::string> peakFunctions = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
    boost::shared_ptr<ListValidator<std::string> > peakFunctionNames(new ListValidator<std::string>(peakFunctions));
    declareProperty("PeakFunction", "Gaussian", peakFunctionNames, "Peak function that will be fitted to all peaks.", Direction::Input);

    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakTable","",Direction::Input), "A table workspace containing POLDI peak data.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","RefinedPeakTable",Direction::Output), "Output workspace with refined peak data.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("ResultTableWorkspace","ResultTable",Direction::Output), "Fit results.");
    declareProperty(new WorkspaceProperty<Workspace>("FitPlotsWorkspace","FitPlots",Direction::Output), "Plots of all peak fits.");
}

void PoldiFitPeaks1D::setPeakFunction(const std::string &peakFunction)
{
    m_profileTemplate = peakFunction;
}

PoldiPeakCollection_sptr PoldiFitPeaks1D::getInitializedPeakCollection(const DataObjects::TableWorkspace_sptr &peakTable) const
{
    PoldiPeakCollection_sptr peakCollection(new PoldiPeakCollection(peakTable));
    peakCollection->setProfileFunctionName(m_profileTemplate);

    return peakCollection;
}

std::vector<RefinedRange_sptr> PoldiFitPeaks1D::getRefinedRanges(const PoldiPeakCollection_sptr &peaks) const
{
    std::vector<RefinedRange_sptr> ranges;
    for(size_t i = 0; i < peaks->peakCount(); ++i) {
        ranges.push_back(boost::make_shared<RefinedRange>(peaks->peak(i), m_fwhmMultiples));
    }

    return ranges;
}

std::vector<RefinedRange_sptr> PoldiFitPeaks1D::getReducedRanges(const std::vector<RefinedRange_sptr> &ranges) const
{
    std::vector<RefinedRange_sptr> workingRanges(ranges);
    std::sort(workingRanges.begin(), workingRanges.end());

    std::vector<RefinedRange_sptr> reducedRanges;
    reducedRanges.push_back(boost::make_shared<RefinedRange>(*(workingRanges.front())));

    for(size_t i = 1; i < workingRanges.size(); ++i) {
        RefinedRange_sptr lastReduced = reducedRanges.back();
        RefinedRange_sptr current = workingRanges[i];

        if(!lastReduced->overlaps(*current)) {
            reducedRanges.push_back(boost::make_shared<RefinedRange>(*current));
        } else {
            lastReduced->merge(*current);
        }
    }

    return reducedRanges;
}

API::IFunction_sptr PoldiFitPeaks1D::getRangeProfile(const RefinedRange_sptr &range, int n) const
{
    CompositeFunction_sptr totalProfile(new CompositeFunction);
    totalProfile->initialize();

    std::vector<PoldiPeak_sptr> peaks = range->getPeaks();
    for(auto it = peaks.begin(); it != peaks.end(); ++it) {
        totalProfile->addFunction(getPeakProfile(*it));
    }

    totalProfile->addFunction(FunctionFactory::Instance().createInitialized("name=Chebyshev,n=" + boost::lexical_cast<std::string>(n)
                                                                                   + ",StartX=" + boost::lexical_cast<std::string>(range->getXStart())
                                                                                   + ",EndX=" + boost::lexical_cast<std::string>(range->getXEnd())));

    return totalProfile;
}

IFunction_sptr PoldiFitPeaks1D::getPeakProfile(const PoldiPeak_sptr &poldiPeak) const {
    IPeakFunction_sptr clonedProfile = boost::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction(m_profileTemplate));
    clonedProfile->setCentre(poldiPeak->q());
    clonedProfile->setFwhm(poldiPeak->fwhm(PoldiPeak::AbsoluteQ));
    clonedProfile->setHeight(poldiPeak->intensity());

    return clonedProfile;
}

void PoldiFitPeaks1D::setValuesFromProfileFunction(PoldiPeak_sptr poldiPeak, const IFunction_sptr &fittedFunction) const
{
    IPeakFunction_sptr peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(fittedFunction);

    if(peakFunction) {
        poldiPeak->setIntensity(UncertainValue(peakFunction->height(), peakFunction->getError(0)));
        poldiPeak->setQ(UncertainValue(peakFunction->centre(), peakFunction->getError(1)));
        poldiPeak->setFwhm(UncertainValue(peakFunction->fwhm(), getFwhmWidthRelation(peakFunction) * peakFunction->getError(2)));
    }
}

double PoldiFitPeaks1D::getFwhmWidthRelation(IPeakFunction_sptr peakFunction) const
{
    return peakFunction->fwhm() / peakFunction->getParameter(2);
}

PoldiPeakCollection_sptr PoldiFitPeaks1D::fitPeaks(const PoldiPeakCollection_sptr &peaks)
{
    g_log.information() << "Peaks to fit: " << peaks->peakCount() << std::endl;

    std::vector<RefinedRange_sptr> rawRanges = getRefinedRanges(peaks);
    std::vector<RefinedRange_sptr> reducedRanges = getReducedRanges(rawRanges);

    g_log.information() << "Ranges used for fitting: " << reducedRanges.size() << std::endl;

    Workspace2D_sptr dataWorkspace = getProperty("InputWorkspace");
    m_fitplots->removeAll();

    for(size_t i = 0; i < reducedRanges.size(); ++i) {
        RefinedRange_sptr currentRange = reducedRanges[i];
        int nMin = getBestChebyshevPolynomialDegree(dataWorkspace, currentRange);

        if(nMin > -1) {
            IAlgorithm_sptr fit = getFitAlgorithm(dataWorkspace, currentRange, nMin);
            fit->execute();

            IFunction_sptr fitFunction = fit->getProperty("Function");
            CompositeFunction_sptr composite = boost::dynamic_pointer_cast<CompositeFunction>(fitFunction);

            if(!composite) {
                throw std::runtime_error("Not a composite function!");
            }

            std::vector<PoldiPeak_sptr> peaks = currentRange->getPeaks();
            for(size_t i = 0; i < peaks.size(); ++i) {
                setValuesFromProfileFunction(peaks[i], composite->getFunction(i));
                MatrixWorkspace_sptr fpg = fit->getProperty("OutputWorkspace");
                m_fitplots->addWorkspace(fpg);
            }
        }
    }

    return getReducedPeakCollection(peaks);
}

int PoldiFitPeaks1D::getBestChebyshevPolynomialDegree(const Workspace2D_sptr &dataWorkspace, const RefinedRange_sptr &range)
{
    int n = 0;
    double chiSquareMin = 1e10;
    int nMin = -1;

    while((n < 3)) {
        IAlgorithm_sptr fit = getFitAlgorithm(dataWorkspace, range, n);
        bool fitSuccess = fit->execute();

        if(fitSuccess) {
            ITableWorkspace_sptr fitCharacteristics = fit->getProperty("OutputParameters");
            TableRow row = fitCharacteristics->getRow(fitCharacteristics->rowCount() - 1);

            double chiSquare = row.Double(1);


            if(fabs(chiSquare - 1) < fabs(chiSquareMin - 1)) {
                chiSquareMin = chiSquare;
                nMin = n;
            }
        }

        ++n;
    }

    g_log.information() << "Chi^2 for range [" << range->getXStart() << " - " << range->getXEnd() << "] is minimal at n = " << nMin << " with Chi^2 = " << chiSquareMin << std::endl;

    return nMin;
}

PoldiPeakCollection_sptr PoldiFitPeaks1D::getReducedPeakCollection(const PoldiPeakCollection_sptr &peaks) const
{
    PoldiPeakCollection_sptr reducedPeaks = boost::make_shared<PoldiPeakCollection>();
    reducedPeaks->setProfileFunctionName(peaks->getProfileFunctionName());

    for(size_t i = 0; i < peaks->peakCount(); ++i) {
        PoldiPeak_sptr currentPeak = peaks->peak(i);

        if(peakIsAcceptable(currentPeak)) {
            reducedPeaks->addPeak(currentPeak);
        }
    }

    return reducedPeaks;
}

bool PoldiFitPeaks1D::peakIsAcceptable(const PoldiPeak_sptr &peak) const
{
    return peak->intensity() > 0 && peak->fwhm(PoldiPeak::Relative) < 0.02;
}

void PoldiFitPeaks1D::exec()
{
    setPeakFunction(getProperty("PeakFunction"));

    // Number of points around the peak center to use for the fit
    m_fwhmMultiples = getProperty("FwhmMultiples");

    // try to construct PoldiPeakCollection from provided TableWorkspace
    TableWorkspace_sptr poldiPeakTable = getProperty("PoldiPeakTable");    
    m_peaks = getInitializedPeakCollection(poldiPeakTable);

    PoldiPeakCollection_sptr fittedPeaksNew = fitPeaks(m_peaks);
    PoldiPeakCollection_sptr fittedPeaksOld = m_peaks;
    while(fittedPeaksNew->peakCount() < fittedPeaksOld->peakCount()) {
        fittedPeaksOld = fittedPeaksNew;
        fittedPeaksNew = fitPeaks(fittedPeaksOld);
    }

    m_peakResultOutput = generateResultTable(fittedPeaksNew);

    setProperty("OutputWorkspace", m_peaks->asTableWorkspace());
    setProperty("ResultTableWorkspace", m_peakResultOutput);
    setProperty("FitPlotsWorkspace", m_fitplots);
}

IAlgorithm_sptr PoldiFitPeaks1D::getFitAlgorithm(const Workspace2D_sptr &dataWorkspace, const RefinedRange_sptr &range, int n)
{
    IFunction_sptr rangeProfile = getRangeProfile(range, n);

    IAlgorithm_sptr fitAlgorithm = createChildAlgorithm("Fit", -1, -1, false);
    fitAlgorithm->setProperty("CreateOutput", true);
    fitAlgorithm->setProperty("Output", "FitPeaks1D");
    fitAlgorithm->setProperty("CalcErrors", true);
    fitAlgorithm->setProperty("OutputCompositeMembers", true);
    fitAlgorithm->setProperty("Function", rangeProfile);
    fitAlgorithm->setProperty("InputWorkspace", dataWorkspace);
    fitAlgorithm->setProperty("WorkspaceIndex", 0);
    fitAlgorithm->setProperty("StartX", range->getXStart());
    fitAlgorithm->setProperty("EndX", range->getXEnd());

    return fitAlgorithm;
}

void PoldiFitPeaks1D::initializePeakResultWorkspace(const DataObjects::TableWorkspace_sptr &peakResultWorkspace) const
{
    peakResultWorkspace->addColumn("str", "Q");
    peakResultWorkspace->addColumn("str", "d");
    peakResultWorkspace->addColumn("double", "deltaD/d *10^3");
    peakResultWorkspace->addColumn("str", "FWHM rel. *10^3");
    peakResultWorkspace->addColumn("str", "Intensity");
}

void PoldiFitPeaks1D::storePeakResult(TableRow tableRow, const PoldiPeak_sptr &peak) const
{
    UncertainValue q = peak->q();
    UncertainValue d = peak->d();

    tableRow << UncertainValueIO::toString(q)
             << UncertainValueIO::toString(d)
             << d.error() / d.value() * 1e3
             << UncertainValueIO::toString(peak->fwhm(PoldiPeak::Relative) * 1e3)
             << UncertainValueIO::toString(peak->intensity());
}

TableWorkspace_sptr PoldiFitPeaks1D::generateResultTable(const PoldiPeakCollection_sptr &peaks) const
{
    TableWorkspace_sptr outputTable = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
    initializePeakResultWorkspace(outputTable);

    for(size_t i = 0; i < peaks->peakCount(); ++i) {
        storePeakResult(outputTable->appendRow(), peaks->peak(i));
    }

    return outputTable;
}

} // namespace Poldi
} // namespace Mantid
