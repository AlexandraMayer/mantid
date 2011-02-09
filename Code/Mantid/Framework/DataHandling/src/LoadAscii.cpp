//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadAscii)
    //register the algorithm into loadalgorithm factory
    DECLARE_LOADALGORITHM(LoadAscii)

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadAscii::LoadAscii() : m_columnSep(), m_separatorIndex()
    {
    }

    /** This method does a quick file check by checking the no.of bytes read nread params and header buffer
     *  @param filePath :: path of the file including name.
     *  @param nread :: no.of bytes read
     *  @param header :: The first 100 bytes of the file as a union
     *  @return true if the given file is of type which can be loaded by this algorithm
     */
    bool LoadAscii::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      std::string extn=extension(filePath);
      bool bascii(false);
      (!extn.compare("dat")||!extn.compare("csv")|| extn.compare("txt"))?bascii=true:bascii=false;

      bool is_ascii (true);
      for(size_t i=0; i<nread; i++)
      {
        if (!isascii(header.full_hdr[i]))
          is_ascii =false;
      }
      return(is_ascii|| bascii?true:false);
    }

    /**
     * Checks the file by opening it and reading few lines 
     * @param filePath name of the file including its path
     * @return an integer value how much this algorithm can load the file 
     */
    int LoadAscii::fileCheck(const std::string& filePath)
    {      
      std::ifstream file(filePath.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filePath);
        throw Exception::FileError("Unable to open file: " , filePath);
      }
      std::string separators(",");
      int ncols=0; 
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      boost::char_separator<char> seps(separators.c_str());
      std::string line;
      int confidence(0);
      while(getline(file,line))
      { 
        if (line.empty()||line[0] == '#') 
        {
          continue;
        }
        else
        {
          //break at a non empty/non comment line is teh 1st data line
          break;
        }
      }

      // iterate through the first line columns
      boost::tokenizer<boost::char_separator<char> > values(line, seps);
      for (tokenizer::iterator it = values.begin(); it != values.end(); ++it)
      {         
        ++ncols;
      } 
      bool bloadAscii(true);
      //if the data is of double type this file can be loaded by loadascci
      double data;
      for (tokenizer::iterator it = values.begin(); it != values.end(); ++it)
      {       
        std::istringstream is(*it);
        is>>data;
        if(is.fail())
        {
          bloadAscii=false;
          break;
        }
       
      }
      //if the line has odd number of coulmns with mantid supported separators
      // this is considered as ascci file 
      if (ncols % 2 == 1 && ncols > 2 && bloadAscii) 
      {
        confidence = 80;
      }
      return confidence;
    }

    //--------------------------------------------------------------------------
    // Protected methods
    //--------------------------------------------------------------------------
    /**
     * Process the header information. This implementation just skips it entirely.
     * @param file :: A reference to the file stream
     */
    void LoadAscii::processHeader(std::ifstream & file) const
    {
      // Most files will have some sort of header. If we've haven't been told how many lines to 
      // skip then try and guess 
      int numToSkip = getProperty("SkipNumLines");
      if( numToSkip == EMPTY_INT() )
      {
	const int rowsToMatch(5);
	// Have a guess where the data starts. Basically say, when we have say "rowsToMatch" lines of pure numbers
	// in a row then the line that started block is the top of the data
	int numCols(-1), matchingRows(0), row(0);
	std::streampos dataStart(0), previousLine(file.tellg());
	std::string line;
	std::vector<double> values;
	while( getline(file,line) )
	{
	  ++row;
	  boost::trim(line);
	  if( this->skipLine(line) )
	  {
	    previousLine = file.tellg();
	    continue;
	  }
	  
	  std::list<std::string> columns;
	  int lineCols = this->splitIntoColumns(columns, line);
	  try
	  {
	    fillInputValues(values, columns);
	  }
	  catch(boost::bad_lexical_cast&)
	  {
	    previousLine = file.tellg();
	    continue;
	  }
	  if( numCols < 0 ) numCols = lineCols;
	  if( lineCols == numCols )
	  {
	    if( matchingRows == 0 ) dataStart = previousLine;
	    ++matchingRows;
	    if( matchingRows == rowsToMatch ) break;
	  }
	  else
	  {
	    numCols = lineCols;
	    matchingRows = 1;
	    dataStart = previousLine;
	  }
	  previousLine = file.tellg();
	}

	// Seek the file pointer to the correct position to start reading data
	file.seekg(dataStart);
	numToSkip = row;
      }
      else
      {
	int i(0);
	std::string line;
	while( i < numToSkip && getline(file, line) )
	{
	  ++i;
	}
      }
      g_log.information() << "Skipped " << numToSkip << " line(s) of header information()\n";
    }

    /**
     * Reads the data from the file. It is assumed that the provided file stream has its position
     * set such that the first call to getline will be give the first line of data
     * @param file :: A reference to a file stream
     * @returns A pointer to a new workspace
     */
    API::Workspace_sptr LoadAscii::readData(std::ifstream & file) const
    {
      // Estimate how much work we have to do by a simple line count
      std::streampos current = file.tellg();
      file.seekg(std::ios::end);
      Progress progress = Progress(const_cast<LoadAscii*>(this),0,1,file.tellg());
      file.seekg (current);

      // Get the first line and find the number of spectra from the number of columns
      std::string line;
      this->peekLine(file,line);
      std::list<std::string> columns;
      const int numCols = splitIntoColumns(columns, line);
      if( numCols < 2 ) 
      {
	g_log.error() << "Invalid data format found in file \"" << getPropertyValue("Filename") << "\"\n";
	throw std::runtime_error("Invalid data format. Fewer than 2 columns found.");
      }
      int numSpectra(0);
      bool haveErrors(false);
      // Assume single data set with no errors
      if( numCols == 2 )
      {
	numSpectra = numCols/2;
      }
      // Data with errors
      else if( (numCols-1) % 2 == 0 )
      {
	numSpectra = (numCols - 1)/2;
	haveErrors = true;
      }
      else
      {
	g_log.error() << "Invalid data format found in file \"" << getPropertyValue("Filename") << "\"\n";
	g_log.error() << "LoadAscii requires the number of columns to be an even multiple of either 2 or 3.";
	throw std::runtime_error("Invalid data format.");
      }
      
      // A quick check at the number of lines won't be accurate enough as potentially there
      // could be blank lines and comment lines
      int numBins(0), lineNo(0);
      std::vector<DataObjects::Histogram1D> spectra(numSpectra);
      std::vector<double> values(numCols, 0.);
      while( getline(file,line) )
      {
	++lineNo;
	boost::trim(line);
	if( this->skipLine(line) ) continue;
	columns.clear();
	int lineCols = this->splitIntoColumns(columns, line); 
	if( lineCols != numCols )
	{
	  std::ostringstream ostr;
          ostr << "Number of columns changed at line " << lineNo;
	  throw std::runtime_error(ostr.str());
	}

	try
	{
	  fillInputValues(values, columns);
	}
	catch(boost::bad_lexical_cast&)
	{
	  g_log.error() << "Invalid value on line " << lineNo << " of \""
			<< getPropertyValue("Filename") << "\"\n";
	  throw std::runtime_error("Invalid value encountered.");
	}

	for(int i = 0; i < numSpectra; ++i)
        {
          spectra[i].dataX().push_back(values[0]);
          spectra[i].dataY().push_back(values[i*2+1]);
	  if( haveErrors )
	  {
	    spectra[i].dataE().push_back(values[i*2+2]);
	  }
	  else
	  {
	    spectra[i].dataE().push_back(0.0);
	  }
        }
        ++numBins;
	progress.report();
      }

      MatrixWorkspace_sptr localWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>
        (WorkspaceFactory::Instance().create("Workspace2D",numSpectra,numBins,numBins));
      try 
      {
        localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(getProperty("Unit"));
      } 
      catch (Exception::NotFoundError&) 
      {
        // Asked for dimensionless workspace (obviously not in unit factory)
      }

      for(size_t i = 0; i < (size_t)numSpectra; ++i)
      {
        localWorkspace->dataX(i) = spectra[i].dataX();
        localWorkspace->dataY(i) = spectra[i].dataY();
        localWorkspace->dataE(i) = spectra[i].dataE();
        // Just have spectrum number start at 1 and count up
        localWorkspace->getAxis(1)->spectraNo(i) = i+1;
      }
      return localWorkspace;
    }

    /**
     * Peek at a line without extracting it from the stream
     */
    void LoadAscii::peekLine(std::ifstream & is, std::string & str) const
    {
      std::streampos sp(is.tellg());
      getline(is, str);
      is.seekg(sp);
      boost::trim(str);
    }
    
    /**
     * Return true if the line is to be skipped.
     * @param line :: The line to be checked
     * @param returns True if the line should be skipped
     */
    bool LoadAscii::skipLine(const std::string & line) const
    {
      // Empty or comment
      return ( line.empty() || boost::starts_with(line, "#") );
    }

    /**
     * Split the data into columns based on the input separator
     * @param[out] columns :: A reference to a list to store the column data
     * @param[in] str :: The input string
     * @returns The number of columns
     */
    int LoadAscii::splitIntoColumns(std::list<std::string> & columns, const std::string & str) const
    {
      boost::split(columns, str, boost::is_any_of(m_columnSep), boost::token_compress_on);
      return columns.size();
    }

    /**
     * Fill the given vector with the data values. Its size is assumed to be correct
     * @param[out] values :: The data vector fill
     * @param columns :: The list of strings denoting columns
     */
    void LoadAscii::fillInputValues(std::vector<double> &values, 
				    const std::list<std::string>& columns) const
    {
      values.resize(columns.size());
      std::list<std::string>::const_iterator iend = columns.end();
      int i = 0;
      for( std::list<std::string>::const_iterator itr = columns.begin();
	   itr != iend; ++itr )
      {
	std::string value = *itr;
	boost::trim(value);
	values[i] = boost::lexical_cast<double>(value);
	++i;
      }
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /// Initialisation method.
    void LoadAscii::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "A comma separated Ascii file");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created.");

      std::string spacers[5][2] = { {"CSV", ","}, {"Tab", "\t"}, {"Space", " "}, 
				    {"Colon", ":"}, {"SemiColon", ";"} };
      // For the ListValidator
      std::vector<std::string> sepOptions;
      for( size_t i = 0; i < 5; ++i )
      {
	std::string option = spacers[i][0];
	m_separatorIndex.insert(std::pair<std::string,std::string>(option, spacers[i][1]));
	sepOptions.push_back(option);
      }
      declareProperty("Separator", "CSV", new ListValidator(sepOptions),
        "The column separator character (default: CSV)");

      std::vector<std::string> units = UnitFactory::Instance().getKeys();
      units.insert(units.begin(),"Dimensionless");
      declareProperty("Unit","Energy",new Kernel::ListValidator(units),
        "The unit to assign to the X axis (default: Energy)");
      BoundedValidator<int> * mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("SkipNumLines", EMPTY_INT(), mustBePosInt,
		      "If set, this number of lines from the top of the file are ignored.");      
    }

    /** 
     *   Executes the algorithm.
     */
    void LoadAscii::exec()
    {
      std::string filename = getProperty("Filename");
      std::ifstream file(filename.c_str(), std::ifstream::in);
      if (!file)
      {
        g_log.error("Unable to open file: " + filename);
        throw Exception::FileError("Unable to open file: " , filename);
      }

      std::string sepOption = getProperty("Separator");
      m_columnSep = m_separatorIndex[sepOption];
      // Process the header information.
      processHeader(file);
      // Read the data
      Workspace_sptr outputWS = readData(file);
      setProperty("OutputWorkspace", outputWS);
    }

    
  } // namespace DataHandling
} // namespace Mantid
