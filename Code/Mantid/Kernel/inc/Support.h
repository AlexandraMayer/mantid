#ifndef MANTID_STRFUNC_SUPPORT_H_
#define MANTID_STRFUNC_SUPPORT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace Mantid
{
/** Holds support functions for strings.

    @author S. Ansell
    @date February 2006
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace StrFunc
{
/// determine if a character group exists in a string
DLLExport int confirmStr(const std::string& S,const std::string& fullPhrase);
/// Get a word from a string
DLLExport int extractWord(std::string& Line,const std::string& Word,const int cnt = 4);

/// strip all spaces
DLLExport std::string removeSpace(const std::string& CLine);
/// strip pre/post spaces
DLLExport std::string fullBlock(const std::string& A);
/// strip trailling comments
DLLExport void stripComment(std::string& A);
/// Determines if a string is only spaces
DLLExport int isEmpty(const std::string& A);
/// Get a line and strip comments 
DLLExport std::string getLine(std::istream& fh,const int spc = 256);
/// get a part of a long line
DLLExport int getPartLine(std::istream& fh,std::string& Out,std::string& Excess,const int spc = 256);

template<typename T> int convPartNum(const std::string& A,T& out);

/// Convert a string into a number
template<typename T> int convert(const std::string& A,T& out);
/// Convert a char* into a number
template<typename T> int convert(const char* A,T& out);


template<typename T> 
int setValues(const std::string& Line,const std::vector<int>& Index,std::vector<T>& Out);

/// Convert and cut a string
template<typename T> int sectPartNum(std::string& A,T& out);

/// Convert and cut a string
template<typename T> int section(std::string& A,T& out);
/// Convert and cut a char* 
template<typename T> int section(char* cA,T& out);

/// Convert and cut a string for MCNPX
template<typename T> int sectionMCNPX(std::string& A,T& out);

/// Write file in standard MCNPX input form 
void writeMCNPX(const std::string& Line,std::ostream& OX);

/// Split tring into spc deliminated components
std::vector<std::string> StrParts(std::string Ln);

/// Write a set of containers to a file
template<template<typename T> class V,typename T> 
int writeFile(const std::string& Fname,const T step,const V<T>& Y);
template<template<typename T> class V,typename T> 
int writeFile(const std::string& Fname,const V<T>& X,const V<T>& Y);
template<template<typename T> class V,typename T> 
int writeFile(const std::string& Fname,const V<T>& X,const V<T>& Y,const V<T>& Err);


/// Convert a VAX number to x86 little eindien
float getVAXnum(const float A);

}   // NAMESPACE StrFunc
}   // NAMESPACE Mantid

#endif //MANTID_STRFUNC_SUPPORT_H_

