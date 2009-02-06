#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/regex.hpp>

#include "MantidKernel/Logger.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Exception.h"

#include "MantidGeometry/RegexSupport.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/BaseVisit.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surface.h"
#include "MantidGeometry/Line.h"
#include "MantidGeometry/LineIntersectVisit.h"
#include "MantidGeometry/Object.h"
#include "MantidGeometry/Rules.h"
#include "MantidGeometry/GeometryHandler.h"
#include "MantidGeometry/CacheGeometryHandler.h"
namespace Mantid
{

  namespace Geometry
  {

    Kernel::Logger& Object::PLog(Kernel::Logger::get("Object"));

    /// @cond
    //const double OTolerance(1e-6);
    /// @endcond

    Object::Object() :
      ObjName(0),MatN(-1),Tmp(300),density(0.0),TopRule(0),
      AABBxMax(0),AABByMax(0),AABBzMax(0),AABBxMin(0),AABByMin(0),AABBzMin(0),boolBounded(false)
      /*!
      Defaut constuctor, set temperature to 300K and material to vacuum
      */
    {
		handle=boost::shared_ptr<GeometryHandler>(new CacheGeometryHandler(this));
	}

    Object::Object(const Object& A) :
      ObjName(A.ObjName),MatN(A.MatN),Tmp(A.Tmp),density(A.density),
      TopRule((A.TopRule) ? A.TopRule->clone() : 0),
      AABBxMax(A.AABBxMax),
      AABByMax(A.AABByMax),
      AABBzMax(A.AABBzMax),
      AABBxMin(A.AABBxMin),
      AABByMin(A.AABByMin),
      AABBzMin(A.AABBzMin),boolBounded(A.boolBounded),
      SurList(A.SurList)
      /*!
      Copy constructor
      \param A :: Object to copy
      */
    {
		handle=boost::shared_ptr<GeometryHandler>(new CacheGeometryHandler(this));
	}

    Object&
      Object::operator=(const Object& A)
      /*!
      Assignment operator
      \param A :: Object to copy
      \return *this
      */
    {
      if (this!=&A)
      {
        ObjName=A.ObjName;
        MatN=A.MatN;
        Tmp=A.Tmp;
        density=A.density;
        delete TopRule;
        TopRule=(A.TopRule) ? A.TopRule->clone() : 0;
        SurList=A.SurList;
        AABBxMax=A.AABBxMax;
        AABByMax=A.AABByMax;
        AABBzMax=A.AABBzMax;
        AABBxMin=A.AABBxMin;
        AABByMin=A.AABByMin;
        AABBzMin=A.AABBzMin;
        boolBounded=A.boolBounded;
        handle=A.handle;
      }
      return *this;
    }

    Object::~Object()
      /*!
      Delete operator : removes Object tree
      */
    {
      delete TopRule;
	  //for(int i=0;i<SurList.size();i++)
		 // delete SurList[i];
    }

    int
      Object::setObject(const int ON,const std::string& Ln)
      /*!
      Object line ==  cell
      \param ON :: Object name
      \param Ln :: Input string must be :  {rules}
      \retval 1 on success
      \retval 0 on failure
      */
    {
      // Split line
      std::string part;
      const boost::regex letters("[a-zA-Z]");  // Does the string now contain junk...
      if (StrFunc::StrLook(Ln,letters))
        return 0;

      if (procString(Ln))     // this currently does not fail:
      {
        SurList.clear();
        ObjName=ON;
        return 1;
      }

      // failure
      return 0;
    }

    void
      Object::convertComplement(const std::map<int,Object>& MList)
      /*!
      Returns just the cell string object
      \param MList :: List of indexable Hulls
      \return Cell String (from TopRule)
      \todo Break infinite recusion
      */
    {
      this->procString(this->cellStr(MList));
      return;
    }


    std::string
      Object::cellStr(const std::map<int,Object>& MList) const
      /*!
      Returns just the cell string object
      \param MList :: List of indexable Hulls
      \return Cell String (from TopRule)
      \todo Break infinite recusion
      */
    {
      std::string TopStr=this->topRule()->display();
      std::string::size_type pos=TopStr.find('#');
      std::ostringstream cx;
      while(pos!=std::string::npos)
      {
        pos++;
        cx<<TopStr.substr(0,pos);            // Everything including the #
        int cN(0);
        const int nLen=StrFunc::convPartNum(TopStr.substr(pos),cN);
        if (nLen>0)
        {
          cx<<"(";
          std::map<int,Object>::const_iterator vc=MList.find(cN);
          if (vc==MList.end())
            throw Kernel::Exception::NotFoundError("Not found in the list of indexable hulls (Object::cellStr)",cN);
          // Not the recusion :: This will cause no end of problems
          // if there is an infinite loop.
          cx<<vc->second.cellStr(MList);
          cx<<") ";
          pos+=nLen;
        }
        TopStr.erase(0,pos);
        pos=TopStr.find('#');
      }
      cx<<TopStr;
      return cx.str();
    }

    int
      Object::complementaryObject(const int Cnum,std::string& Ln)
      /*!
      Calcluate if there are any complementary components in
      the object. That is lines with #(....)
      \throws ColErr::ExBase :: Error with processing
      \param Ln :: Input string must:  ID Mat {Density}  {rules}
      \param Cnum :: Number for cell since we don't have one
      \retval 0 on no work to do
      \retval 1 :: A (maybe there are many) #(...) object found
      */
    {
      std::string::size_type posA=Ln.find("#(");
      // No work to do ?
      if (posA==std::string::npos)
        return 0;
      posA+=2;

      // First get the area to be removed
      int brackCnt;
      std::string::size_type posB;
      posB=Ln.find_first_of("()",posA);
      if (posB==std::string::npos)
        throw std::runtime_error("Object::complemenet :: "+Ln);

      brackCnt=(Ln[posB] == '(') ? 1 : 0;
      while (posB!=std::string::npos && brackCnt)
      {
        posB=Ln.find_first_of("()",posB);
        brackCnt+=(Ln[posB] == '(') ? 1 : -1;
        posB++;
      }

      std::string Part=Ln.substr(posA,posB-(posA+1));

      ObjName=Cnum;
      MatN=0;
      density=0;
      if (procString(Part))
      {
        SurList.clear();
        Ln.erase(posA-1,posB+1);  //Delete brackets ( Part ) .
        std::ostringstream CompCell;
        CompCell<<Cnum<<" ";
        Ln.insert(posA-1,CompCell.str());
        return 1;
      }

      throw std::runtime_error("Object::complemenet :: "+Part);
      return 0;
    }

    int
      Object::hasComplement() const
      /*!
      Determine if the object has a complementary object

      \retval 1 :: true
      \retval 0 :: false
      */
    {

      if (TopRule)
        return TopRule->isComplementary();
      return 0;
    }

    int
      Object::populate(const std::map<int,Surface*>& Smap)
      /*!
      Goes through the cell objects and adds the pointers
      to the SurfPoint keys (using their keyN)
      \param Smap :: Map of surface Keys and Surface Pointers
      \retval 1000+ keyNumber :: Error with keyNumber
      \retval 0 :: successfully populated all the whole Object.
      */
    {
      std::deque<Rule*> Rst;
      Rst.push_back(TopRule);
      Rule* TA, *TB;      //Tmp. for storage

      int Rcount(0);
      while(Rst.size())
      {
        Rule* T1=Rst.front();
        Rst.pop_front();
        if (T1)
        {
          // if an actual surface process :
          SurfPoint* KV=dynamic_cast<SurfPoint*>(T1);
          if (KV)
          {
            // Ensure that we have a it in the surface list:
            std::map<int,Surface*>::const_iterator mf=Smap.find(KV->getKeyN());
            if (mf!=Smap.end())
            {
              KV->setKey(mf->second);
              Rcount++;
            }
            else
            {
              PLog.error("Error finding key");
              throw Kernel::Exception::NotFoundError("Object::populate",KV->getKeyN());
            }
          }
          // Not a surface : Determine leaves etc and add to stack:
          else
          {
            TA=T1->leaf(0);
            TB=T1->leaf(1);
            if (TA)
              Rst.push_back(TA);
            if (TB)
              Rst.push_back(TB);
          }
        }
      }
#if debugObject
      std::cerr<<"Populated surfaces =="<<Rcount<<" in "<<ObjName<<std::endl;
#endif

      createSurfaceList();
      return 0;
    }

    int
      Object::procPair(std::string& Ln,std::map<int,Rule*>& Rlist,int& compUnit) const
      /*!
      This takes a string Ln, finds the first two
      Rxxx function, determines their join type
      make the rule,  adds to vector then removes two old rules from
      the vector, updates string
      \param Ln :: String to porcess
      \param Rlist :: Map of rules (added to)
      \param compUnit :: Last computed unit
      \retval 0 :: No rule to find
      \retval 1 :: A rule has been combined
      */
    {
      unsigned int Rstart;
      unsigned int Rend;
      int Ra,Rb;

      for(Rstart=0;Rstart<Ln.size() &&
        Ln[Rstart]!='R';Rstart++);

        int type=0;      //intersection

      //plus 1 to skip 'R'
      if (Rstart==Ln.size() || !StrFunc::convert(Ln.c_str()+Rstart+1,Ra) ||
        Rlist.find(Ra)==Rlist.end())
        return 0;

      for(Rend=Rstart+1;Rend<Ln.size() &&
        Ln[Rend]!='R';Rend++)
      {
        if (Ln[Rend]==':')
          type=1;        //make union
      }
      if (Rend==Ln.size() || !StrFunc::convert(Ln.c_str()+Rend+1,Rb) ||
        Rlist.find(Rb)==Rlist.end())
        return 0;

      // Get end of number (digital)
      for(Rend++;Rend<Ln.size() &&
        Ln[Rend]>='0' && Ln[Rend]<='9';Rend++);

        // Get rules
        Rule* RRA=Rlist[Ra];
      Rule* RRB=Rlist[Rb];
      Rule* Join=(type) ? static_cast<Rule*>(new Union(RRA,RRB)) :
        static_cast<Rule*>(new Intersection(RRA,RRB));
      Rlist[Ra]=Join;
      Rlist.erase(Rlist.find(Rb));

      // Remove space round pair
      int fb;
      for(fb=Rstart-1;fb>=0 && Ln[fb]==' ';fb--);
      Rstart=(fb<0) ? 0 : fb;
      for(fb=Rend;fb<static_cast<int>(Ln.size()) && Ln[fb]==' ';fb++);
      Rend=fb;

      std::stringstream cx;
      cx<<" R"<<Ra<<" ";
      Ln.replace(Rstart,Rend,cx.str());
      compUnit=Ra;
      return 1;
    }

    CompGrp*
      Object::procComp(Rule* RItem) const
      /*!
      Taks a Rule item and makes it a complementary group
      \param RItem to encapsulate
      */
    {
      if (!RItem)
        return new CompGrp();

      Rule* Pptr=RItem->getParent();
      CompGrp* CG=new CompGrp(Pptr,RItem);
      if (Pptr)
      {
        const int Ln=Pptr->findLeaf(RItem);
        Pptr->setLeaf(CG,Ln);
      }
      return CG;
    }

    int
      Object::isOnSide(const Geometry::V3D& Pt) const
      /*!
      - (a) Uses the Surface list to check those surface
      that the point is on.
      - (b) Creates a list of normals to the touching surfaces
      - (c) Checks if normals and "normal pair bisection vector"
      are contary.
      If any are found to be so the the point is
      on a surface.
      - (d) Return 1 / 0 depending on test (c)

      \todo This needs to be completed to deal with apex points
            In the case of a apex (e.g. top of a pyramid) you need
            to interate over all clusters of points on the Snorm
            ie. sum of 2, sum of 3 sum of 4. etc. to be certain
            to get a correct normal test.

      \param Pt :: Point to check
      \returns 1 if the point is on the surface
      */
    {
      std::list<Geometry::V3D> Snorms;     // Normals from the constact surface.

      std::vector<const Surface*>::const_iterator vc;
      for(vc=SurList.begin();vc!=SurList.end();vc++)
      {
        if ((*vc)->onSurface(Pt))
          {
            Snorms.push_back((*vc)->surfaceNormal(Pt));
            // can check direct normal here since one success
            // means that we can return 1 and finish
            if (!checkSurfaceValid(Pt,Snorms.back()))
              return 1;
          }
       }
      std::list<Geometry::V3D>::const_iterator xs,ys;
      Geometry::V3D NormPair;
      for(xs=Snorms.begin();xs!=Snorms.end();xs++)
        for(ys=xs,ys++;ys!=Snorms.end();ys++)
        {
          NormPair=(*ys)+(*xs);
          NormPair.normalize();
          if (!checkSurfaceValid(Pt,NormPair))
            return 1;
        }
        // Ok everthing failed return 0;
        return 0;
    }

    int
      Object::checkSurfaceValid(const Geometry::V3D& C,const Geometry::V3D& Nm) const
      /*!
      Determine if a point is valid by checking both
      directions of the normal away from the line
      A good point will have one valid and one invalid.
      \param C :: Point on a basic surface to check
      \param Nm :: Direction +/- to be checked
      \retval +1 ::  Point outlayer (ie not in object)
      \retval -1 :: Point included (e.g at convex intersection)
      \retval 0 :: success
      */
    {
      int status(0);
	  Geometry::V3D tmp=C+Nm*(Surface::getSurfaceTolerance()*5.0);
      status= (!isValid(tmp)) ? 1 : -1;
      tmp-= Nm*(Surface::getSurfaceTolerance()*10.0);
      status+= (!isValid(tmp)) ? 1 : -1;
      return status/2;
    }

    int
      Object::isValid(const Geometry::V3D& Pt) const
      /*!
      Determines is Pt is within the object
      or on the surface
      \param Pt :: Point to be tested
      \returns 1 if true and 0 if false
      */
    {
      if (!TopRule)
        return 0;
      return TopRule->isValid(Pt);
    }

    int
      Object::isValid(const std::map<int,int>& SMap) const
      /*!
      Determines is group of surface maps are valid
      \param SMap :: map of SurfaceNumber : status
      \returns 1 if true and 0 if false
      */
    {
      if (!TopRule)
        return 0;
      return TopRule->isValid(SMap);
    }

    int
      Object::createSurfaceList(const int outFlag)
      /*!
      Uses the topRule* to create a surface list
      by iterating throught the tree
      \param outFlag Sends output to standard error if true
      \return 1 (should be number of surfaces)
      */
    {
      SurList.clear();
      std::stack<const Rule*> TreeLine;
      TreeLine.push(TopRule);
      while(!TreeLine.empty())
      {
        const Rule* tmpA=TreeLine.top();
        TreeLine.pop();
        const Rule* tmpB=tmpA->leaf(0);
        const Rule* tmpC=tmpA->leaf(1);
        if (tmpB || tmpC)
        {
          if (tmpB)
            TreeLine.push(tmpB);
          if (tmpC)
            TreeLine.push(tmpC);
        }
        else
        {
          const SurfPoint* SurX=dynamic_cast<const SurfPoint*>(tmpA);
          if (SurX)
            SurList.push_back(SurX->getKey());
          //	  else if (dynamic_cast<const CompObj*>(tmpA))
          //	    {
          //	      std::cerr<<"Ojbect::is Complementary"<<std::endl;
          //	    }
          //	  else
          //	    std::cerr<<"Object::Error with surface List"<<std::endl;
        }
      }
      if (outFlag)
      {

        std::vector<const Surface*>::const_iterator vc;
        for(vc=SurList.begin();vc!=SurList.end();vc++)
        {
          std::cerr<<"Point == "<<reinterpret_cast<long int>(*vc)<<std::endl;
          std::cerr<<(*vc)->getName()<<std::endl;
        }
      }
      return 1;
    }

    std::vector<int>
      Object::getSurfaceIndex() const
      /*!
      Returns all of the numbers of surfaces
      \return Surface numbers
      */
    {
      std::vector<int> out;
      transform(SurList.begin(),SurList.end(),
        std::insert_iterator<std::vector<int> >(out,out.begin()),
        std::mem_fun(&Surface::getName));
      return out;
    }

    int
      Object::removeSurface(const int SurfN)
      /*!
      Removes a surface and then re-builds the
      cell. This could be done by just removing
      the surface from the object.
      \param SurfN :: Number for the surface
      \return number of surfaces removes
      */
    {
      if (!TopRule)
        return -1;
      const int cnt=Rule::removeItem(TopRule,SurfN);
      if (cnt)
        createSurfaceList();
      return cnt;
    }

    int
      Object::substituteSurf(const int SurfN,const int NsurfN,Surface* SPtr)
      /*!
      Removes a surface and then re-builds the
      cell.
      \param SurfN :: Number for the surface
      \param NsurfN :: New surface number
      \param SPtr :: Surface pointer for surface NsurfN
      \return number of surfaces substituted
      */
    {
      if (!TopRule)
        return 0;
      const int out=TopRule->substituteSurf(SurfN,NsurfN,SPtr);
      if (out)
        createSurfaceList();
      return out;
    }

    void
      Object::print() const
      /*!
      Prints almost everything
      */
    {
      std::deque<Rule*> Rst;
      std::vector<int> Cells;
      int Rcount(0);
      Rst.push_back(TopRule);
      Rule* TA, *TB;      //Temp. for storage

      while(Rst.size())
      {
        Rule* T1=Rst.front();
        Rst.pop_front();
        if (T1)
        {
          Rcount++;
          SurfPoint* KV=dynamic_cast<SurfPoint*>(T1);
          if (KV)
            Cells.push_back(KV->getKeyN());
          else
          {
            TA=T1->leaf(0);
            TB=T1->leaf(1);
            if (TA)
              Rst.push_back(TA);
            if (TB)
              Rst.push_back(TB);
          }
        }
      }

      std::cout<<"Name == "<<ObjName<<std::endl;
      std::cout<<"Material == "<<MatN<<std::endl;
      std::cout<<"Rules == "<<Rcount<<std::endl;
      std::vector<int>::const_iterator  mc;
      std::cout<<"Surface included == ";
      for(mc=Cells.begin();mc<Cells.end();mc++)
      {
        std::cout<<(*mc)<<" ";
      }
      std::cout<<std::endl;
      return;
    }


    void
      Object::makeComplement()
      /*!
      Takes the complement of a group
      */
    {
      Rule* NCG=procComp(TopRule);
      TopRule=NCG;
      return;
    }


    void
      Object::printTree() const
      /*!
      Displays the rule tree
      */
    {
      std::cout<<"Name == "<<ObjName<<std::endl;
      std::cout<<"Material == "<<MatN<<std::endl;
      std::cout<<TopRule->display()<<std::endl;
      return;
    }


    std::string
      Object::cellCompStr() const
      /*!
      Write the object to a string.
      This includes only rules.
      \return Object Line
      */
    {
      std::ostringstream cx;
      if (TopRule)
        cx<<TopRule->display();
      return cx.str();
    }

    std::string
      Object::str() const
      /*!
      Write the object to a string.
      This includes the Name , material and density but
      not post-fix operators
      \return Object Line
      */
    {
      std::ostringstream cx;
      if (TopRule)
      {
        cx<<ObjName<<" "<<MatN;
        if (MatN!=0)
          cx<<" "<<density;
        cx<<" "<<TopRule->display();
      }
      return cx.str();
    }

    void
      Object::write(std::ostream& OX) const
      /*!
      Write the object to a standard stream
      in standard MCNPX output format.
      \param OX :: Output stream (required for multiple std::endl)
      */
    {
      std::ostringstream cx;
      cx.precision(10);
      cx<<str();
      if (Tmp!=300.0)
        cx<<" "<<"tmp="<<Tmp*8.6173422e-11;
      StrFunc::writeMCNPX(cx.str(),OX);
      return;
    }

    int
      Object::procString(const std::string& Line)
      /*!
      Processes the cell string. This is an internal function
      to process a string with
      - String type has #( and ( )
      \param Line :: String value
      \returns 1 on success
      */
    {
      delete TopRule;
      TopRule=0;
      std::map<int,Rule*> RuleList;    //List for the rules
      int Ridx=0;                     //Current index (not necessary size of RuleList
      // SURFACE REPLACEMENT
      // Now replace all free planes/Surfaces with appropiate Rxxx
      SurfPoint* TmpR(0);       //Tempory Rule storage position
      CompObj* TmpO(0);         //Tempory Rule storage position

      std::string Ln=Line;
      // Remove all surfaces :
      std::ostringstream cx;
      for(int i=0;i<static_cast<int>(Ln.length());i++)
      {
        if (isdigit(Ln[i]) || Ln[i]=='-')
        {
          int SN;
          int nLen=StrFunc::convPartNum(Ln.substr(i),SN);
          if (!nLen)
            throw std::invalid_argument("Invalid surface string in Object::ProcString : " + Line);
          // Process #Number
          if (i!=0 && Ln[i-1]=='#')
          {
            TmpO=new CompObj();
            TmpO->setObjN(SN);
            RuleList[Ridx]=TmpO;
          }
          else       // Normal rule
          {
            TmpR=new SurfPoint();
            TmpR->setKeyN(SN);
            RuleList[Ridx]=TmpR;
          }
          cx<<" R"<<Ridx<<" ";
          Ridx++;
          i+=nLen;
        }
        cx<<Ln[i];
      }
      Ln=cx.str();
      // PROCESS BRACKETS

      int brack_exists=1;
      while (brack_exists)
      {
        std::string::size_type rbrack=Ln.find(')');
        std::string::size_type lbrack=Ln.rfind('(',rbrack);
        if (rbrack!=std::string::npos && lbrack!=std::string::npos)
        {
          std::string Lx=Ln.substr(lbrack+1,rbrack-lbrack-1);
          // Check to see if a #( unit
          int compUnit(0);
          while(procPair(Lx,RuleList,compUnit));
          Ln.replace(lbrack,1+rbrack-lbrack,Lx);
          // Search back and find if # ( exists.
          int hCnt;
          for(hCnt=lbrack-1;hCnt>=0 && isspace(Ln[hCnt]);hCnt--);
          if (hCnt>=0 && Ln[hCnt]=='#')
          {
            RuleList[compUnit]=procComp(RuleList[compUnit]);
            std::ostringstream px;
            Ln.erase(hCnt,lbrack-hCnt);
          }
        }
        else
          brack_exists=0;
      }
      // Do outside loop...
      int nullInt;
      while(procPair(Ln,RuleList,nullInt));

      if (RuleList.size()!=1)
      {
        std::cerr<<"Map size not equal to 1 == "<<RuleList.size()<<std::endl;
        std::cerr<<"Error Object::ProcString : "<<Ln<<std::endl;
        exit(1);
        return 0;
      }
      TopRule=(RuleList.begin())->second;
      return 1;
    }

    int
    Object::interceptSurface(Geometry::Track& UT) const
      /*!
      Given a track, fill the track with valid section
      \param UT :: Initial track
      \return Number of segments added
      */
    {
      int cnt = UT.count();    // Number of intersections original track
      // Loop over all the surfaces.

      LineIntersectVisit LI(UT.getInit(),UT.getUVec());
      std::vector<const Surface*>::const_iterator vc;
      for(vc=SurList.begin();vc!=SurList.end();vc++)
      {
        (*vc)->acceptVisitor(LI);
      }
      const std::vector<Geometry::V3D>& IPts(LI.getPoints());
      const std::vector<double>& dPts(LI.getDistance());

      for(unsigned int i=0;i<IPts.size();i++)
      {
        if (dPts[i]>0.0)  // only interested in forward going points
        {
          // Is the point and enterance/exit Point
          const int flag=calcValidType(IPts[i],UT.getUVec());
          UT.addPoint(ObjName,flag,IPts[i]);
        }
      }
      UT.buildLink();
      // Get number of track segments added
      cnt = UT.count() - cnt;
      return cnt;
    }

    int
      Object::calcValidType(const Geometry::V3D& Pt,
      const Geometry::V3D& uVec) const
      /*!
      Calculate if a point PT is a valid point on the track
      \param Pt :: Point to calculate from.
      \param uVec :: Unit vector of the track
      \retval 0 :: Not valid / double valid
      \retval 1 :: Entry point
      \retval -1 :: Exit Point
      */
    {
      const Geometry::V3D testA(Pt-uVec*Surface::getSurfaceTolerance()*25.0);
      const Geometry::V3D testB(Pt+uVec*Surface::getSurfaceTolerance()*25.0);
      const int flagA=isValid(testA);
      const int flagB=isValid(testB);
      if (!(flagA ^ flagB)) return 0;
      return (flagA) ? -1 : 1;
    }

    double
        Object::solidAngle(const Geometry::V3D& observer) const
        /*!
        Find soild angle of object wrt the observer. This interface routine calls either
        getTriangleSoldiAngle or getRayTraceSolidAngle. Choice made on number of triangles
        in the discete surface representation.
        @param observer :: point to measure solid angle from
        @return :: estimate of solid angle of object. Accuracy depends on object shape.
        */
    {
        if( this->NumberOfTriangles()>30000 )
            return rayTraceSolidAngle(observer);
        return triangleSolidAngle(observer);
    }
    double
      Object::rayTraceSolidAngle(const Geometry::V3D& observer) const
      /*!
      Given an observer position find the approximate solid angle of the object
      \param observer :: position of the observer (V3D)
      \return Solid angle in steradians (+/- 1% if accurate bounding box available)
      */
    {
      // Calculation of solid angle as numerical double integral over all
      // angles. This could be optimised further e.g. by
      // using a light weight version of the interceptSurface method - this does more work
      // than is necessary in this application.
      // Accuracy is of the order of 1% for objects with an accurate boundng box, though
      // less in the case of high aspect ratios.
      //
	  // resBB controls accuracy and cost - linear accuracy improvement with increasing res,
	  // but quadratic increase in run time. If no bounding box found, resNoBB used instead.
	  const int resNoBB=200,resBB=100,resPhiMin=10;
      int res=resNoBB,itheta,jphi,resPhi;
      double theta,phi,sum,dphi,dtheta;
      if( this->isValid(observer) && ! this->isOnSide(observer) )
         return 4*M_PI;  // internal point
      if( this->isOnSide(observer) )
         return 2*M_PI;  // this is wrong if on an edge
	  // Use BB if available, and if observer not within it
	  const double big(1e10);
	  double xmin,ymin,zmin,xmax,ymax,zmax,thetaMax=M_PI;
	  bool useBB=false,usePt=false;
	  Geometry::V3D ptInObject,axis;
	  Quat zToPt;

	  xmin=ymin=zmin=-big;
	  xmax=ymax=zmax=big;
	  getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	  // Is the bounding box a reasonable one?
	  if( xmax<big && ymax<big && zmax<big && xmin>-big && ymin>-big && zmin>-big )
      {
            // If observer not inside bounding box, then can make use of it
			if( !inBoundingBox(observer,xmax,ymax,zmax,xmin,ymin,zmin) )
			{
				useBB=usePt=true;
				thetaMax=bbAngularWidth(observer,xmax,ymax,zmax,xmin,ymin,zmin);
				ptInObject=Geometry::V3D(0.5*(xmin+xmax),0.5*(ymin+ymax),0.5*(zmin+zmax));
				res=resBB;
			}
      }
	  // Try and find a point in the object if useful bounding box not found
      if( !useBB )
		  usePt=getPointInObject(ptInObject)==1;
	  if( usePt )
	  {
		  // found point in object, now get rotation that maps z axis to this direction from observer
		  ptInObject-=observer;
		  double theta0=-180.0/M_PI*acos(ptInObject.Z()/ptInObject.norm());
		  axis=ptInObject.cross_prod(Geometry::V3D (0.0,0.0,1.0));
		  if(axis.nullVector()) axis=Geometry::V3D (1.0,0.0,0.0);
		  zToPt(theta0,axis);
	  }
      dtheta=thetaMax/res;
	  int count=0,countPhi;
	  sum=0.;
      for(itheta=1;itheta<=res;itheta++)
      {
         // itegrate theta from 0 to maximum from bounding box, or PI otherwise
         theta=thetaMax*(itheta-0.5)/res;
         resPhi=static_cast<int> (res*sin(theta));
         if(resPhi<resPhiMin) resPhi=resPhiMin;
         dphi=2*M_PI/resPhi;
		 countPhi=0;
         for(jphi=1;jphi<=resPhi;jphi++)
         {
		    // integrate phi from 0 to 2*PI
            phi=2.0*M_PI*(jphi-0.5)/resPhi;
			Geometry::V3D dir(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
			if( usePt ) zToPt.rotate(dir);
			if( !useBB || lineHitsBoundingBox(observer,dir,xmax,ymax,zmax,xmin,ymin,zmin) )
			{
               Track tr(observer, dir);
               if(this->interceptSurface(tr)>0)
               {
                  sum+=dtheta*dphi*sin(theta);
				  countPhi++;
		       }
			}
		 }
		 // this break (only used in no BB defined) may be wrong if object has hole in middle
		 if(!useBB && countPhi==0) break;
		 count+=countPhi;
	  }
	  if(!useBB && count<resPhiMin+1)
	  {
		  // case of no bound box defined and object has few if any points in sum
		  // redo integration on finer scale
	     thetaMax=thetaMax*(itheta-0.5)/res;
         dtheta=thetaMax/res;
	     sum=0;
         for(itheta=1;itheta<=res;itheta++)
         {
            theta=thetaMax*(itheta-0.5)/res;
            resPhi=static_cast<int> (res*sin(theta));
            if(resPhi<resPhiMin) resPhi=resPhiMin;
            dphi=2*M_PI/resPhi;
			countPhi=0;
            for(jphi=1;jphi<=resPhi;jphi++)
            {
               phi=2.0*M_PI*(jphi-0.5)/resPhi;
	    	   Geometry::V3D dir(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
			   if( usePt ) zToPt.rotate(dir);
               Track tr(observer, dir);
               if(this->interceptSurface(tr)>0)
               {
                  sum+=dtheta*dphi*sin(theta);
			      countPhi++;
		       }
		    }
		    if(countPhi==0) break;
	     }
	  }

      return sum;
    }

    /**
     * Find the solid angle of a triangle defined by vectors a,b,c from point "observer"
     *
     * formula (Oosterom) O=2atan([a,b,c]/(abc+(a.b)c+(a.c)b+(b.c)a))
     *
     * @param a :: first point of triangle
     * @param b :: second point of triangle
     * @param c :: third point of triangle
     * @param observer :: point from which solid angle is required
     * @return :: solid angle of triangle in Steradians.
     */
    double Object::getTriangleSolidAngle(const V3D& a, const V3D& b, const V3D& c, const V3D& observer) const
    {
        const V3D ao=a-observer;
        const V3D bo=b-observer;
        const V3D co=c-observer;
        const double modao=ao.norm();
        const double modbo=bo.norm();
        const double modco=co.norm();
        const double aobo=ao.scalar_prod(bo);
        const double aoco=ao.scalar_prod(co);
        const double boco=bo.scalar_prod(co);
        const double scalTripProd=ao.scalar_prod(bo.cross_prod(co));
        const double denom=modao*modbo*modco+modco*aobo+modbo*aoco+modao*boco;
        if(denom!=0.0)
            return 2.0*atan(scalTripProd/denom);
        else
            return 0.0; // not certain this is correct
    }


	/**
	 * Find solid angle of object from point "observer" using the
	 * OC triangluation of the object, if it exists
	 *
	 * @param observer :: Point from which solid angle is required
	 */
	double Object::triangleSolidAngle(const V3D& observer) const
    {
       this->initDraw();
       //
       // Because the triangles from OC are not consistently ordered wrt their outward normal
       // internal points give incorrect solid angle. Surface points are difficult to get right
       // with the triangle based method. Hence catch these two (unlikely) cases.
       if(!boolBounded)
       {
           const double big(1e8);
           AABBxMax=AABByMax=AABBzMax=big; AABBxMin=AABByMin=AABBzMin=-big;
           getBoundingBox(AABBxMax,AABByMax,AABBzMax,AABBxMin,AABByMin,AABBzMin);
       }
       if(inBoundingBox(observer,AABBxMax,AABByMax,AABBzMax,AABBxMin,AABByMin,AABBzMin))
       {
           if(isValid(observer))
           {
               if(isOnSide(observer))
                   return(2.0*M_PI);
               else
                   return(4.0*M_PI);
           }
       }
	   int nTri=this->NumberOfTriangles();
       //
       // If triangulation is not available fall back to ray tracing method
       //
       if(nTri==0)
       {
           double height,radius;
           int type;
           std::vector<Geometry::V3D> vectors;
           this->GetObjectGeom( type, vectors, radius, height);
           if(type==1)
               return CuboidSolidAngle(observer,vectors);
           //else if(type==2)
           //    return SphereSolidAngle(observer,vectors,radius);
           return rayTraceSolidAngle(observer);
       }
	   int nPoints=this->NumberOfPoints();
	   double* vertices=this->getTriangleVertices();
	   int *faces=this->getTriangleFaces();
       double sangle=0,sneg=0;
       for(int i=0;i<NumberOfTriangles();i++)
       {
           int p1=faces[i*3],p2=faces[i*3+1],p3=faces[i*3+2];
           V3D vp1=V3D(vertices[3*p1],vertices[3*p1+1],vertices[3*p1+2]);
           V3D vp2=V3D(vertices[3*p2],vertices[3*p2+1],vertices[3*p2+2]);
           V3D vp3=V3D(vertices[3*p3],vertices[3*p3+1],vertices[3*p3+2]);
           double sa=getTriangleSolidAngle(vp1,vp2,vp3,observer);
           if(sa>0)
              sangle+=sa;
           else
              sneg+=sa;
     //    std::cout << vp1 << vp2 << vp2;
       }
       return(0.5*(sangle-sneg));
    }
    /**
     * Get the solid angle of a cuboid defined by 4 points. Simple use of triangle based soild angle
     * calculation. Should work for parallel-piped as well.
     * @param observer :: point from which solid angle required
     * @param vectors :: vector of V3D - the values are the 4 points used to defined the cuboid
     * @return :: solid angle of cuboid - good accuracy
     */
	double Object::CuboidSolidAngle(const V3D observer, const std::vector<Geometry::V3D> vectors) const
	{
      // Build bounding points, then set up map of 12 bounding
      // triangles defining the 6 surfaces of the bounding box. Using a consistent
      // ordering of points the "away facing" triangles give -ve contributions to the
      // solid angle and hence are ignored.
      std::vector<V3D> pts;
      Geometry::V3D dx=vectors[1]-vectors[0];
      Geometry::V3D dz=vectors[3]-vectors[0];
      pts.push_back(vectors[2]); pts.push_back(vectors[2]+dx);
      pts.push_back(vectors[1]); pts.push_back(vectors[0]);
      pts.push_back(vectors[2]+dz); pts.push_back(vectors[2]+dz+dx);
      pts.push_back(vectors[1]+dz); pts.push_back(vectors[0]+dz);

      std::vector<std::vector<int>> triMap;
      for(int i=0;i<12;i++)
      {
          triMap.push_back(std::vector<int>());
          for(int j=0;j<3;j++)
              triMap[i].push_back(0);
      }
      triMap[0][0]=1; triMap[0][1]=4; triMap[0][2]=3;triMap[1][0]=3;triMap[1][1]=2;triMap[1][2]=1;
      triMap[2][0]=5; triMap[2][1]=6; triMap[2][2]=7;triMap[3][0]=7;triMap[3][1]=8;triMap[3][2]=5;
      triMap[4][0]=1; triMap[4][1]=2; triMap[4][2]=6;triMap[5][0]=6;triMap[5][1]=5;triMap[5][2]=1;
      triMap[6][0]=2; triMap[6][1]=3; triMap[6][2]=7;triMap[7][0]=7;triMap[7][1]=6;triMap[7][2]=2;
      triMap[8][0]=3; triMap[8][1]=4; triMap[8][2]=8;triMap[9][0]=8;triMap[9][1]=7;triMap[9][2]=3;
      triMap[10][0]=1; triMap[10][1]=5; triMap[10][2]=8;triMap[11][0]=8;triMap[11][1]=4;triMap[11][2]=1;
      double sangle=0.0;
      for(int i=0;i<triMap.size();i++)
      {
          double sa=getTriangleSolidAngle(pts[triMap[i][0]-1],pts[triMap[i][1]-1],pts[triMap[i][2]-1],observer);
          if(sa>0)
              sangle+=sa;
      }
      return(sangle);
   }
	/**
	 * Takes input axis aligned bounding box max and min points and calculates the bounding box for the
	 * object and returns them back in max and min points.
	 *
	 * @param xmax :: Maximum value for the bounding box in x direction
	 * @param ymax :: Maximum value for the bounding box in y direction
	 * @param zmax :: Maximum value for the bounding box in z direction
	 * @param xmin :: Minimum value for the bounding box in x direction
	 * @param ymin :: Minimum value for the bounding box in y direction
	 * @param zmin :: Minimum value for the bounding box in z direction
	 */
	void Object::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
	{
		if (!TopRule)
		{ //If no rule defined then return zero boundbing box
			xmax=ymax=zmax=xmin=ymin=zmin=0.0;
			return;
		}
		if(!boolBounded){
			AABBxMax=xmax;AABByMax=ymax;AABBzMax=zmax;
			AABBxMin=xmin;AABByMin=ymin;AABBzMin=zmin;
			TopRule->getBoundingBox(AABBxMax,AABByMax,AABBzMax,AABBxMin,AABByMin,AABBzMin);
			if(AABBxMax>=xmax||AABBxMin<=xmin||AABByMax>=ymax||AABByMin<=ymin||AABBzMax>=zmax||AABBzMin<=zmin)
				boolBounded=false;
			else
				boolBounded=true;
		}
		xmax=AABBxMax;	ymax=AABByMax;	zmax=AABBzMax;
		xmin=AABBxMin;	ymin=AABByMin;	zmin=AABBzMin;
	}

	/*!
	 * Takes input axis aligned bounding box max and min points and stores these as the
	 * bounding box for the object. Can be used when getBoundingBox fails and bounds are
	 * known.
	 *
	 * @param xMax :: Maximum value for the bounding box in x direction
	 * @param yMax :: Maximum value for the bounding box in y direction
	 * @param zMax :: Maximum value for the bounding box in z direction
	 * @param xMin :: Minimum value for the bounding box in x direction
	 * @param yMin :: Minimum value for the bounding box in y direction
	 * @param zMin :: Minimum value for the bounding box in z direction
	 */
	void Object::defineBoundingBox(const double &xMax, const double &yMax, const double &zMax, const double &xMin, const double &yMin, const double &zMin)
	{
		if(xMax<xMin || yMax<yMin || zMax<zMin)
            throw std::invalid_argument("Invalid range in Object::defineBoundingBox");
		AABBxMax=xMax;	AABByMax=yMax;	AABBzMax=zMax;
		AABBxMin=xMin;	AABByMin=yMin;	AABBzMin=zMin;
		boolBounded=true;
	}


	int
      Object::getPointInObject(Geometry::V3D& point) const
      /*!
      Try to find a point that lies within (or on) the object
      \param point :: on exit set to the point value, if found
      \return 1 if point found, 0 otherwise
      */
    {
      //
      // Simple method - check if origin in object, if not search directions along
	  // axes. If that fails, try centre of boundingBox, and paths about there
	  //
		Geometry::V3D testPt(0,0,0);
		if(searchForObject(testPt))
		{
			point=testPt;
			return 1;
		}
		//
		// Try centre of bounding box as initial guess, if one can be found
		// Note that if initial bounding box estimate greater than ~O(1e16) times
		// actual size, boundingBox may be wrong.
		//
		const double big(1e10);
		double xmin,ymin,zmin,xmax,ymax,zmax;
		xmin=ymin=zmin=-big;
		xmax=ymax=zmax=big;
		this->getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
		if( xmax<big && ymax<big && zmax<big && xmin>-big && ymin>-big && zmin>-big )
		{
		   testPt=Geometry::V3D(0.5*(xmax+xmin),0.5*(ymax+ymin),0.5*(zmax+zmin));
		   if(searchForObject(testPt)>0)
		   {
			   point=testPt;
			   return 1;
		   }
		}
		return 0;
	}

	int
      Object::searchForObject(Geometry::V3D& point) const
      /*!
      Try to find a point that lies within (or on) the object, given a seed point
      \param point :: on entry the seed point, on exit point in object, if found
      \return 1 if point found, 0 otherwise
      */
    {
      //
      // Method - check if point in object, if not search directions along
	  // principle axes using interceptSurface
	  //
		Geometry::V3D testPt;
		if(isValid(point))
			return 1;
		std::vector<Geometry::V3D> axes;
		axes.push_back(Geometry::V3D(1,0,0)); axes.push_back(Geometry::V3D(-1,0,0));
		axes.push_back(Geometry::V3D(0,1,0)); axes.push_back(Geometry::V3D(0,-1,0));
		axes.push_back(Geometry::V3D(0,0,1)); axes.push_back(Geometry::V3D(0,0,-1));
		std::vector<Geometry::V3D>::const_iterator  dir;
		for(dir=axes.begin();dir!=axes.end();dir++)
		{
			Geometry::Track tr(point,(*dir));
			if(this->interceptSurface(tr)>0)
			{
				point=tr.begin()->PtA;
				return 1;
			}
		}
		return 0;
	}

    int
      Object::lineHitsBoundingBox(const Geometry::V3D& orig, const Geometry::V3D& dir,
	                              const double& xmax, const double& ymax, const double& zmax,
	                              const double& xmin, const double& ymin, const double& zmin ) const
      /*!
      Fast test to determine if Line hits BoundingBox.
      \param orig :: Origin of line to test
	  \param dir  :: direction of line
	  \param xmax :: Maximum value for the bounding box in x direction
	  \param ymax :: Maximum value for the bounding box in y direction
	  \param zmax :: Maximum value for the bounding box in z direction
	  \param xmin :: Minimum value for the bounding box in x direction
	  \param ymin :: Minimum value for the bounding box in y direction
	  \param zmin :: Minimum value for the bounding box in z direction
      \return 1 if line hits, 0 otherwise
      */
    {
      //
      // Method - Loop through planes looking for ones that are visible and check intercept
	  // Assume that orig is outside of BoundingBox.
	  //
		double lambda;
		const double tol=Surface::getSurfaceTolerance();
		if(orig.X()>xmax)
		{
			if(dir.X()<-tol)
			{
	           lambda=(xmax-orig.X())/dir.X();
			   if( ymin < orig.Y()+lambda*dir.Y() && ymax > orig.Y()+lambda*dir.Y() )
                  if( zmin < orig.Z()+lambda*dir.Z() && zmax > orig.Z()+lambda*dir.Z() )
					  return 1;
			}
		}
		if(orig.X()<xmin)
		{
			if(dir.X()>tol)
			{
	           lambda=(xmin-orig.X())/dir.X();
			   if( ymin < orig.Y()+lambda*dir.Y() && ymax > orig.Y()+lambda*dir.Y() )
                  if( zmin < orig.Z()+lambda*dir.Z() && zmax > orig.Z()+lambda*dir.Z() )
					  return 1;
			}
		}
		if(orig.Y()>ymax)
		{
			if(dir.Y()<-tol)
			{
	           lambda=(ymax-orig.Y())/dir.Y();
			   if( xmin < orig.X()+lambda*dir.X() && xmax > orig.X()+lambda*dir.X() )
                  if( zmin < orig.Z()+lambda*dir.Z() && zmax > orig.Z()+lambda*dir.Z() )
					  return 1;
			}
		}
		if(orig.Y()<ymin)
		{
			if(dir.Y()>tol)
			{
	           lambda=(ymin-orig.Y())/dir.Y();
			   if( xmin < orig.X()+lambda*dir.X() && xmax > orig.X()+lambda*dir.X() )
                  if( zmin < orig.Z()+lambda*dir.Z() && zmax > orig.Z()+lambda*dir.Z() )
					  return 1;
			}
		}
		if(orig.Z()>zmax)
		{
			if(dir.Z()<-tol)
			{
	           lambda=(zmax-orig.Z())/dir.Z();
			   if( ymin < orig.Y()+lambda*dir.Y() && ymax > orig.Y()+lambda*dir.Y() )
                  if( xmin < orig.X()+lambda*dir.X() && xmax > orig.X()+lambda*dir.X() )
					  return 1;
			}
		}
		if(orig.Z()<zmin)
		{
			if(dir.Z()>tol)
			{
	           lambda=(zmin-orig.Z())/dir.Z();
			   if( ymin < orig.Y()+lambda*dir.Y() && ymax > orig.Y()+lambda*dir.Y() )
                  if( xmin < orig.X()+lambda*dir.X() && xmax > orig.X()+lambda*dir.X() )
					  return 1;
			}
		}
		return 0;

    }

    int
      Object::inBoundingBox(const Geometry::V3D& point,
	                        const double& xmax, const double& ymax, const double& zmax,
	                        const double& xmin, const double& ymin, const double& zmin ) const
      /*!
      Test point in BoundingBox
      \param point :: Point to test
	  \param xmax :: Maximum value for the bounding box in x direction
	  \param ymax :: Maximum value for the bounding box in y direction
	  \param zmax :: Maximum value for the bounding box in z direction
	  \param xmin :: Minimum value for the bounding box in x direction
	  \param ymin :: Minimum value for the bounding box in y direction
	  \param zmin :: Minimum value for the bounding box in z direction
      \return 1 if in, 0 otherwise
      */
    {
      //
      const double tol=Surface::getSurfaceTolerance();
      if(point.X()<=xmax+tol && point.X()>=xmin-tol && point.Y()<=ymax+tol && point.Y()>=ymin-tol
         && point.Z()<=zmax+tol && point.Z()>=zmin-tol )
		   return 1;
	   return 0;
	}

    double
		Object::bbAngularWidth(const Geometry::V3D& observer,
	                         const double& xmax, const double& ymax, const double& zmax,
	                         const double& xmin, const double& ymin, const double& zmin ) const
      /*!
      Find maximum angular half width of the bounding box from the observer, that is
	  the greatest angle between the centre point and any corner point
      \param observer :: Viewing point
	  \param xmax :: Maximum value for the bounding box in x direction
	  \param ymax :: Maximum value for the bounding box in y direction
	  \param zmax :: Maximum value for the bounding box in z direction
	  \param xmin :: Minimum value for the bounding box in x direction
	  \param ymin :: Minimum value for the bounding box in y direction
	  \param zmin :: Minimum value for the bounding box in z direction
      \return 1 if in, 0 otherwise
      */
    {
		double theta,thetaMax=-1;
		Geometry::V3D centre(0.5*(xmax+xmin),0.5*(ymax+ymin),0.5*(zmax+zmin));
		centre-=observer;
		std::vector<Geometry::V3D> pts;
		pts.push_back(Geometry::V3D(xmin,ymin,zmin)-observer);
		pts.push_back(Geometry::V3D(xmin,ymin,zmax)-observer);
		pts.push_back(Geometry::V3D(xmin,ymax,zmin)-observer);
		pts.push_back(Geometry::V3D(xmin,ymax,zmax)-observer);
		pts.push_back(Geometry::V3D(xmax,ymin,zmin)-observer);
		pts.push_back(Geometry::V3D(xmax,ymin,zmax)-observer);
		pts.push_back(Geometry::V3D(xmin,ymax,zmin)-observer);
		pts.push_back(Geometry::V3D(xmin,ymax,zmax)-observer);
		std::vector<Geometry::V3D>::const_iterator ip;
		for(ip=pts.begin();ip!=pts.end();ip++)
		{
			theta=acos((ip)->scalar_prod(centre)/((ip)->norm()*centre.norm()));
			if(theta>thetaMax) thetaMax=theta;
		}
		return thetaMax;
	}


	/**
	* Set the geometry handler for Object
	* @param[in] h is pointer to the geometry handler. don't delete this pointer in the calling function.
	*/
	void Object::setGeometryHandler(boost::shared_ptr<GeometryHandler> h)
	{
		if(h==NULL)return;
		handle=h;
	}

	/**
	* Draws the Object using geometry handler, If the handler is not set then this function does nothing.
	*/
	void Object::draw() const
	{
		if(handle==NULL)return;
		//Render the Object
		handle->Render();
	}

	/**
	* Initializes/prepares the object to be rendered, this will generate geometry for object,
	* If the handler is not set then this function does nothing.
	*/
	void Object::initDraw() const
	{
		if(handle==NULL)return;
		//Render the Object
		handle->Initialize();
	}
	//Initialize Draw Object

	/**
	* get number of triangles
	*/
	int Object::NumberOfTriangles() const
	{
		if(handle==NULL)return 0;
		return handle->NumberOfTriangles();
	}
	/**
	* get number of points
	*/
	int Object::NumberOfPoints() const
	{
		if(handle==NULL)return 0;
		return handle->NumberOfPoints();
	}
	/**
	* get vertices
	*/
	double* Object::getTriangleVertices() const
	{
		if(handle==NULL)return NULL;
		return handle->getTriangleVertices();
	}
	/**
	* get faces
	*/
	int* Object::getTriangleFaces() const
	{
		if(handle==NULL)return NULL;
		return handle->getTriangleFaces();
	}
    /**
    * get info on standard shapes
    */
    void Object::GetObjectGeom(int& type, std::vector<Geometry::V3D>& vectors, double& myradius, double myheight) const
    {
       type=0;
       if(handle==NULL)return;
       handle->GetObjectGeom( type, vectors, myradius, myheight);
    }

  }  // NAMESPACE Geometry

}  // NAMESPACE Mantid
