/*WIKI* 


Given a set of peaks, and given lattice parameters (<math>a,b,c,alpha,beta,gamma</math>), this algorithm will find the UB matrix, that best fits the data.  The algorithm searches over a large range of possible orientations for the orientation for which the rotated B matrix best fits the data.  It then uses a least squares approach to optimize the complete UB matrix.


*WIKI*/
#include "MantidCrystal/FindUBUsingLatticeParameters.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include <cstdio>

namespace Mantid
{
namespace Crystal
{
  Kernel::Logger& FindUBUsingLatticeParameters::g_log = 
                        Kernel::Logger::get("FindUBUsingLatticeParameters");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindUBUsingLatticeParameters)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  FindUBUsingLatticeParameters::FindUBUsingLatticeParameters()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  FindUBUsingLatticeParameters::~FindUBUsingLatticeParameters()
  {
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void FindUBUsingLatticeParameters::initDocs()
  {
    std::string summary("Calculate the UB matrix from a peaks workspace, ");
    summary += "given lattice parameters.";
    this->setWikiSummary( summary );

    std::string message("Calculate the UB matrix from a peaks workspace, ");
    message += "given lattice parameters.";
    this->setOptionalMessage( message );
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FindUBUsingLatticeParameters::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    boost::shared_ptr<BoundedValidator<int> > moreThan2Int(new BoundedValidator<int>());
    moreThan2Int->setLower(2);

    boost::shared_ptr<BoundedValidator<double> > reasonable_angle(new BoundedValidator<double>());
    reasonable_angle->setLower(5.0);
    reasonable_angle->setUpper(175.0);

    // use negative values, force user to input all parameters
    this->declareProperty("a",-1.0, mustBePositive, "Lattice parameter a");
    this->declareProperty("b",-1.0, mustBePositive, "Lattice parameter b");
    this->declareProperty("c",-1.0, mustBePositive, "Lattice parameter c");
    this->declareProperty("alpha",-1.0,reasonable_angle,"Lattice parameter alpha");
    this->declareProperty("beta",-1.0, reasonable_angle,"Lattice parameter beta");
    this->declareProperty("gamma",-1.0,reasonable_angle,"Lattice parameter gamma");
    this->declareProperty("NumInitial", 15, moreThan2Int, "Number of Peaks to Use on First Pass(15)");
    this->declareProperty("Tolerance",0.15, mustBePositive,"Indexing Tolerance (0.15)");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FindUBUsingLatticeParameters::exec()
  {
    double a           = this->getProperty("a");
    double b           = this->getProperty("b");
    double c           = this->getProperty("c");
    double alpha       = this->getProperty("alpha");
    double beta        = this->getProperty("beta");
    double gamma       = this->getProperty("gamma");
    int    num_initial = this->getProperty("NumInitial");
    double tolerance   = this->getProperty("Tolerance");
                                          
    int    base_index         = -1;   // these "could" be properties if need be
    double degrees_per_step   = 1.5;

    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );

    if (!ws) throw std::runtime_error("Could not read the peaks workspace");

    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();

    std::vector<V3D>  q_vectors;
    q_vectors.reserve( n_peaks );

      for (size_t i = 0; i < n_peaks; i++)
        q_vectors.push_back(peaks[i].getQSampleFrame());

    Matrix<double> UB(3,3,false);
    double error = IndexingUtils::Find_UB( UB, q_vectors, 
                                           a, b, c, alpha, beta, gamma,
                                           tolerance, 
                                           base_index, 
                                           num_initial, 
                                           degrees_per_step );

    std::cout << "Error = " << error << std::endl;
    std::cout << "UB = " << UB << std::endl;

    if ( ! IndexingUtils::CheckUB( UB ) ) // UB not found correctly
    {
      g_log.notice( std::string(
         "Found Invalid UB...peaks used might not be linearly independent") );
      g_log.notice( std::string(
         "UB NOT SAVED.") );
    }
    else                                 // tell user how many would be indexed
      {                                    // and save the UB in the sample

        std::vector<V3D> hkl_vectors;
        hkl_vectors.reserve(n_peaks);
        Matrix<double> UBinv(UB);
        UBinv.Invert();
        UBinv /= (2 * M_PI);
        for (size_t i = 0; i < n_peaks; i++)
        {
          V3D hkl = UBinv * peaks[i].getQSampleFrame();
          for (size_t k = 0; k < 3; ++k)
            hkl[k] = floor(hkl[k] + .5);
          hkl_vectors.push_back(hkl);
        }
        OrientedLattice o_lattice;
        std::vector<double> sigabc(7);
        IndexingUtils::Optimize_UB(UB, hkl_vectors, q_vectors, sigabc);

        char logInfo[200];
        int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
        sprintf(logInfo,
            std::string("New UB will index %1d Peaks out of %1d with tolerance %5.3f").c_str(),
            num_indexed, n_peaks, tolerance);
        g_log.notice(std::string(logInfo));

        o_lattice.setUB(UB);
        o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);

        o_lattice.setUB(UB);
        double calc_a = o_lattice.a();
        double calc_b = o_lattice.b();
        double calc_c = o_lattice.c();
        double calc_alpha = o_lattice.alpha();
        double calc_beta = o_lattice.beta();
        double calc_gamma = o_lattice.gamma();
        // Show the modified lattice parameters
        sprintf(logInfo, std::string("Lattice Parameters: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
            calc_a, calc_b, calc_c, calc_alpha, calc_beta, calc_gamma);
        g_log.notice(std::string(logInfo));
        sprintf(logInfo,
            std::string("Lattice Parameters (Refined - Input): %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
            calc_a - a, calc_b - b, calc_c - c, calc_alpha - alpha, calc_beta - beta,
            calc_gamma - gamma);
        g_log.notice(std::string(logInfo));

        g_log.notice()<<std::fixed<<std::setprecision(3)<<std::setw(9);
        g_log.notice()<<"Parameter Errors  :"<<sigabc[0]<<sigabc[1]<<sigabc[2]<<sigabc[3]<<sigabc[4]<<sigabc[5]<<std::endl;
        g_log.notice()<<"Parameter Errors   :"<<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[0]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[1]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[2]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[3]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[4]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[5]
                                              <<std::endl;
        ws->mutableSample().setOrientedLattice(new OrientedLattice(o_lattice));
      }
  }


} // namespace Mantid
} // namespace Crystal

