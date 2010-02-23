#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <functional>

#include "MantidGeometry/Math/RegexSupport.h"

namespace Mantid
{

namespace StrFunc
{

template<typename T>
int
StrComp(const std::string& Text,const boost::regex& Re,T& Aout,
	const int compNum) 
  /*!
    Find the match in regular expression and places number in Aout 
    \param Text :: string to search
    \param Re :: regular expression to use
    \param Aout :: Place to put Unit found
    \param compNum :: item to extract [0:N-1]
    \returns 0 on failure and 1 on success
  */
{
	boost::sregex_iterator m1(Text.begin(),Text.end(),Re);
	boost::sregex_iterator empty;
	// Failed search
	if (m1==empty || 
		static_cast<int>((*m1).size())<compNum)  
		return 0;
	int count=compNum;
	for(;count!=0;count--)
		m1++;
	return convert( (*m1)[0].str(),Aout);
}


template<typename T>
int
StrComp(const char* Text,const boost::regex& Re,
	T& Aout,const int compNum) 
  /*!
    Find the match in regular expression and places number in Aout 
    \param Text :: string to search
    \param Re :: regular expression to use
    \param Aout :: Place to put Unit found
    \param compNum :: item to extract [0:N-1]
    \returns 0 on failure and 1 on success
  */
{
	return StrComp(std::string(Text),Re,Aout,compNum);
}


int
StrLook(const char* Sx,const boost::regex& Re)
  /*!
    Find the match in regular expression and return 1 if good match 
    \param Sx :: string to match
    \param Re :: regular expression to use
    \returns 0 on failure and 1 on success
  */
{
  boost::cmatch ans;
  if (boost::regex_search(Sx,ans,Re,boost::match_default))
    return 1;
  return 0;
}

int
StrLook(const std::string& Text,const boost::regex& Re)
  /*!
    Find the match in regular expression and return 1 if good match 
    \param Text :: string to match
    \param Re :: regular expression to use
    \returns 0 on failure and 1 on success
  */
{
  boost::sregex_iterator m1(Text.begin(),Text.end(),Re);
  boost::sregex_iterator empty;
  // Failed search
  if (m1==empty)
    return 0;
  return 1;
}

std::vector<std::string> 
StrParts(std::string Sdx,const boost::regex& Re)
  /*! 
    Find the match, return the disected items.
    Note it is complementary to support.h StrParts(Sdx)
    \param Sdx :: Input string (note implicit copy since altered)
    \param Re :: Regular expression for separator component
    \returns vector of string components
  */
{   
  std::vector<std::string> Aout;
  boost::regex_split(std::back_inserter(Aout), Sdx, Re);   // Destroys string in process
  return Aout;
}   

template<typename T>
int
StrFullCut(std::string& Text,const boost::regex& Re,T& Aout,
       const int compNum)
  /*! 
    Find the match, return the disected items:
    Then remove the whole of the match
    The regexpression must have  one  ( ) around the area to extract
    \param Text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: Value to extract
    \param compNum :: Index of matches [0->N-1] (-1 :: whole match)
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval 1 :: success
   */
{
  boost::sregex_iterator m1(Text.begin(),Text.end(),Re);
  boost::sregex_iterator empty;
  if (m1==empty)
    return 0;
  
  if (compNum+1>=static_cast<int>(m1->size()))
    return 0;
  // StrFunc::Convert to required output form
  if (!StrFunc::convert((*m1)[compNum+1].str(),Aout))
    return 0;
  // Found object 
  unsigned int zero = 0; // Needed for boost 1.40 (can't just put 0 in next line)
  Text.erase(m1->position(zero),(*m1)[0].str().length());
  return 1;
}

template<typename T>
int
StrFullCut(std::string& Text,const boost::regex& Re,std::vector<T>& Aout)
  /*! 
    Find the match, return the disected items:
    Then remove the whole of the match
    The regexpression must have  one  ( ) around the area to extract
    \param Text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: Values to extract
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval 1 :: success
   */
{
  boost::sregex_iterator m1(Text.begin(),Text.end(),Re);
  boost::sregex_iterator empty;
  if (m1==empty)
    return 0;

  std::cerr<<"SFC :: "<<std::endl;
  Aout.clear();
  unsigned int zero = 0; // Needed for boost 1.40
  const int M0=m1->position(zero);
  int ML=M0;
  for(;m1!=empty;m1++)
    {
      for(unsigned int index=1;index<m1->size();index++)
        {
	  T tmp;
	  if (!StrFunc::convert((*m1)[index].str(),tmp))
	    return 0;
	  Aout.push_back(tmp);
	}
      ML=m1->position(zero)+(*m1)[0].str().length();
    }
  std::cerr<<"SFC :: "<<M0<<" "<<ML<<std::endl;
  // Found object 
  Text.erase(M0,ML);
  return 1;
}

template<>
int
StrFullCut(std::string& Text,const boost::regex& Re,
	   std::vector<std::string>& Aout)
  /*! 
    Find the match, return the disected items:
    Then remove the whole of the match
    The regexpression must have  one  ( ) around the area to extract
    This is specialised for string and thus does not need 
    a convert.
    \param Text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: Values to extract
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval 1 :: success
   */
{
  boost::sregex_iterator m1(Text.begin(),Text.end(),Re);
  boost::sregex_iterator empty;
  if (m1==empty)
    return 0;

  unsigned int zero = 0; // Needed for boost 1.40
  const int M0=m1->position(zero);
  int ML=M0;
  for(;m1!=empty;m1++)
    {
      ML=m1->position(zero)+(*m1)[0].str().length();
      for(unsigned int index=1;index<m1->size();index++)
	Aout.push_back((*m1)[index].str());
    }
  std::cerr<<"SFC :: "<<M0<<" "<<ML<<std::endl;
  // Found object 
  Text.erase(M0,ML);
  return 1;
}

int
StrRemove(std::string& Sdx,std::string& Extract,const boost::regex& Re)
/*! 
  Find the match, return the string - the bit 
  \param Sdx :: string to split, is returned with the string after
  the find (if successful).
  \param Extract :: Full piece extracted
  \param Re :: regular expression to use.
  \retval 0 :: failed to match the string or there were no parts to match.
  \retval 1 :: succes
*/
{
  boost::sregex_token_iterator empty;

  boost::cmatch ans;
  if (boost::regex_search(Sdx.c_str(),ans,Re,boost::match_default))
    {
      if (!ans[0].matched)       // no match
	return 0;
      std::string xout(ans[0].first,ans[0].second);
      Extract=std::string(ans[0].first,ans[0].second);
      Sdx= std::string(Sdx.c_str(),ans[0].first)+
	std::string(ans[0].second);
      return 1;
    }
  return 0;
}

template<typename T>
int
StrFullSplit(const std::string& text,
	     const boost::regex& Re,std::vector<T>& Aout)
  /*! 
    Find the match, return the disected items
    The rege xpression must have ( ) around the area to extract.
    The function appends the results onto Aout.
    \param text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: vector to add components to.
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval Number :: number of components added to Aout.
   */
{
  boost::sregex_iterator m1(text.begin(),text.end(),Re);
  boost::sregex_iterator empty;
  for(;m1!=empty;m1++)
    for(unsigned int index=1;index<m1->size();index++)
      {
	T tmp;
	if (!StrFunc::convert((*m1)[index].str(),tmp))
	  return static_cast<int>(Aout.size());
	Aout.push_back(tmp);
      }
  return static_cast<int>(Aout.size());
}

template<typename T>
int     
StrSingleSplit(const std::string& text,
	     const boost::regex& Re,std::vector<T>& Aout)
  /*! 
    Find the match, return the disected items
    The regexpression must have ( ) around the area to extract.
    The function appends the results onto Aout.
    \param text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: vector to add components to.
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval Number :: number of components added to Aout.
   */
{
  boost::sregex_iterator m1(text.begin(),text.end(),Re);
  boost::sregex_iterator empty;
  if (m1!=empty)
    for(unsigned int index=1;index<m1->size();index++)
      {
	T tmp;
	if (!StrFunc::convert((*m1)[index].str(),tmp))
	  return static_cast<int>(Aout.size());
	Aout.push_back(tmp);
      }

  return static_cast<int>(Aout.size());
}

template<>
int
StrSingleSplit(const std::string& text,
	     const boost::regex& Re,std::vector<std::string>& Aout)
  /*! 
    Find the match, return the disected items
    The regexpression must have ( ) around the area to extract.
    The function appends the results onto Aout.
    - Specialised to avoid convert for std::string
    \param text :: string to split, is returned with the string after
    the find (if successful).
    \param Re :: regular expression to use.
    \param Aout :: vector to add components to.
    \retval 0 :: failed to match the string or there were no parts to match.
    \retval Number :: number of components added to Aout.
   */
{
  boost::sregex_iterator m1(text.begin(),text.end(),Re);
  boost::sregex_iterator empty;
  if (m1!=empty)
    {
      for(unsigned int index=1;index<m1->size();index++)
	Aout.push_back((*m1)[index].str());
      return 1;
    }
  return 0;
}

DLLExport int
findPattern(std::istream& fh,const boost::regex& Re,std::string& Out)
  /*! 
    Finds the start of the tally 
    \param fh :: open file stream
    \param Re :: regular expression to match
    \param Out :: string to place match 
    \returns count of line that matched (or zero on failure)
  */
{
  char ss[512];   // max of 512 
  boost::cmatch ans;

  int cnt=1;
  fh.getline(ss,512,'\n');
  while(!fh.fail() && !boost::regex_search(ss,ans,Re,boost::match_default))
    {
      fh.getline(ss,512,'\n');
      cnt++;
    }
  if (fh.fail())
    return 0;
  Out = ss;
  return cnt;
}

template<typename T>
int 
findComp(std::istream& fh,const boost::regex& Re,T& Out)
  /*! 
    Finds the start of the tally 
    \param fh :: open file stream
    \param Re :: regular expression to match
    \param Out :: component in ( ) expression must be first.
    \returns count of line that matched (or zero on failure)
  */
{
  char ss[512];   // max of 512 
  boost::cmatch ans;

  int cnt(1);
  fh.getline(ss,512,'\n');
  while(!fh.fail() && !boost::regex_search(ss,ans,Re,boost::match_default))
    {
      cnt++;
      fh.getline(ss,512,'\n');
    }
  if (ans[0].matched)
    {
      std::string xout(ans[1].first,ans[1].second);
      if (StrFunc::convert(xout,Out))
	return cnt;
    }
  return 0;
}

template<>
DLLExport int 
findComp(std::istream& fh,const boost::regex& Re,std::string& Out)
  /*! 
    Finds the start of the tally 
    \param fh :: open file stream
    \param Re :: regular expression to match
    \param Out :: component in ( ) expression must be first.
    \returns count of line that matched (or zero on failure)
  */
{
  char ss[512];   // max of 512 
  boost::cmatch ans;

  int cnt(1);
  fh.getline(ss,512,'\n');
  while(!fh.fail() && !boost::regex_search(ss,ans,Re,boost::match_default))
    {
      cnt++;
      fh.getline(ss,512,'\n');
    }
  if (ans[0].matched)
    {	
      Out=std::string(ans[1].first,ans[1].second);
	return cnt;
    }
  return 0;
}

/// \cond TEMPLATE 

template DLLExport int StrFullCut(std::string&,const boost::regex&,
			std::string&,const int);
template DLLExport int StrFullCut(std::string&,const boost::regex&,int&,const int);
template DLLExport int StrFullCut(std::string&,const boost::regex&,double&,const int);


// --------------------------------------------------------
template DLLExport int StrFullSplit(const std::string&,const boost::regex&,
			  std::vector<int>&);
template DLLExport int StrFullSplit(const std::string&,const boost::regex&,
			  std::vector<double>&);
template DLLExport int StrFullSplit(const std::string&,const boost::regex&,
			  std::vector<std::string>&);
// --------------------------------------------------------
template DLLExport int StrSingleSplit(const std::string&,const boost::regex&,
			  std::vector<int>&);
template DLLExport int StrSingleSplit(const std::string&,const boost::regex&,
			  std::vector<double>&);
// --------------------------------------------------------
template DLLExport int StrComp(const char*,const boost::regex&,double&,const int);
template DLLExport int StrComp(const char*,const boost::regex&,int&,const int);
template DLLExport int StrComp(const std::string&,const boost::regex&,double&,const int);
template DLLExport int StrComp(const std::string&,const boost::regex&,int&,const int);
// ------------------------------------------------------------------
template DLLExport int findComp(std::istream&,const boost::regex&,int&);

/// \endcond TEMPLATE 
}  // NAMESPACE StrFunc

} // NAMESPACE MAntid

