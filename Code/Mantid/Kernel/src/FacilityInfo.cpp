//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include <Poco/StringTokenizer.h>

#include <algorithm>
#include <iostream>

namespace Mantid
{
namespace Kernel
{

Logger& FacilityInfo::g_log(Logger::get("FacilityInfo"));

/** Constructor.
  * @elem The Poco::XML::Element to read the data from
  * @throws std::runtime_error if name or file extensions are not defined
  */
FacilityInfo::FacilityInfo(const Poco::XML::Element* elem) : 
  m_name(elem->getAttribute("name")), m_zeroPadding(0), m_extensions(), m_archiveSearch(), 
  m_instruments()
{
  if (m_name.empty())
  {
    g_log.error("Facility name is not defined");
    throw std::runtime_error("Facility name is not defined");
  }
  std::string paddingStr = elem->getAttribute("zeropadding");
  if ( paddingStr.empty() || !StrFunc::convert(paddingStr,m_zeroPadding) )
  {
    m_zeroPadding = 0;
  }

  std::string extsStr = elem->getAttribute("FileExtensions");
  if (extsStr.empty())
  {
    g_log.error("No file extensions defined");
    throw std::runtime_error("No file extensions defined");
  }
  typedef Poco::StringTokenizer tokenizer;
  tokenizer exts(extsStr, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (tokenizer::Iterator it = exts.begin(); it != exts.end(); ++it)
  {
    addExtension(*it);
  }

  Poco::XML::NodeList* pNL_archives = elem->getElementsByTagName("archive");
  if (pNL_archives->length() > 1)
  {
    g_log.error("Facility must have only one archive tag");
    throw std::runtime_error("Facility must have only one archive tag");
  }
  else if (pNL_archives->length() == 1)
  {
    Poco::XML::NodeList* pNL_interfaces = elem->getElementsByTagName("archiveSearch");
    for (unsigned int i = 0; i < pNL_interfaces->length(); ++i)
    {
      Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_interfaces->item(i));
      std::string plugin = elem->getAttribute("plugin");
      if (!plugin.empty())
      {
        m_archiveSearch.insert(plugin);
      }
    }

  }

  Poco::XML::NodeList* pNL_instrument = elem->getElementsByTagName("instrument");
  unsigned int n = pNL_instrument->length();

  for (unsigned int i = 0; i < n; ++i)
  {
    Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_instrument->item(i));
    if (elem)
    {
      try
      {
        InstrumentInfo instr(this,elem);
        m_instruments.push_back(instr);
      }
      catch(...)
      {/*skip this instrument*/}
    }
  }

  if (m_instruments.empty())
  {
    throw std::runtime_error("Facility "+m_name+" does not have any instrument;");
  }
}

/**
  * Add new extension. Adds both a lowercase and uppercase version
  * @param ext File extension, including the dot, e.g. ".nxs" or ".raw"
  */
void FacilityInfo::addExtension(const std::string& ext)
{
  std::string casedExt(ext); 
  std::transform(ext.begin(), ext.end(), casedExt.begin(), tolower);
  std::vector<std::string>::iterator it = std::find(m_extensions.begin(),m_extensions.end(),casedExt);
  if (it == m_extensions.end())
  {
    m_extensions.push_back(casedExt);
    std::transform(ext.begin(), ext.end(), casedExt.begin(), toupper);
    m_extensions.push_back(casedExt);
  }
}

/**
  * Returns instruments with given name
  * @param  iName Instrument name
  * @throws NotFoundError if iName was not found
  */
const InstrumentInfo FacilityInfo::Instrument(const std::string& iName)const
{
  std::string iname;
  if (iName.empty())
  {
    iname = ConfigService::Instance().getString("default.instrument");
    if (iname.empty())
    {
      return m_instruments.front();
    }
  }
  else
  {
    iname = iName;
  }
  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for(;it != m_instruments.end(); ++it)
  {
    if (it->name() == iname)
    {
      return *it;
    }
  }

  // if unsuccessful try shortname
  for(it = m_instruments.begin(); it != m_instruments.end(); ++it)
  {
    if (it->shortName() == iname)
    {
      return *it;
    }
  }
  g_log.error("Instrument "+iname+" not found in facility "+name());
  throw Exception::NotFoundError("FacilityInfo",iname);
}

/**
  * Returns a list of instruments of given technique
  * @param tech Technique name
  */
const std::vector<InstrumentInfo> FacilityInfo::Instruments(const std::string& tech)const
{
  std::vector<InstrumentInfo> out;
  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for(;it != m_instruments.end(); ++it)
  {
    if (it->techniques().count(tech))
    {
      out.push_back(*it);
    }
  }
  return out;
}


} // namespace Kernel
} // namespace Mantid
