//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include "Poco/File.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadGSS)

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
* Initialise the algorithm
*/
void LoadGSS::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load), "The input filename of the stored data");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
}

/**
* Execute the algorithm
*/
void LoadGSS::exec()
{
  using namespace Mantid::API;
  std::string filename = getProperty("Filename");

  std::vector<MantidVec*> gsasDataX;
  std::vector<MantidVec*> gsasDataY;
  std::vector<MantidVec*> gsasDataE;

  std::string wsTitle;

  std::ifstream input(filename.c_str(), std::ios_base::in);

  MantidVec* X = new MantidVec();
  MantidVec* Y = new MantidVec();
  MantidVec* E = new MantidVec();

  char currentLine[256];

  // Gather data
  if ( input.is_open() )
  {
    if ( ! input.eof() )
    {
      // Get workspace title (should be first line)
      input.getline(currentLine, 256);
      wsTitle = currentLine;
    }
    while ( ! input.eof() && input.getline(currentLine, 256) )
    {
      double bc1;
      double bc2;
      double bc4;
      if (  currentLine[0] == '\n' || currentLine[0] == '#' )
      {
        continue;
      }
      else if ( currentLine[0] == 'B' )
      {
        if ( X->size() != 0 )
        {
          gsasDataX.push_back(X);
          gsasDataY.push_back(Y);
          gsasDataE.push_back(E);
        }

        /* BANK <SpectraNo> <NBins> <NBins> RALF <BC1> <BC2> <BC1> <BC4> 
        *  BC1 = X[0] * 32
        *  BC2 = X[1] * 32 - BC1
        *  BC4 = ( X[1] - X[0] ) / X[0]
        */
        X = new MantidVec();
        Y = new MantidVec();
        E = new MantidVec();

        std::istringstream inputLine(currentLine, std::ios::in);
        inputLine.ignore(256, 'F');
        inputLine >> bc1 >> bc2 >> bc1 >> bc4;

        double x0 = bc1 / 32;
        X->push_back(x0);
      }
      else
      {
        double xValue;
        double yValue;
        double eValue;

        double xPrev = X->back();

        std::istringstream inputLine(currentLine, std::ios::in);
        inputLine >> xValue >> yValue >> eValue;

        xValue = (2 * xValue) - xPrev;
        yValue = yValue / ( xPrev * bc4 );
        eValue = eValue / ( xPrev * bc4 );
        X->push_back(xValue);
        Y->push_back(yValue);
        E->push_back(eValue);
      }
    }
    if ( X->size() != 0 )
    { // Put final spectra into data
      gsasDataX.push_back(X);
      gsasDataY.push_back(Y);
      gsasDataE.push_back(E);
    }
    input.close();
  }

  int nHist = gsasDataX.size();
  int xWidth = X->size();
  int yWidth = Y->size();

  // Create workspace
  MatrixWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace> (WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
  // GSS Files data is always in TOF
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  // Set workspace title
  outputWorkspace->setTitle(wsTitle);
  // Put data from MatidVec's into outputWorkspace
  for ( unsigned int i = 0; i < nHist; ++i )
  {
    // Move data across
    outputWorkspace->dataX(i) = *gsasDataX[i];
    outputWorkspace->dataY(i) = *gsasDataY[i];
    outputWorkspace->dataE(i) = *gsasDataE[i];
  }

  setProperty("OutputWorkspace", outputWorkspace);
  return;
}