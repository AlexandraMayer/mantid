#ifndef DOWNLOADDATAFILE_H_
#define DOWNLOADDATAFILE_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidICat/GetDataFiles.h"
#include "MantidICat/Search.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include <iomanip>
#include <fstream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace Mantid::API;
class DownloadDataFileTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( downloadobj.initialize());
		TS_ASSERT( downloadobj.isInitialized() );
	}
	void xtestDownLoadDataFile()
	{		
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
	
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

				
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		searchobj.setPropertyValue("StartRun", "100.0");
		searchobj.setPropertyValue("EndRun", "102.0");
		searchobj.setPropertyValue("Instrument","HET");
		searchobj.setPropertyValue("OutputWorkspace","investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if ( !invstObj.isInitialized() ) invstObj.initialize();
		invstObj.setPropertyValue("InvestigationId","13539191");
		invstObj.setPropertyValue("OutputWorkspace","investigation");//selected invesigation
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );
		
		clock_t start=clock();
		if ( !downloadobj.isInitialized() ) downloadobj.initialize();

		downloadobj.setPropertyValue("Filenames","HET00097.RAW");
		
		TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

		clock_t end=clock();
		float diff = float(end - start)/CLOCKS_PER_SEC;

		std::string filepath=Kernel::ConfigService::Instance().getString("defaultsave.directory");
		filepath += "download_time.txt";

		std::ofstream ofs(filepath.c_str(), std::ios_base::out );
		if ( ofs.rdstate() & std::ios::failbit )
		{
			throw Mantid::Kernel::Exception::FileError("Error on creating File","download_time.txt");
		}
		ofs<<"Time taken to  download files with investigation id 12576918 is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
		
						
		TS_ASSERT( downloadobj.isExecuted() );
		//delete the file after execution
		//remove("HET00097.RAW");

		AnalysisDataService::Instance().remove("investigations");
		AnalysisDataService::Instance().remove("investigation");
    // Clean up test files
    if (Poco::File(filepath).exists()) Poco::File(filepath).remove();
	}

	void xtestDownLoadNexusFile()
	{				
		Session::Instance();

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
			
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

				
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		searchobj.setPropertyValue("StartRun", "17440.0");
		searchobj.setPropertyValue("EndRun", "17556.0");
		searchobj.setPropertyValue("Instrument","EMU");
		searchobj.setPropertyValue("OutputWorkspace","investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if ( !invstObj.isInitialized() ) invstObj.initialize();
	
		invstObj.setPropertyValue("InvestigationId", "24070400");

		invstObj.setPropertyValue("OutputWorkspace","investigation");
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );
		
		clock_t start=clock();
		if ( !downloadobj.isInitialized() ) downloadobj.initialize();

		downloadobj.setPropertyValue("Filenames","EMU00017452.nxs");
		TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

		clock_t end=clock();
		float diff = float(end -start)/CLOCKS_PER_SEC;
		
		std::string filepath=Kernel::ConfigService::Instance().getString("defaultsave.directory");
		filepath += "download_time.txt";
		
		std::ofstream ofs(filepath.c_str(), std::ios_base::out | std::ios_base::app);
		if ( ofs.rdstate() & std::ios::failbit )
		{
			throw Mantid::Kernel::Exception::FileError("Error on creating File","download_time.txt");
		}
		ofs<<"Time taken to download files with investigation id 24070400 is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
		//ofs.close();
		
		TS_ASSERT( downloadobj.isExecuted() );
		//delete the file after execution
		//remove("EMU00017452.nxs");
		AnalysisDataService::Instance().remove("investigations");
		AnalysisDataService::Instance().remove("investigation");
    // Clean up test files
    if (Poco::File(filepath).exists()) Poco::File(filepath).remove();
	}

	void xtestDownLoadDataFile_Merlin()
	{
		
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
	
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

				
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		searchobj.setPropertyValue("StartRun", "600.0");
		searchobj.setPropertyValue("EndRun", "601.0");
		searchobj.setPropertyValue("Instrument","MERLIN");
		searchobj.setPropertyValue("OutputWorkspace","investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if ( !invstObj.isInitialized() ) invstObj.initialize();
		invstObj.setPropertyValue("InvestigationId","24022007");
		invstObj.setPropertyValue("OutputWorkspace","investigation");//selected invesigation
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );
		
		clock_t start=clock();
		if ( !downloadobj.isInitialized() ) downloadobj.initialize();

		downloadobj.setPropertyValue("Filenames","MER00599.raw");
			
		TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

		clock_t end=clock();
		float diff = float(end - start)/CLOCKS_PER_SEC;

		std::string filepath=Kernel::ConfigService::Instance().getString("defaultsave.directory");
		filepath += "download_time.txt";

		std::ofstream ofs(filepath.c_str(), std::ios_base::out | std::ios_base::app);
		if ( ofs.rdstate() & std::ios::failbit )
		{
			throw Mantid::Kernel::Exception::FileError("Error on creating File","download_time.txt");
		}
		ofs<<"Time taken to download files with investigation id 24022007 is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
		
						
		TS_ASSERT( downloadobj.isExecuted() );
		AnalysisDataService::Instance().remove("investigations");
		AnalysisDataService::Instance().remove("investigation");
    // Clean up test files
    if (Poco::File(filepath).exists()) Poco::File(filepath).remove();
	}

	void testDownloaddataFile1()
	{	
		std::string filepath=Kernel::ConfigService::Instance().getString("defaultsave.directory");
		filepath += "download_time.txt";
		std::ofstream ofs(filepath.c_str(), std::ios_base::out | std::ios_base::app);
		if ( ofs.rdstate() & std::ios::failbit )
		{
			throw Mantid::Kernel::Exception::FileError("Error on creating File","download_time.txt");
		}

		CDownloadDataFile downloadobj1;
		clock_t start=clock();
		downloadobj1.testDownload("http://download.mantidproject.org/videos/Installation.htm","test.htm");
		clock_t end=clock();
		float diff = float(end -start)/CLOCKS_PER_SEC;

		ofs<<"Time taken for http download from mantidwebserver over internet for a small file of size 1KB is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;

    //delete the file after execution
		remove("test.htm");
    // Clean up test files
//    std::string htmPath = Kernel::ConfigService::Instance().getString("defaultsave.directory") + "/test.htm";
//    if (Poco::File(htmPath).exists()) Poco::File(htmPath).remove();
//    if (Poco::File(filepath).exists()) Poco::File(filepath).remove();
	}

private:
	    CSearch searchobj;
	    CGetDataFiles invstObj;
		CDownloadDataFile downloadobj;
		Login loginobj;
};
#endif
