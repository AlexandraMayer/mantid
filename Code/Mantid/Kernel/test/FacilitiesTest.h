#ifndef MANTID_FACILITIESTEST_H_
#define MANTID_FACILITIESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ConfigService.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"

#include <string>
#include <fstream>

using namespace Mantid::Kernel;
using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;

class FacilitiesTest : public CxxTest::TestSuite
{
public: 

  FacilitiesTest()
  {
	  
  }

  void testFacilities()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\"nxs,raw,sav,n*,s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ADataSearch\" />"
      "      <archiveSearch plugin=\"BDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"WISH\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "      <technique>Single Crystal Diffraction</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo* fac = getFacility(xmlStr);

    TS_ASSERT(fac);

    TS_ASSERT_EQUALS(fac->name(),"ISIS");
    TS_ASSERT_EQUALS(fac->zeroPadding(),5);
    const std::vector<std::string> exts = fac->extensions();
    TS_ASSERT_EQUALS(exts.size(),5);
    TS_ASSERT_EQUALS(exts[0],"nxs");
    TS_ASSERT_EQUALS(exts[1],"raw");
    TS_ASSERT_EQUALS(exts[2],"sav");
    TS_ASSERT_EQUALS(exts[3],"n*");
    TS_ASSERT_EQUALS(exts[4],"s*");
    TS_ASSERT_EQUALS(fac->preferredExtension(),"nxs");

    TS_ASSERT_EQUALS(fac->archiveSearch().size(),2);
    std::set<std::string>::const_iterator it = fac->archiveSearch().begin();
    TS_ASSERT_EQUALS(*it,"ADataSearch");
    TS_ASSERT_EQUALS(*++it,"BDataSearch");

    const std::vector<InstrumentInfo> instrums = fac->Instruments();
    TS_ASSERT_EQUALS(instrums.size(),2);

    TS_ASSERT_THROWS_NOTHING(fac->Instrument("HRPD"));
    InstrumentInfo instr = fac->Instrument("HRPD");
    TS_ASSERT_EQUALS(instr.name(),"HRPD");
    TS_ASSERT_EQUALS(instr.shortName(),"HRP");
    TS_ASSERT_EQUALS(instr.zeroPadding(),5);

    TS_ASSERT_THROWS_NOTHING(fac->Instrument("WISH"));
    instr = fac->Instrument("WISH");
    TS_ASSERT_EQUALS(instr.name(),"WISH");
    TS_ASSERT_EQUALS(instr.shortName(),"WISH");
    TS_ASSERT_EQUALS(instr.zeroPadding(),8);

    const std::vector<InstrumentInfo> pwdInstr = fac->Instruments("Powder Diffraction");
    TS_ASSERT_EQUALS(pwdInstr.size(),2);

    const std::vector<InstrumentInfo> crysInstr = fac->Instruments("Single Crystal Diffraction");
    TS_ASSERT_EQUALS(crysInstr.size(),1);

    delete fac;
  }

  void testConfigService()
  {
    TS_ASSERT_THROWS_NOTHING(
      const FacilityInfo& fac = ConfigService::Instance().Facility("ISIS")
    );
  }

  void testDefaultFacility()
  {
    TS_ASSERT_THROWS_NOTHING(
      const FacilityInfo& fac = ConfigService::Instance().Facility()
    );
    const FacilityInfo& fac = ConfigService::Instance().Facility();
    TS_ASSERT_EQUALS(fac.name(),"ISIS");

    ConfigService::Instance().setString("default.facility","SNS");
    const FacilityInfo& fac1 = ConfigService::Instance().Facility();
    TS_ASSERT_EQUALS(fac1.name(),"SNS");

    ConfigService::Instance().setString("default.facility","");
    const FacilityInfo& fac2 = ConfigService::Instance().Facility();
    TS_ASSERT_EQUALS(fac2.name(),"ISIS");
  }

  void testDefaultInstrument()
  {
    ConfigService::Instance().setString("default.instrument","HRPD");
    const FacilityInfo& fac = ConfigService::Instance().Facility();
    InstrumentInfo instr = fac.Instrument();
    TS_ASSERT_EQUALS(instr.name(),"HRPD");
  }

  void testFacilitiesArchiveMissing()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\"nxs,raw,sav,n*,s*\">"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"WISH\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "      <technique>Single Crystal Diffraction</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo* fac = getFacility(xmlStr);

    TS_ASSERT(fac);

    TS_ASSERT_EQUALS(fac->name(),"ISIS");
    TS_ASSERT_EQUALS(fac->archiveSearch().size(),0);
  }

private:

  FacilityInfo* getFacility(const std::string& xmlStr)const
  {
    DOMParser parser;
    Document* pDoc = parser.parseString(xmlStr);
    TS_ASSERT(pDoc);

    Poco::XML::Element* pRootElem = pDoc->documentElement();
    Poco::XML::NodeList* pNL_facility = pRootElem->getElementsByTagName("facility");
    unsigned int n = pNL_facility->length();

    TS_ASSERT(n > 0);

    Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_facility->item(0));
    TS_ASSERT(elem);

    return new FacilityInfo(elem);
  }

};

#endif /*MANTID_FACILITIESTEST_H_*/
