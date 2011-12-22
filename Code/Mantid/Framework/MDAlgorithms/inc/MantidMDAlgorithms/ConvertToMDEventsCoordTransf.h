#ifndef  H_CONVERT_TO_MDEVENTS_COORD_TRANSF
#define  H_CONVERT_TO_MDEVENTS_COORD_TRANSF
//
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
//
namespace Mantid
{
namespace MDAlgorithms
{
/** Set of internal classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into from 1 to 4 output dimensions as function of input parameters
   *
   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/** The template below describes general ingerface to coordinate transformation:
*
*   Usual transformation constis of 3 steps
* 1) set-up, calculation and copying generic multidimensional variables which are not depenent on data
* 2) set-up, calculation and copying the multidimensional variables which dependent on detectors id only 
* 3) calculation of the multidimensional variables which depend on the data along x-axis of the workspace
*    and possibly on detectors parameters. 
* 
*  Generic template defines interface to 3 functions which perform these three steps. 
*/
template<Q_state Q,AnalMode MODE,CnvrtUnits CONV>
struct COORD_TRANSFORMER
{
      COORD_TRANSFORMER(ConvertToMDEvents *){}
    /**Template defines common interface to common part of the algorithm, where all variables
     * needed within the loop calculated before the loop starts. 
     *
     * In addition it caluclates the property-dependant coordinates 
     *
     * @param n_ws_variabes -- subalgorithm specific number of variables, calculated from the workspace data
     *
     * @return Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
     *
     * has to be specialized
    */
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes){
        UNUSED_ARG(Coord); UNUSED_ARG(n_ws_variabes);throw(Kernel::Exception::NotImplementedError(""));
        return false;}

   
    /** template generalizes the code to calculate Y-variables within the detector's loop of processQND workspace
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return Coord  -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.    
     * @return true   -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     *  some default implementations possible (e.g mode Q3D,ragged  Any_Mode( Direct, indirect,elastic), 
     */
    inline bool calcYDepCoordinatese(std::vector<coord_t> &Coord,uint64_t i){
        UNUSED_ARG(Coord); UNUSED_ARG(i);  return true;}

    /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @return Coord --Subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector   
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized
     */
    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(X); UNUSED_ARG(i); UNUSED_ARG(j); UNUSED_ARG(Coord);throw(Kernel::Exception::NotImplementedError(""));
        return false;}
  
private:
 
  
}; // end COORD_TRANSFORMER structure:



//----------------------------------------------------------------------------------------------------------------------
// SPECIALIZATIONS:
//----------------------------------------------------------------------------------------------------------------------
// ---->    NoQ
// NoQ,ANY_Mode -- no units conversion. This templates just copies the data into MD events and not doing any momentum transformations
//
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<NoQ,MODE,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)    
    {
       // get optional Y axis which can be used in NoQ-kind of algorithms
       pYAxis = dynamic_cast<API::NumericAxis *>(pHost->inWS2D->getAxis(1));
       if(pYAxis){  // two inital properties came from workspace. All are independant; All other dimensions are obtained from properties
           if(!pHost->fillAddProperties(Coord,nd,2))return false;
       }else{        // only one workspace property availible;
           if(!pHost->fillAddProperties(Coord,nd,1))return false;
       }
        // set up units conversion defined by the host algorithm.  
       CONV_UNITS_FROM.setUpConversion(this->pHost); 
       return true;
    }

    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
        CONV_UNITS_FROM.updateConversion(i);
        if(pYAxis){   
            Coord[1] = pYAxis->operator()(i);
            if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
        }
        return true;
    }

    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)
    {
       UNUSED_ARG(i);
       coord_t X_ev = CONV_UNITS_FROM.getXConverted(X,j);

       if(X_ev<pHost->dim_min[0]||X_ev>=pHost->dim_max[0])return false;
          
       Coord[0]=X_ev;
       return true;
    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){} 
private:
   // the variables used for exchange data between different specific parts of the generic ND algorithm:
    // pointer to Y axis of MD workspace
     API::NumericAxis *pYAxis;
     // pointer to MD workspace convertor
     ConvertToMDEvents *pHost;
     // class which would convert units
     UNITS_CONVERSION<CONV> CONV_UNITS_FROM;
};
//
////----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
// the module of the wavevector for scattered neutrons
template<AnalMode MODE>
inline double k_trans(double Ei, double E_tr){
    UNUSED_ARG(Ei);UNUSED_ARG(E_tr);
    throw(Kernel::Exception::NotImplementedError("Generic K_tr should not be implemented"));
}
// Direct Inelastic analysis
template<>
inline double k_trans<Direct>(double Ei, double E_tr){
    return sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}
// Indirect Inelastic analysis
template<>
inline double k_trans<Indir>(double Ei, double E_tr){
    return sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}

//  ----->  modQ
// modQ,Inelastic 
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<modQ,MODE,CONV>
{ 
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // 2 coordinates (|Q|, DeltaE) came from workspace, are interconnnected all additional defined by  properties:
        if(!pHost->fillAddProperties(Coord,nd,2))return false;

        // energy 
         Ei  =  ConvertToMDEvents::getEi(pHost);
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // get transformation matrix (needed for CrystalAsPoder mode)
         rotMat = pHost->getTransfMatrix();
         // 
         CONV_UNITS_FROM.setUpConversion(this->pHost); 

        // get pointer to the positions of the detectors
          pDet = &(ConvertToMDEvents::getPrepDetectors(pHost).det_dir[0]);
        //
         return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
        UNUSED_ARG(Coord); 
        CONV_UNITS_FROM.updateConversion(i);
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
    }
    //
    inline bool calcMatrixCoord(const MantidVec& X,uint64_t i,size_t j,std::vector<coord_t> &Coord)
    {
        UNUSED_ARG(i);
        // convert X-data into energy transfer (if necessary)
        coord_t E_tr = CONV_UNITS_FROM.getXConverted(X,j);
        Coord[1]    = E_tr;
        if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;

        // get module of the wavevector for scattered neutrons
        double k_tr = k_trans<MODE>(Ei,E_tr);
   
        double  qx  =  -ex*k_tr;                
        double  qy  =  -ey*k_tr;       
        double  qz  = ki - ez*k_tr;
        // transformation matrix has to be here for "Crystal AS Powder conversion mode, further specialization possible if "powder" switch provided"
        double Qx  = (rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);
        double Qy  = (rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz); 
        double Qz  = (rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        Coord[0]   = sqrt(Qsq);
        if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
        return true;

    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){}
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    //
    Kernel::V3D *pDet;
    // Calling Mantid algorithm
    ConvertToMDEvents *pHost;
    // class that performs untis conversion;
    UNITS_CONVERSION<CONV> CONV_UNITS_FROM;
};
// modQ,Elastic 
template<CnvrtUnits CONV> 
struct COORD_TRANSFORMER<modQ,Elastic,CONV>
{ 
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
      
        // 1 coordinate (|Q|) came from workspace, all additional defined by  properties:
        if(!pHost->fillAddProperties(Coord,nd,1))return false;
          // get transformation matrix (needed for CrystalAsPoder mode)
          rotMat = pHost->getTransfMatrix();
          // 
          CONV_UNITS_FROM.setUpConversion(this->pHost); 
         // get pointer to the positions of the detectors
          pDet = ConvertToMDEvents::getPrepDetectors(pHost).pDetDir();
          //
        return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
        UNUSED_ARG(Coord); 
        CONV_UNITS_FROM.updateConversion(i);
        ex = (pDet+i)->X();
        ey = (pDet+i)->Y();
        ez = (pDet+i)->Z();
        return true;
    }
    //
    inline bool calcMatrixCoord(const MantidVec& X,uint64_t i,size_t j,std::vector<coord_t> &Coord)
    {
        UNUSED_ARG(i);
        // convert X-data into momentum transfer (if necessary)
        coord_t k0 = CONV_UNITS_FROM.getXConverted(X,j);
   
        double  qx  =  -ex*k0;                
        double  qy  =  -ey*k0;       
        double  qz  = (1 - ez)*k0;
        // transformation matrix has to be here for "Crystal AS Powder mode, further specialization possible if "
        double Qx  = (rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);
        double Qy  = (rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz); 
        double Qz  = (rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);

        double Qsq = Qx*Qx+Qy*Qy+Qz*Qz;
        Coord[0]   = sqrt(Qsq);
        if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
        return true;

    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){}
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    //  directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    //
    Kernel::V3D *pDet;
    // Calling Mantid algorithm
    ConvertToMDEvents *pHost;
    // class that performs untis conversion;
    UNITS_CONVERSION<CONV> CONV_UNITS_FROM;
};


// Direct/Indirect tramsformatiom, this template describes 3D Q analysis mode. 
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<Q3D,MODE,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // four inital properties came from workspace and all are interconnnected all additional defined by  properties:
        if(!pHost->fillAddProperties(Coord,nd,4))return false;
        // energy 
         Ei  =  ConvertToMDEvents::getEi(pHost);
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // 
         rotMat = pHost->getTransfMatrix();
         CONV_UNITS_FROM.setUpConversion(this->pHost); 

        // get pointer to the positions of the detectors
          pDet = &(ConvertToMDEvents::getPrepDetectors(pHost).det_dir[0]);
        //
        return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
           UNUSED_ARG(Coord); 
           CONV_UNITS_FROM.updateConversion(i);
           ex = (pDet+i)->X();
           ey = (pDet+i)->Y();
           ez = (pDet+i)->Z();
           return true;
    }

    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
         UNUSED_ARG(i);
         // convert X-data into energy transfer (if necessary)
          coord_t E_tr = CONV_UNITS_FROM.getXConverted(X,j);
          Coord[3]    = E_tr;
          if(Coord[3]<pHost->dim_min[3]||Coord[3]>=pHost->dim_max[3])return false;

         // get module of the wavevector for scattered neutrons
         double k_tr = k_trans<MODE>(Ei,E_tr);
   
         double  qx  =  -ex*k_tr;                
         double  qy  =  -ey*k_tr;
         double  qz  = ki - ez*k_tr;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(Coord[2]<pHost->dim_min[2]||Coord[2]>=pHost->dim_max[2])return false;

         return true;
    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){}
private:
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to orthogonal crystall coordinate system. 
    std::vector<double> rotMat;
    // pointer to the detectors directions
    Kernel::V3D *pDet;
    // Calling Mantid algorithm
    ConvertToMDEvents *pHost;
    // class that performs untis conversion;
    UNITS_CONVERSION<CONV> CONV_UNITS_FROM;
};

// Elastic
template<CnvrtUnits CONV> 
struct COORD_TRANSFORMER<Q3D,Elastic,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // tree inital coordinates came from workspace and all are interconnnected. All additional are defined by properties:
        if(!pHost->fillAddProperties(Coord,nd,3))return false;
         // 
        rotMat = pHost->getTransfMatrix();
        //
        CONV_UNITS_FROM.setUpConversion(this->pHost); 
        // get pointer to the positions of the detectors
        pDet = &(ConvertToMDEvents::getPrepDetectors(pHost).det_dir[0]);
    
        return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
          UNUSED_ARG(Coord); 
          CONV_UNITS_FROM.updateConversion(i);

          ex = (pDet+i)->X();
          ey = (pDet+i)->Y();
          ez = (pDet+i)->Z();
          return true;
    }

    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
          UNUSED_ARG(i);
          //convert X from any units it initally is, into momentum transfer (if necessary)
          coord_t k0 = CONV_UNITS_FROM.getXConverted(X,j);   

          double  qx  =  -ex*k0;                
          double  qy  =  -ey*k0;
          double  qz  = (1 - ez)*k0;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(Coord[2]<pHost->dim_min[2]||Coord[2]>=pHost->dim_max[2])return false;

         return true;
    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){}
private:
    // directions to the detectors 
    double ex,ey,ez;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // pointer to the beginning of the array with 
    Kernel::V3D *pDet;
    // pointer to the algoritm, which calls all these transformations
    ConvertToMDEvents *pHost;
    // class that performs untis conversion;
    UNITS_CONVERSION<CONV> CONV_UNITS_FROM;
};


} // End MDAlgorighms namespace
} // End Mantid namespace

#endif