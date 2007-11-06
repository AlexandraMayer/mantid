#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>

#include "Support.h"

namespace Mantid
{
namespace  StrFunc
{

/// \cond TEMPLATE 

template DLLExport int section(std::string&,double&);
template DLLExport int section(std::string&,float&);
template DLLExport int section(std::string&,int&);
template DLLExport int section(std::string&,std::string&);

template DLLExport int sectPartNum(std::string&,double&);
template DLLExport int sectPartNum(std::string&,int&);
template DLLExport int sectionMCNPX(std::string&,double&);

template DLLExport int convert(const std::string&,double&);
template DLLExport int convert(const std::string&,std::string&);
template DLLExport int convert(const std::string&,int&);
template DLLExport int convert(const char*,std::string&);
template DLLExport int convert(const char*,double&);
template DLLExport int convert(const char*,int&);

template DLLExport int convPartNum(const std::string&,double&);
template DLLExport int convPartNum(const std::string&,int&);

template DLLExport int setValues(const std::string&,const std::vector<int>&,std::vector<double>&);

template DLLExport int writeFile(const std::string&,const double,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<double>&,const std::vector<double>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&);
template DLLExport int writeFile(const std::string&,const std::vector<float>&,const std::vector<float>&,const std::vector<float>&);

/// \endcond TEMPLATE 

void 
printHex(std::ostream& OFS,const int n)
  /*!
    Function to convert and number into hex
    output (and leave the stream un-changed)
    \param OFS :: Output stream
    \param n :: integer to convert
    \todo Change this to a stream operator
  */
{
  std::ios_base::fmtflags PrevFlags=OFS.flags();
  OFS<<"Ox";
  OFS.width(8);
  OFS.fill('0');
  hex(OFS);
  OFS << n;
  OFS.flags(PrevFlags);
  return;
} 


std::string
stripMultSpc(const std::string& Line)
  /*!
    Removes the multiple spaces in the line
    \param Line :: Line to process
    \return String with single space components
  */
{
  std::string Out;
  int spc(1);
  int lastReal(-1);
  for(unsigned int i=0;i<Line.length();i++)
    {
      if (Line[i]!=' ' && Line[i]!='\t' &&
    		Line[i]!='\r' &&  Line[i]!='\n')		
        {
	  lastReal=i;
	  spc=0;
	  Out+=Line[i];
	}
      else if (!spc)
        {
	  spc=1;
	  Out+=' ';
	}
    }
  lastReal++;
  if (lastReal<static_cast<int>(Out.length()))
    Out.erase(lastReal);
  return Out;
}

int
extractWord(std::string& Line,const std::string& Word,const int cnt)
  /*!
    Checks that as least cnt letters of 
    works is part of the string. It is currently 
    case sensative. It removes the Word if found
    \param Line :: Line to process
    \param Word :: Word to use
    \param cnt :: Length of Word for significants [default =4]
    \retval 1 on success (and changed Line) 
    \retval 0 on failure 
  */
{
  if (Word.empty())
    return 0;

  unsigned int minSize(cnt>static_cast<int>(Word.size()) ?  Word.size() : cnt);
  std::string::size_type pos=Line.find(Word.substr(0,minSize));
  if (pos==std::string::npos)
    return 0;
  // Pos == Start of find
  unsigned int LinePt=minSize+pos;
  for(;minSize<Word.size() && LinePt<Line.size()
	&& Word[minSize]==Line[LinePt];LinePt++,minSize++);

  Line.erase(pos,LinePt-(pos-1));
  return 1;
}

int
confirmStr(const std::string& S,const std::string& fullPhrase)
  /*!
    Check to see if S is the same as the
    first part of a phrase. (case insensitive)
    \param S :: string to check
    \param fullPhrase :: complete phrase
    \returns 1 on success 
  */
{
  const int nS(S.length());
  const int nC(fullPhrase.length());
  if (nS>nC || nS<=0)    
    return 0;           
  for(int i=0;i<nS;i++)
    if (S[i]!=fullPhrase[i])
      return 0;
  return 1;
}

int
getPartLine(std::istream& fh,std::string& Out,std::string& Excess,const int spc)
  /*!
    Gets a line and determine if there is addition component to add
    in the case of a very long line.
    \param fh :: input stream to get line 
    \param Out :: string up to last 'tab' or ' '
    \param Excess :: string after 'tab or ' ' 
    \param spc :: number of char to try to read 
    \retval 1 :: more line to be found
    \retval -1 :: Error with file
    \retval 0  :: line finished.
  */
{
  std::string Line;
  if (fh.good())
    {
      char* ss=new char[spc+1];
      const int clen=spc-Out.length();
      fh.getline(ss,clen,'\n');
      ss[clen+1]=0;           // incase line failed to read completely
      Out+=static_cast<std::string>(ss);
      delete [] ss;                   
      // remove trailing comments
      std::string::size_type pos = Out.find_first_of("#!");        
      if (pos!=std::string::npos)
        {
	  Out.erase(pos); 
	  return 0;
	}
      if (fh.gcount()==clen-1)         // cont line
        {
	  pos=Out.find_last_of("\t ");
	  if (pos!=std::string::npos)
	    {
	      Excess=Out.substr(pos,std::string::npos);
	      Out.erase(pos);
	    }
	  else
	    Excess.erase(0,std::string::npos);
	  fh.clear();
	  return 1;
	}
      return 0;
    }
  return -1;
}

std::string
removeSpace(const std::string& CLine)
  /*!
    Removes all spaces from a string 
    except those with in the form '\ '
    \param CLine :: Line to strip
    \return String without space
  */
{
  std::string Out;
  char prev='x';
  for(unsigned int i=0;i<CLine.length();i++)
    {
      if (!isspace(CLine[i]) || prev=='\\')
        {
	  Out+=CLine[i];
	  prev=CLine[i];
	}
    }
  return Out;
}
	

std::string 
getLine(std::istream& fh,const int spc)
  /*!
    Reads a line from the stream of max length spc.
    Trailing comments are removed. (with # or ! character)
    \param fh :: already open file handle
    \param spc :: max number of characters to read 
    \return String read.
  */

{
  char* ss=new char[spc+1];
  std::string Line;
  if (fh.good())
    {
      fh.getline(ss,spc,'\n');
      ss[spc]=0;           // incase line failed to read completely
      Line=ss;
      // remove trailing comments
      std::string::size_type pos = Line.find_first_of("#!");
      if (pos!=std::string::npos)
	Line.erase(pos); 
    }
  delete [] ss;
  return Line;
}

int
isEmpty(const std::string& A)
  /*!
    Determines if a string is only spaces
    \param A :: string to check
    \returns 1 on an empty string , 0 on failure
  */
{
  std::string::size_type pos=
    A.find_first_not_of(" \t");
  return (pos!=std::string::npos) ? 0 : 1;
}

void
stripComment(std::string& A)
  /*!
    removes the string after the comment type of 
    '$ ' or '!' or '#  '
    \param A :: String to process
  */
{
  std::string::size_type posA=A.find("$ ");
  std::string::size_type posB=A.find("# ");
  std::string::size_type posC=A.find("!");
  if (posA>posB)
    posA=posB;
  if (posA>posC)
    posA=posC;
  if (posA!=std::string::npos)
    A.erase(posA,std::string::npos);
  return;
}

std::string
fullBlock(const std::string& A)
  /*!
    Returns the string from the first non-space to the 
    last non-space 
    \param A :: string to process
    \returns shortened string
  */
{
  std::string::size_type posA=A.find_first_not_of(" ");
  std::string::size_type posB=A.find_last_not_of(" ");
  if (posA==std::string::npos)
    return "";
  return A.substr(posA,1+posB-posA);
}

template<typename T>
int
sectPartNum(std::string& A,T& out)
  /*!
    Takes a character string and evaluates 
    the first [typename T] object. The string is then 
    erase upt to the end of number.
    The diffierence between this and section is that
    it allows trailing characters after the number. 
    \param out :: place for output
    \param A :: string to process
    \returns 1 on success 0 on failure
   */ 
{
  if (A.empty())
    return 0;

  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  const int xpt=cx.tellg();
  if (xpt<0)
    return 0;
  A.erase(0,xpt);
  out=retval;
  return 1; 
}

template<typename T>
int 
section(char* cA,T& out)
  /*!
    Takes a character string and evaluates 
    the first [typename T] object. The string is then filled with
    spaces upto the end of the [typename T] object
    \param out :: place for output
    \param cA :: char array for input and output. 
    \returns 1 on success 0 on failure
   */ 
{
  if (!cA) return 0;
  std::string sA(cA);
  const int item(section(sA,out));
  if (item)
    {
      strcpy(cA,sA.c_str());
      return 1;
    }
  return 0;
}

template<typename T>
int
section(std::string& A,T& out)
  /* 
    takes a character string and evaluates 
    the first <T> object. The string is then filled with
    spaces upto the end of the <T> object
    \param out :: place for output
    \param A :: string for input and output. 
    \return 1 on success 0 on failure
  */
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (cx.fail())
    return 0;
  const int xpt=cx.tellg();
  const char xc=cx.get();
  if (!cx.fail() && !isspace(xc))
    return 0;
  A.erase(0,xpt);
  out=retval;
  return 1;
}

template<typename T>
int
sectionMCNPX(std::string& A,T& out)
  /* 
    Takes a character string and evaluates 
    the first [T] object. The string is then filled with
    spaces upto the end of the [T] object.
    This version deals with MCNPX numbers. Those
    are numbers that are crushed together like
    - 5.4938e+04-3.32923e-6
    \param out :: place for output
    \param A :: string for input and output. 
    \return 1 on success 0 on failure
  */
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (!cx.fail())
    {
      int xpt=cx.tellg();
      const char xc=cx.get();
      if (!cx.fail() && !isspace(xc) 
	  && (xc!='-' || xpt<5))
	return 0;
      A.erase(0,xpt);
      out=retval;
      return 1;
    }
  return 0;
}

void
writeMCNPX(const std::string& Line,std::ostream& OX)
  /*!
    Write out the line in the limited form for MCNPX
    ie initial line from 0->72 after that 8 to 72
    (split on a space or comma)
    \param Line :: full MCNPX line
    \param OX :: ostream to write to
  */
{
  const int MaxLine(72);
  std::string::size_type pos(0);
  std::string X=Line.substr(0,MaxLine);
  std::string::size_type posB=X.find_last_of(" ,");
  int spc(0);
  while (posB!=std::string::npos && 
	 static_cast<int>(X.length())>=MaxLine-spc)
    {
      pos+=posB+1;
      if (!isspace(X[posB]))
	posB++;
      const std::string Out=X.substr(0,posB);
      if (!isEmpty(Out))
        {
	  if (spc)
	    OX<<std::string(spc,' ');
	  OX<<X.substr(0,posB)<<std::endl;
	}
      spc=8;
      X=Line.substr(pos,MaxLine-spc);
      posB=X.find_last_of(" ,");
    }
  if (!isEmpty(X))
    {
      if (spc)
	OX<<std::string(spc,' ');
      OX<<X<<std::endl;
    }
  return;
}

std::vector<std::string>
StrParts(std::string Ln)
  /*!
    Splits the sting into parts that are space delminated.
    \param Ln :: line component to strip
    \returns vector of components
  */
{
  std::vector<std::string> Out;
  std::string Part;
  while(section(Ln,Part))
    Out.push_back(Part);
  return Out;
}

template<typename T>
int
convPartNum(const std::string& A,T& out)
  /*!
    Takes a character string and evaluates 
    the first [typename T] object. The string is then 
    erase upto the end of number.
    The diffierence between this and convert is that
    it allows trailing characters after the number. 
    \param out :: place for output
    \param A :: string to process
    \retval number of char read on success
    \retval 0 on failure
   */ 
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  const int xpt=cx.tellg();
  if (xpt<0)
    return 0;
  out=retval;
  return xpt; 
}

template<typename T>
int
convert(const std::string& A,T& out)
  /*!
    Convert a string into a value 
    \param A :: string to pass
    \param out :: value if found
    \returns 0 on failure 1 on success
  */
{
  if (A.empty()) return 0;
  std::istringstream cx;
  T retval;
  cx.str(A);
  cx.clear();
  cx>>retval;
  if (cx.fail())  
    return 0;
  const char clast=cx.get();
  if (!cx.fail() && !isspace(clast))
    return 0;
  out=retval;
  return 1;
}

template<typename T>
int
convert(const char* A,T& out)
  /*!
    Convert a string into a value 
    \param A :: string to pass
    \param out :: value if found
    \returns 0 on failure 1 on success
  */
{
  // No string, no conversion
  if (!A) return 0;
  std::string Cx=A;
  return convert(Cx,out);
}

template<template<typename T> class V,typename T> 
int
writeFile(const std::string& Fname,const T step, const V<T>& Y)
  /*!
    Write out the three vectors into a file of type dc 9
    \param step :: parameter to control x-step (starts from zero)
    \param Y :: Y column
    \param Fname :: Name of the file
    \returns 0 on success and -ve on failure
  */
{
  V<T> Ex;   // Empty vector
  V<T> X;    // Empty vector
  for(unsigned int i=0;i<Y.size();i++)
    X.push_back(i*step);

  return writeFile(Fname,X,Y,Ex);
}

template<template<typename T> class V,typename T> 
int
writeFile(const std::string& Fname,const V<T>& X,
	  const V<T>& Y)
  /*!
    Write out the three vectors into a file of type dc 9
    \param X :: X column
    \param Y :: Y column
    \param Fname :: Name of the file
    \returns 0 on success and -ve on failure
  */
{
  V<T> Ex;   // Empty vector/list
  return writeFile(Fname,X,Y,Ex);  // don't need to specific ??
}

template<template<typename T> class V,typename T> 
int
writeFile(const std::string& Fname,const V<T>& X,
	  const V<T>& Y,const V<T>& Err)
  /*!
    Write out the three container into a file of type dc 9
    \param X :: X column
    \param Y :: Y column
    \param Err :: Err column
    \param Fname :: Name of the file
    \returns 0 on success and -ve on failure
  */
{
  const int Npts(X.size()>Y.size() ? Y.size() : X.size());
  const int Epts(Npts>static_cast<int>(Err.size()) ? Err.size() : Npts);

  std::ofstream FX;

  FX.open(Fname.c_str());
  if (!FX.good())
    return -1;

  FX<<"# "<<Npts<<" "<<Epts<<std::endl;
  FX.precision(10);
  FX.setf(std::ios::scientific,std::ios::floatfield);
  typename V<T>::const_iterator xPt=X.begin();
  typename V<T>::const_iterator yPt=Y.begin();
  typename V<T>::const_iterator ePt=(Epts ? Err.begin() : Y.begin());
  
  // Double loop to include/exclude a short error stack
  int eCount=0;
  for(;eCount<Epts;eCount++)
    {
      FX<<(*xPt)<<" "<<(*yPt)<<" "<<(*ePt)<<std::endl;
      xPt++;
      yPt++;
      ePt++;
    }
  for(;eCount<Npts;eCount++)
    {
      FX<<(*xPt)<<" "<<(*yPt)<<" 0.0"<<std::endl;
      xPt++;
      yPt++;
    }
  FX.close();
  return 0;
}

float
getVAXnum(const float A) 
  /*!
    Converts a vax number into a standard unix number
    \param A :: float number as read from a VAX file
    \returns float A in IEEE little eindian format
  */
{
  union 
   {
     char a[4];
     float f;
     int ival;
   } Bd;

  int sign,expt,fmask;
  float frac;
  double onum;

  Bd.f=A;
  sign  = (Bd.ival & 0x8000) ? -1 : 1;
  expt = ((Bd.ival & 0x7f80) >> 7);   //reveresed ? 
  if (!expt) 
    return 0.0;

  fmask = ((Bd.ival & 0x7f) << 16) | ((Bd.ival & 0xffff0000) >> 16);
  expt-=128;
  fmask |=  0x800000;
  frac = (float) fmask  / 0x1000000;
  onum= frac * sign * 
         pow(2.0,expt);
  return (float) onum;
}

template<typename T> 
int
setValues(const std::string& Line,const std::vector<int>& Index,
	  std::vector<T>& Out)
  /*!  
    Call to read in various values in position x1,x2,x3 from the
    line. Note to avoid the dependency on crossSort this needs
    to be call IN ORDER 
    \param Line :: string to read
    \param Index :: Indexes to read
    \param Out :: OutValues [unchanged if not read]
    \retval 0 :: success
    \retval -ve on failure.
  */
{
  if (Index.empty())
    return 0;
  
  if(Out.size()!=Index.size())
    return -1;
//    throw ColErr::MisMatch<int>(Index.size(),Out.size(),
//				"StrFunc::setValues");

  std::string modLine=Line;
  std::vector<int> sIndex(Index);     // Copy for sorting
  std::vector<int> OPt(Index.size());
  for(unsigned int i=0;i<Index.size();i++)
    OPt[i]=i;
      
  
  //  mathFunc::crossSort(sIndex,OPt);
  
  typedef std::vector<int>::const_iterator iVecIter;
  std::vector<int>::const_iterator sc=sIndex.begin();
  std::vector<int>::const_iterator oc=OPt.begin();
  int cnt(0);
  T value;
  std::string dump;
  while(sc!=sIndex.end() && *sc<0)
    {
      sc++;
      oc++;
    }
  
  while(sc!=sIndex.end())
    {
      if (*sc==cnt)
        {
	  if (!section(modLine,value))
	    return -1-distance(static_cast<iVecIter>(sIndex.begin()),sc);  
	  // this loop handles repeat units
	  do
	    {
	      Out[*oc]=value;
	      sc++;
	      oc++;
	    } while (sc!=sIndex.end() && *sc==cnt); 
	}
      else
        {
	  if (!section(modLine,dump))
	    return -1-distance(static_cast<iVecIter>(sIndex.begin()),sc);  
	}
      cnt++;         // Add only to cnt [sc/oc in while loop]
    }
  // Success since loop only gets here if sc is exhaused.
  return 0;       
}



}  // NAMESPACE StrFunc

}  // NAMESPACE Mantid
