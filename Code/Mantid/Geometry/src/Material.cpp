#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <boost/regex.hpp>

#include "Logger.h"
#include "AuxException.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "Support.h"
#include "RefCon.h"
#include "Material.h"


namespace Mantid
{

namespace Geometry
{
Kernel::Logger& Material::PLog(Kernel::Logger::get("Material"));

Material::Material() : 
  density(0),scoh(0.0),
  sinc(0.0),sabs(0.0)
  /*!
    Constructor
  */
{}

Material::Material(const std::string& N,const double D,const double S,
		   const double I,const double A) : 
  Name(N),density(D),scoh(S),
  sinc(I),sabs(A)
  /*!
    Constructor for values
    \param Name :: Material name
    \param D :: density [atom/angtrom^3]
    \param S :: scattering cross section [barns]
    \param I :: incoherrent scattering cross section [barns]
    \param A :: absorption cross section [barns]
  */
{}

Material::Material(const double D,const double S,
		   const double I,const double A) : 
  density(D),scoh(S),
  sinc(I),sabs(A)
  /*!
    Constructor for values
    \param D :: density [atom/angtrom^3]
    \param S :: scattering cross section [barns]
    \param I :: incoherrent scattering cross section [barns]
    \param A :: absorption cross section [barns]
  */
{}

Material::Material(const Material& A) : 
  Name(A.Name),density(A.density),scoh(A.scoh),
  sinc(A.sinc),sabs(A.sabs)
  /*!
    Copy constructor
    \param A :: Material to copy
  */
{}

Material&
Material::operator=(const Material& A) 
  /*!
    Assignment operator 
    \param A :: Material to copy
    return *this
  */
{
  if (this!=&A)
    {
      Name=A.Name;
      density=A.density;
      scoh=A.scoh;
      sinc=A.sinc;
      sabs=A.sabs;
    }
  return *this;
}

Material*
Material::clone() const
  /*!
    Clone method
    \return new (this)
   */
{
  return new Material(*this);
}


Material::~Material() 
  /*!
    Destructor
   */
{}

void 
Material::setDensity(const double D) 
  /*!
    Sets the density
    \param D :: Density [Atom/Angstrom^3]
  */
{
  density=D;
  return;
}

void
Material::setScat(const double S,const double I,const double A)
  /*!
    Set the scattering factors
    \param S :: Coherrent-Scattering cross section
    \param S :: Inc-Scattering cross section
    \param A :: Absorption cross section
  */
{
  scoh=S;
  sinc=I;
  sabs=A;
  return;
}
  
double
Material::getAtten(const double Wave) const
  /*!
    Given Wavelength get the attenuation 
    coefficient.
    \param Wave :: Wavelength [Angstrom]
    \return Attenuation (including density)
  */
{
  return density*(scoh+Wave*sabs/1.798);
}

double
Material::calcAtten(const double Wave,const double Length) const
  /*!
    Calculate the attenuation factor

    coefficient.
    \param Wave :: Wavelength [Angstrom]
    \param Length :: Absorption length
    \return Attenuation (including density)
  */
{
  return exp(-Length*density*(scoh+sinc+Wave*sabs/1.798));
}

double
Material::getAttenAbs(const double Wave) const
  /*!
    Given Wavelength get the attenuation 
    coefficient.
    \param Wave :: wavelength
    \return Attenution coefficient
  */
{
  return density*Wave*sabs/1.798;
}

double
Material::getScatFrac(const double Wave) const
  /*!
    Get the fraction of scattering / total
    \param Wave :: Wavelength [Angstrom]
    \return Scattering fraction
  */
{
  return (density>0) ? (scoh+sinc)/(scoh+sinc+Wave*sabs/1.798) : 1.0;
}

void
Material::procXML(XML::XMLcollect& XOut) const
  /*!
    Write out the XML form
    \param XOut :: Output collection for the XMLschema
   */
{
  XOut.addComp("name",Name);
  XOut.addComp("density",density);
  XOut.addComp("scoh",scoh);
  XOut.addComp("sinc",sinc);
  XOut.addComp("sabs",sabs);
  return;
}

int
Material::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		 const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Material. 

    \param SK :: IndexIterator scheme
    \param singleFlag :: Single pass identifer
    \retval 0 :: success
   */
{
  int errCnt(0);
  int levelExit(SK.getLevel());
  do
    {
      if (*SK)
        {
	  const std::string& KVal= SK->getKey();
	  const XML::XMLread* RPtr=dynamic_cast<const XML::XMLread*>(*SK);
	  
	  if (RPtr)
	    {
	      if (KVal=="density" && RPtr)
	        {
		  if (!StrFunc::convert(RPtr->getFront(),density))
		    errCnt++;
		}
	      else if (KVal=="scoh" && RPtr)
	        {
		  if (!StrFunc::convert(RPtr->getFront(),scoh))
		    errCnt++;
		}
	      else if (KVal=="sinc" && RPtr)
	        {
		  if (!StrFunc::convert(RPtr->getFront(),sinc))
		    errCnt++;
		}
	      else if (KVal=="sabs" && RPtr)
	        {
		  if (!StrFunc::convert(RPtr->getFront(),sabs))
		    errCnt++;
		}
	      else
	        {
		  errCnt++;
		  PLog.warning("importXML :: Key failed "+KVal);
		}
	    }
	}
      if (!singleFlag) SK++;
    } while (!singleFlag && SK.getLevel()>=levelExit);
  
  return errCnt;
}


} // NAMESPACE MonteCarlo


}  // NAMESPACE Mantid
