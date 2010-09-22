#ifndef CORRECTTOFILE_H_
#define CORRECTTOFILE_H_

//-------------------
// Includes
//--------------------
#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CorrectToFile.h"
#include "MantidDataHandling/LoadRKH.h"
#include "MantidKernel/UnitFactory.h"
#include "WorkspaceCreationHelper.hh"
#include "Poco/Path.h"

class CorrectToFileTest : public CxxTest::TestSuite
{
public:

  CorrectToFileTest() : inputFile("")
  {
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/DIRECT.041").toString();
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( correctToFile.initialize() );
    TS_ASSERT( correctToFile.isInitialized() );
  }

  // This algorithm really just runs LoadRKH, RebinToWorkspace and then Divide
  // so given that each of those has its own test, this test does not need to be
  // that complicated
  void testExec2D()
  {
    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 102, 1.5);
    testInput->getAxis(0)->unit() =  Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_DELTA( testInput->readY(0)[0], 2.0 , 0.0001 );

    MatrixWorkspace_sptr data = executeAlgorithm(testInput);

    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 10 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 103);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 102);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(0)[0], 0.6986 , 0.0001 );

    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }

  void testExecEvent()
  {
    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::CreateEventWorkspace(10, 102, 100, 1.5);
    testInput->getAxis(0)->unit() =  Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_DELTA( testInput->readY(1)[0], 1.0 , 0.0001 );

    MatrixWorkspace_sptr data = executeAlgorithm(testInput);

    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 10 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 103);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 102);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(1)[0], 0.3493 , 0.0001 );


    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }

  MatrixWorkspace_sptr executeAlgorithm(MatrixWorkspace_sptr testInput)
  {

    if( !correctToFile.isInitialized() ) correctToFile.initialize();

    //Executing now should throw since none of the properties have been set
    TS_ASSERT_THROWS( correctToFile.execute(), std::runtime_error );

    //Register this with the service
    using namespace Mantid::API;
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("CorrectThis", testInput) );

    //Set the properties
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("WorkspaceToCorrect", "CorrectThis"));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("Filename", inputFile));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("FirstColumnValue", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("WorkspaceOperation", "Divide"));
    std::string outputSpace("outputTest");
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("OutputWorkspace", outputSpace));

    //check that retrieving the filename and output workspace gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = correctToFile.getPropertyValue("Filename") )
    TS_ASSERT( result.compare(inputFile) == 0 );

    TS_ASSERT_THROWS_NOTHING( result = correctToFile.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now not throw anything
    TS_ASSERT_THROWS_NOTHING( correctToFile.execute() );
    TS_ASSERT( correctToFile.isExecuted() );
    
    //Now need to test the resultant workspace, first retrieve it
    Workspace_sptr wkspOut;
    TS_ASSERT_THROWS_NOTHING( wkspOut = AnalysisDataService::Instance().retrieve(outputSpace) );
    MatrixWorkspace_sptr data = boost::dynamic_pointer_cast<MatrixWorkspace>(wkspOut);

    //cleanup the input workspace
    AnalysisDataService::Instance().remove(testInput->getName());

    return data;
  }
  
private:
  Mantid::Algorithms::CorrectToFile correctToFile;
  std::string inputFile;
};

#endif //CORRECTTOFILE_H_
