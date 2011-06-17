//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Glob.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>

#include <cctype>
#include <algorithm>

namespace Mantid
{
  namespace API
  {
    using std::string;

    // this allowed string could be made into an array of allowed, currently used only by the ISIS SANS group
    const std::string FileFinderImpl::ALLOWED_SUFFIX = "-add";
    //----------------------------------------------------------------------
    // Public member functions
    //----------------------------------------------------------------------
    /**
     * Default constructor
     */
    FileFinderImpl::FileFinderImpl() : g_log(Mantid::Kernel::Logger::get("FileFinderImpl"))
    {
      // Make sure plugins are loaded
      std::string libpath = Kernel::ConfigService::Instance().getString("plugins.directory");
      if (!libpath.empty())
      {
        Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
      }
    }

    /**
     * Return the full path to the file given its name
     * @param fName :: A full file name (without path) including extension
     * @return The full path if the file exists and can be found in one of the search locations
     *  or an empty string otherwise.
     */
    std::string FileFinderImpl::getFullPath(const std::string& fName) const
    {
      g_log.debug() << "getFullPath(" << fName << ")\n";
      // If this is already a full path, nothing to do
      if (Poco::Path(fName).isAbsolute())
        return fName;

      // First try the path relative to the current directory. Can throw in some circumstances with extensions that have wild cards
      try
      {
        Poco::File fullPath(Poco::Path().resolve(fName));
        if (fullPath.exists())
          return fullPath.path();
      }
      catch (std::exception&)
      {
      }

      const std::vector<std::string>& searchPaths =
          Kernel::ConfigService::Instance().getDataSearchDirs();
      std::vector<std::string>::const_iterator it = searchPaths.begin();
      for (; it != searchPaths.end(); ++it)
      {
        if (fName.find("*") != std::string::npos)
        {
          Poco::Path path(*it, fName);
          Poco::Path pathPattern(path);
          std::set < std::string > files;
          Kernel::Glob::glob(pathPattern, files);
          if (!files.empty())
          {
            return *files.begin();
          }
        }
        else
        {
          Poco::Path path(*it, fName);
          Poco::File file(path);
          if (file.exists())
          {
            return path.toString();
          }
        }
      }
      return "";
    }

    /** Run numbers can be followed by an allowed string. Check if there is
     *  one, remove it from the name and return the string, else return empty
     *  @param userString run number that may have a suffix
     *  @return the suffix, if there was one
     */
    std::string FileFinderImpl::extractAllowedSuffix(std::string & userString) const
    {
      if (userString.find(ALLOWED_SUFFIX) == std::string::npos)
      {
        //short cut processing as normally there is no suffix
        return "";
      }

      // ignore any file extension in checking if a suffix is present
      Poco::Path entry(userString);
      std::string noExt(entry.getBaseName());
      const size_t repNumChars = ALLOWED_SUFFIX.size();
      if (noExt.find(ALLOWED_SUFFIX) == noExt.size() - repNumChars)
      {
        userString.replace(userString.size() - repNumChars, repNumChars, "");
        return ALLOWED_SUFFIX;
      }
      return "";
    }

    /**
     * Return the name of the facility as determined from the hint.
     *
     * @param hint :: The name hint.
     * @return This will return the default facility if it cannot be determined.
     */
    const Kernel::FacilityInfo FileFinderImpl::getFacility(const string& hint) const
    {
      if ((!hint.empty()) && (!isdigit(hint[0])))
      {
        string instrName(hint);
        Poco::Path path(instrName);
        instrName = path.getFileName();
        if ((instrName.find("PG3") == 0) || (instrName.find("pg3") == 0))
        {
          instrName = instrName.substr(0, 3);
        }
        else
        {
          // go forwards looking for the run number to start
          {
            string::const_iterator it = std::find_if(instrName.begin(), instrName.end(), std::ptr_fun(isdigit));
            std::string::size_type nChars = std::distance( static_cast<string::const_iterator>(instrName.begin()), it);
            instrName = instrName.substr(0, nChars);
          }

          // go backwards looking for the instrument name to end - gets around delimiters
          if (!instrName.empty())
          {
            string::const_reverse_iterator it = std::find_if(instrName.rbegin(), instrName.rend(),
                                                             std::ptr_fun(isalpha));
            string::size_type nChars = std::distance(it,
                                        static_cast<string::const_reverse_iterator>(instrName.rend()));
            instrName = instrName.substr(0, nChars);
          }
        }
        try {
          const Kernel::InstrumentInfo instrument = Kernel::ConfigService::Instance().getInstrument(instrName);
          return instrument.facility();
        } catch (Kernel::Exception::NotFoundError &e) {
          g_log.debug() << e.what() << "\n";
        }
      }
      return Kernel::ConfigService::Instance().getFacility();;
    }

    /**
     * Extracts the instrument name and run number from a hint
     * @param hint :: The name hint
     * @return A pair of instrument name and run number
     */
    std::pair<std::string, std::string> FileFinderImpl::toInstrumentAndNumber(const std::string& hint) const
    {
      std::string instrPart;
      std::string runPart;

      if (isdigit(hint[0]))
      {
        instrPart = Kernel::ConfigService::Instance().getInstrument().shortName();
        runPart = hint;
      }
      else
      {
        // PG3 is a special case (name ends in a number)- don't trust them
        if ((hint.find("PG3") == 0) || (hint.find("pg3") == 0)) {
          instrPart = hint.substr(0, 3);
          runPart = hint.substr(3);
        }
        else {
          /// Find the last non-digit as the instrument name can contain numbers
          std::string::const_reverse_iterator it = std::find_if(hint.rbegin(), hint.rend(),
              std::not1(std::ptr_fun(isdigit)));
          // No non-digit or all non-digits
          if (it == hint.rend() || it == hint.rbegin())
          {
            throw std::invalid_argument("Malformed hint to FileFinderImpl::makeFileName: " + hint);
          }
          std::string::size_type nChars = std::distance(it, hint.rend());
          instrPart = hint.substr(0, nChars);
          runPart = hint.substr(nChars);
        }
      }

      Kernel::InstrumentInfo instr = Kernel::ConfigService::Instance().getInstrument(instrPart);
      size_t nZero = instr.zeroPadding();
      // remove any leading zeros in case there are too many of them
      std::string::size_type i = runPart.find_first_not_of('0');
      runPart.erase(0, i);
      while (runPart.size() < nZero)
        runPart.insert(0, "0");
      if (runPart.size() > nZero && nZero != 0)
      {
        throw std::invalid_argument("Run number does not match instrument's zero padding");
      }

      instrPart = instr.shortName();

      return std::make_pair(instrPart, runPart);

    }

    /**
     * Make a data file name (without extension) from a hint. The hint can be either a run number or
     * a run number prefixed with an instrument name/short name. If the instrument
     * name is absent the default one is used.
     * @param hint :: The name hint
     * @return The file name
     * @throw NotFoundError if a required default is not set
     * @throw std::invalid_argument if the argument is malformed or run number is too long
     */
    std::string FileFinderImpl::makeFileName(const std::string& hint, const Kernel::FacilityInfo& facility) const
    {
      if (hint.empty())
        return "";

      std::string filename(hint);
      const std::string suffix = extractAllowedSuffix(filename);

      std::pair < std::string, std::string > p = toInstrumentAndNumber(filename);

      std::string delimiter = facility.delimiter();

      filename = p.first;
      if (!delimiter.empty())
      {
        filename += delimiter;
      }
      filename += p.second;

      if (!suffix.empty())
      {
        filename += suffix;
      }

      return filename;
    }

    /**
     * Find the file given a hint. If the name contains a dot(.) then it is assumed that it is already a file stem
     * otherwise calls makeFileName internally.
     * @param hint :: The name hint, format: [INSTR]1234[.ext]
     * @param exts :: Optional list of allowed extensions. Only those extensions found in both
     *  facilities extension list and exts will be used in the search. If an extension is given in hint 
     *  this argument is ignored.
     * @return The full path to the file or empty string if not found
     */
    std::string FileFinderImpl::findRun(const std::string& hint, const std::set<std::string> *exts) const
    {
      if (hint.empty())
        return "";
      std::vector<std::string> exts_v;
      if (exts != NULL && exts->size() > 0)
        exts_v.assign(exts->begin(), exts->end());

      return this->findRun(hint, exts_v);
    }

    std::string FileFinderImpl::findRun(const std::string& hint,const std::vector<std::string> &exts)const
    {
      g_log.debug() << "findRun(\'" << hint << "\', exts[" << exts.size() << "])\n";
      if (hint.empty())
        return "";

      // if it looks like a full filename just do a quick search for it
      Poco::Path hintPath(hint);
      if (!hintPath.getExtension().empty())
      {
        // check in normal search locations
        std::string path = getFullPath(hint);
        if (!path.empty() && Poco::File(path).exists())
        {
          return path;
        }
      }

      // so many things depend on the facility just get it now
      const Kernel::FacilityInfo facility = this->getFacility(hint);
      // initialize the archive searcher
      IArchiveSearch_sptr arch;
      { // hide in a local namespace so things fall out of scope
        std::string archiveOpt = Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
        std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(), tolower);
        if (!archiveOpt.empty() && archiveOpt != "off" && !facility.archiveSearch().empty())
        {
          g_log.debug() << "Starting archive search..." << *facility.archiveSearch().begin() << "\n";
          arch = ArchiveSearchFactory::Instance().create(*facility.archiveSearch().begin());
        }
      }

      // ask the archive search for help
      if (!hintPath.getExtension().empty())
      {
        if (arch)
        {
          std::string path = arch->getPath(hint);
          Poco::File file(path);
          if (file.exists())
          {
            return file.path();
          }
        }
      }

      // Do we need to try and form a filename from our preset rules
      std::string filename(hint);
      std::string extension;
      if (hintPath.depth() == 0)
      {
        std::size_t i = filename.find_last_of('.');
        if (i != std::string::npos)
        {
          extension = filename.substr(i);
          filename.erase(i);
        }
        filename = makeFileName(filename, facility);
      }

      // work through the extensions
      const std::vector<std::string> facility_extensions = facility.extensions();
      // select allowed extensions
      std::vector < std::string > extensions;
      if (!extension.empty())
      {
        extensions.push_back(extension);
      }
      else if (!exts.empty())
      {
        extensions.insert(extensions.end(), exts.begin(), exts.end());
        // find intersection of facility_extensions and exts, preserving the order of facility_extensions
        std::vector<std::string>::const_iterator it = facility_extensions.begin();
        for (; it != facility_extensions.end(); ++it)
        {
          if (std::find(exts.begin(), exts.end(), *it) == exts.end())
          {
            extensions.push_back(*it);
          }
        }
      }
      else
      {
        extensions.assign(facility_extensions.begin(), facility_extensions.end());
      }
      // Look first at the original filename then for case variations. This is important
      // on platforms where file names ARE case sensitive.
      std::vector<std::string> filenames(3,filename);
      std::transform(filename.begin(),filename.end(),filenames[1].begin(),toupper);
      std::transform(filename.begin(),filename.end(),filenames[2].begin(),tolower);
      std::vector<std::string>::const_iterator ext = extensions.begin();
      for (; ext != extensions.end(); ++ext)
      {
        for(size_t i = 0; i < filenames.size(); ++i)
        {
          std::string path = getFullPath(filenames[i] + *ext);
          if (!path.empty())
            return path;
        }
      }

      // Search the archive of the default facility
      if (arch)
      {
        std::string path;
        std::vector<std::string>::const_iterator ext = extensions.begin();
        for (; ext != extensions.end(); ++ext)
        {
          for(size_t i = 0; i < filenames.size(); ++i)
          {
            path = arch->getPath(filenames[i] + *ext);
            Poco::Path pathPattern(path);
            if (ext->find("*") != std::string::npos)
            {
              continue;
              std::set < std::string > files;
              Kernel::Glob::glob(pathPattern, files);
            }
            else
            {
              Poco::File file(pathPattern);
              if (file.exists())
              {
                return file.path();
              }
            }
          } // i
        }  // ext
      } // arch

      return "";
    }

    /**
     * Find a list of files file given a hint. Calls findRun internally.
     * @param hint :: Comma separated list of hints to findRun method.
     *  Can also include ranges of runs, e.g. 123-135 or equivalently 123-35.
     *  Only the beginning of a range can contain an instrument name.
     * @return A vector of full paths or empty vector
     * @throw std::invalid_argument if the argument is malformed
     */
    std::vector<std::string> FileFinderImpl::findRuns(const std::string& hint) const
    {
      std::vector < std::string > res;
      Poco::StringTokenizer hints(hint, ",",
          Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
      Poco::StringTokenizer::Iterator h = hints.begin();

      for (; h != hints.end(); ++h)
      {
        // Quick check for a filename
        bool fileSuspected = false;
        // Assume if the hint contains either a "/" or "\" it is a filename..
        if ((*h).find("\\") != std::string::npos)
        {
          fileSuspected = true;
        }
        if ((*h).find("/") != std::string::npos)
        {
          fileSuspected = true;
        }
        if ((*h).find(ALLOWED_SUFFIX) != std::string::npos)
        {
          fileSuspected = true;
        }

        Poco::StringTokenizer range(*h, "-",
            Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
        if ((range.count() > 2) && (!fileSuspected))
        {
          throw std::invalid_argument("Malformed range of runs: " + *h);
        }
        else if ((range.count() == 2) && (!fileSuspected))
        {
          std::pair < std::string, std::string > p1 = toInstrumentAndNumber(range[0]);
          std::string run = p1.second;
          size_t nZero = run.size(); // zero padding
          if (range[1].size() > nZero)
          {
            ("Malformed range of runs: " + *h
                + ". The end of string value is longer than the instrument's zero padding");
          }
          int runNumber = boost::lexical_cast<int>(run);
          std::string runEnd = run;
          runEnd.replace(runEnd.end() - range[1].size(), runEnd.end(), range[1]);
          int runEndNumber = boost::lexical_cast<int>(runEnd);
          if (runEndNumber < runNumber)
          {
            throw std::invalid_argument("Malformed range of runs: " + *h);
          }
          for (int irun = runNumber; irun <= runEndNumber; ++irun)
          {
            run = boost::lexical_cast<std::string>(irun);
            while (run.size() < nZero)
              run.insert(0, "0");
            std::string path = findRun(p1.first + run);
            if (!path.empty())
            {
              res.push_back(path);
            }
          }

        }
        else
        {
          std::string path = findRun(*h);
          if (!path.empty())
          {
            res.push_back(path);
          }
        }
      }

      return res;
    }

  }// API

}// Mantid

