#include "MantidMDAlgorithms/CenterpieceRebinning.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm(), m_progress(NULL) 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
    if( m_progress ){
            delete m_progress;
            m_progress=NULL;
    }
}
void 
CenterpieceRebinning::init_source(MDWorkspace_sptr inputWSX)
{
MDWorkspace_sptr inputWS;
  // Get the input workspace
    if(existsProperty("Input")){
         inputWS = getProperty("Input");
         if(!inputWS){
              throw(std::runtime_error("input workspace has to exist"));
         }

    }else{
       throw(std::runtime_error("input workspace has to be availible through properties"));
    }
   std::string filename;
   if(existsProperty("Filename")){
      filename= getProperty("Filename");
   }else{
      throw(std::runtime_error("filename property can not be found"));
   }

    inputWS->read_mdd(filename.c_str());

    // set up slicing property to the shape of current workspace;
    MDGeometryDescription *pSlicing = dynamic_cast< MDGeometryDescription *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
            throw(std::runtime_error("can not obtain slicing property from the property manager"));
     }

     pSlicing->build_from_geometry(*inputWS);
     pSlicing=NULL; // should remain in Property
}
/*
void
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description,const std::string &XML_definition)
{
    this->slicingProperty.fromXMLstring(slicing_description);
}
*/
void
CenterpieceRebinning::init()
{
      declareProperty(new WorkspaceProperty<MDWorkspace>("Input","",Direction::Input),"initial MD workspace");
      declareProperty(new WorkspaceProperty<MDWorkspace>("Result","",Direction::Output),"final MD workspace");

      declareProperty(new MDPropertyGeometry("SlicingData","",Direction::Input));
      declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load), "The file containing input MD dataset");


      m_progress = new Progress(this,0,1,10);

   
}
//
void 
CenterpieceRebinning::exec()
{
 MDWorkspace_sptr inputWS;
 MDWorkspace_sptr outputWS;

  
  // Get the input workspace
  if(existsProperty("Input")){
        inputWS = getProperty("Input");
        if(!inputWS){
            throw(std::runtime_error("input workspace has to exist"));
        }
  }else{
      throw(std::runtime_error("input workspace has to be accessible through properties"));
  }


  // Now create the output workspace
  if(existsProperty("Result")){
 
     outputWS = getProperty("Result");
     if(!outputWS){
        outputWS      = MDWorkspace_sptr(new MDWorkspace(4));
        setProperty("Result", outputWS);
     }
  }else{
        throw(std::runtime_error("output workspace has to be created "));
  }
  if(inputWS==outputWS){
      throw(std::runtime_error("input and output workspaces have to be different"));
  }

 
  MDPropertyGeometry  *pSlicing; 
  if(existsProperty("SlicingData")){ 
 // get slicing data from property manager. These data has to bebeen shaped to proper form . 
    pSlicing = dynamic_cast< MDPropertyGeometry *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
                throw(std::runtime_error("can not obtain slicing property from the property manager"));
    }
  }else{
        throw(std::runtime_error("slising property has to exist and has to be defined "));
  }
 
 
  // transform output workspace to the target shape and allocate memory for resulting matrix
  outputWS->alloc_mdd_arrays(*pSlicing);

 

  std::vector<size_t> preselected_cells_indexes;
  size_t  n_precelected_pixels(0);

  this->preselect_cells(*inputWS,*pSlicing,preselected_cells_indexes,n_precelected_pixels);
  if(n_precelected_pixels == 0)return;

  unsigned int n_hits = n_precelected_pixels/PIX_BUFFER_SIZE+1;

  size_t    n_pixels_read(0),
            n_pixels_selected(0),
            n_pix_in_buffer(0),pix_buffer_size(PIX_BUFFER_SIZE);
  
  std::vector<sqw_pixel> pix_buf;
  pix_buf.resize(PIX_BUFFER_SIZE);
  

  // get pointer for data to rebin to; 
  MD_image_point *pImage    = outputWS->get_pData();
  // and the number of elements the image has;
  size_t         image_size=  outputWS->getDataSize();
 //
  double boxMin[4],boxMax[4];
  boxMin[0]=boxMin[1]=boxMin[2]=boxMin[3]=FLT_MAX;
  boxMax[0]=boxMax[1]=boxMax[2]=boxMax[3]=FLT_MIN;

  transf_matrix trf = this->build_scaled_transformation_matrix(*inputWS,*pSlicing);
// start reading and rebinning;
  size_t n_starting_cell(0);
  for(unsigned int i=0;i<1;i++){
      n_starting_cell+=inputWS->read_pix_selection(preselected_cells_indexes,n_starting_cell,pix_buf,pix_buffer_size,n_pix_in_buffer);
      n_pixels_read  +=n_pix_in_buffer;
      
      n_pixels_selected+=this->rebin_dataset4D(trf,&pix_buf[0],n_pix_in_buffer,pImage,boxMin,boxMax);
  } 
  this->finalise_rebinning(pImage,image_size);

  pix_buf.clear();


}
//

void 
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition)
{  

double originX, originY, originZ, normalX, normalY, normalZ;
/*
Mantid::API::ImplicitFunction* ifunc = Mantid::API::Instance().ImplicitFunctionFactory(xmlDefinitions, xmlInstructions);
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(ifunc);

for(int i = 0; i < compFunction->getNFunctions() ; i++)
{

  ImplicitFunction* nestedFunction =   compFunction->getFunction().at(i).get();
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(nestedFunction);
  if(NULL != plane)
  {
    originX = plane->getOriginX();
    originY = plane->getOriginY();
    originZ = plane->getOriginZ();
    normalX = plane->getNOrmalX();
    normalY = plane->getNormalY();
    normalZ = plane->getNOrmalX();
    break;
  }
}
*/
}

transf_matrix 
CenterpieceRebinning::build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target)
{

    transf_matrix trf;


    unsigned int  i,ic,j;
    unsigned int nDims = Source.getNumDims();
    trf.nDimensions    = nDims;
    trf.trans_bott_left.assign(nDims,0);
    trf.cut_min.assign(nDims,-1);
    trf.cut_max.assign(nDims,1);
    trf.axis_step.assign(nDims,1);
    trf.stride.assign(nDims,0);

    MDDimension *pDim(NULL);
    // for convenience can use dimension accessors from source
    //MDGeometryDescription tSource(Source);

    for(i=0;i<nDims;i++){
        trf.trans_bott_left[i]=target.shift(i);
        trf.axis_step[i]=(target.cutMax(i)-target.cutMin(i))/target.numBins(i);
        trf.cut_max[i]  = target.cutMax(i)/trf.axis_step[i];
        trf.cut_min[i]  = target.cutMin(i)/trf.axis_step[i];
        trf.stride[i]   = target.getStride(i);
    }
    std::vector<double> rot = target.getRotations();
    std::vector<double> basis[3]; // not used at the momemnt;

    for(i=0;i<3;i++){
        ic = i*3;
        for(j=0;j<3;j++){
            trf.rotations[ic+j]=rot[ic+j]/trf.axis_step[i];
        }
    }
   
    return trf;

}



//
/*! function calculates min and max values of the array of nPoints points (vertices of hypercube)
*
*/
void 
minmax(double &rMin,double &rMax,const std::vector<double> &box)
{
    rMin=box[0];
    rMax=box[0];
    size_t nPoints = box.size();
    for(int i=1;i<nPoints ;i++){
        if(box[i]<rMin)rMin=box[i];
        if(box[i]>rMax)rMax=box[i];
    }
}
//
void
CenterpieceRebinning::preselect_cells(const MDDataObjects::MDData &Source, const Geometry::MDGeometryDescription &target, std::vector<size_t> &cells_to_select,size_t &n_preselected_pix)
{
// this algorithm can be substantially enhanced;
    int i;
    n_preselected_pix=0;
    //transf_matrix scaled_trf = this->build_scaled_transformation_matrix(Source,target);

   // transform the grid into new system of coordinate and calculate cell indexes, which contribute into the 
   // dataset;
   int j,k,l,m,mp,mm,sizem;
   double xt1,yt1,zt1,Etm,Etp;
   MDDimension *pDim(NULL);
   double rotations[9];
   rotations[0]=rotations[1]=rotations[2]=rotations[3]=rotations[4]=rotations[5]=rotations[6]=rotations[7]=rotations[8]=0;
   rotations[0]=rotations[4]=rotations[8]=1;

   // get pointer to the image data;
   const MD_image_point  *const data = Source.get_const_pData();

   // evaluate the capacity of the orthogonal dimensions;
   unsigned int  nReciprocalDims= Source.getNumReciprocalDims();
   unsigned int nOrthogonal     = Source.getNumDims()-nReciprocalDims;
   std::vector<std::string> tag=Source.getBasisTags();
   long ind,orthoSize=1;
 
   size_t stride;
   int  nContributed(0);
   // this is the array of vectors to keep orthogonal indexes;
   std::vector<size_t> *enInd = new std::vector<size_t>[nOrthogonal];
   for(l=nReciprocalDims;l<Source.getNumDims();l++){
       nContributed=0;
       pDim   = Source.getDimension(tag[l]);
       sizem  = pDim->getNBins();
       stride = pDim->getStride();
       for(m=0;m<sizem;m++){
           // is rightmpst for min or leftmost for max in range?
              mp=m+1; 
              mm=m-1; if(mm<0)    mm=0;
            // check if it can be in ranges
              Etp=pDim->getX(mp);
              Etm=pDim->getX(mm);

           if(Etp<target.cutMin(l)||Etm>=target.cutMax(l)) continue;
            // remember the index of THIS axis 
            enInd[l-nReciprocalDims].push_back(m*stride);
            nContributed++;
       }
       orthoSize*=nContributed;
       if(nContributed==0){  // no cells contribute into the cut; Return
           return;
       }

   }
   // multiply all orthogonal vectors providing size(en)*size(ga1)*size(ga2)*size(ga3) matrix;
   std::vector<long> *orthoInd = new std::vector<long>;
   orthoInd->reserve(orthoSize);
   for(i=0;i<enInd[0].size();i++){
       orthoInd->push_back(enInd[0].at(i));
   }
   for(l=1;l<nOrthogonal;l++){
       size_t orthSize=orthoInd->size();
       for(j=0;j<orthSize;j++){
           long indDim0=orthoInd->at(j);
           for(i=0;enInd[l].size();i++){
               orthoInd->push_back(indDim0+enInd[l].at(i));
           }
       }
   }
   delete [] enInd;

// evaluate the capacity of the 3D space;
// Define 3D subspace and transform it into the coordinate system of the new box;
   size_t size3D(1);
   std::vector<MDDimension* const>rec_dim(nReciprocalDims,NULL);
   for(i=0;i<nReciprocalDims;i++){
       rec_dim[i] = Source.getDimension(tag[i]);
       size3D    *= (rec_dim[i]->getNBins()+1);
   }
   /*
   // dummy dimension is always integrated and 
   MDDimension Dummy();
   for(i=nReciprocalDims;i<3;i++){
       rec_dim[i]=&Dummy;
   }
   */
   std::vector<double> rx,ry,rz,xx,yy,zz;
   rx.assign(size3D,0); xx.assign(size3D,0);
   ry.assign(size3D,0); yy.assign(size3D,0);
   rz.assign(size3D,0); zz.assign(size3D,0);
// nAxis points equal nBins+1;
// lattice points transformed into new coordinate system. 
// needed modifications for nRecDim<3
   int ic(0);
   for(k=0;k<=rec_dim[2]->getNBins();k++){
       for(j=0;j<=rec_dim[1]->getNBins();j++){
           for(i=0;i<=rec_dim[0]->getNBins();i++){
               rx[ic]=rec_dim[0]->getX(i);
               ry[ic]=rec_dim[1]->getX(j);
               rz[ic]=rec_dim[2]->getX(k);
               ic++;
           }
       }
   }
   for(i=0;i<size3D;i++){
        xt1=rx[i];yt1=ry[i];zt1=rz[i];

        xx[i]=xt1*rotations[0]+yt1*rotations[3]+zt1*rotations[6];
        yy[i]=xt1*rotations[1]+yt1*rotations[4]+zt1*rotations[7];
        zz[i]=xt1*rotations[2]+yt1*rotations[5]+zt1*rotations[8];
   }
   rx.clear();   ry.clear();   rz.clear();
   int im,ip,jm,jp,km,kp;
   nCell3D sh(rec_dim[0]->getNBins()+1,rec_dim[1]->getNBins()+1);
//           ind3D(this->dim_sizes[u1],this->dim_sizes[u2]);
   double rMin,rMax;
   unsigned int box_dim=1;
   for(i=0;i<nReciprocalDims;i++){
       box_dim*=2;
   }
   std::vector<double> r(box_dim,0);
   size_t ind3;

   for(k=0;k<rec_dim[2]->getNBins();k++){
       km=k-1; if(km<0)            km=0;
       kp=k+1; 
       for(j=0;j<rec_dim[1]->getNBins();j++){
           jm=j-1; if(jm<0)            jm=0;
           jp=j+1; 

           for(i=0;i<rec_dim[0]->getNBins();i++){
               im=i-1; if(im<0)            im=0;
               ip=i+1; 

               r[0]=xx[sh.nCell(im,jm,km)]; r[1]=xx[sh.nCell(ip,jm,km)]; r[2]=xx[sh.nCell(im,jp,km)]; r[3]=xx[sh.nCell(ip,jp,km)];
               r[4]=xx[sh.nCell(im,jm,kp)]; r[5]=xx[sh.nCell(ip,jm,kp)]; r[6]=xx[sh.nCell(im,jp,kp)]; r[7]=xx[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<target.cutMin(0)||rMin>=target.cutMax(0))continue;

               r[0]=yy[sh.nCell(im,jm,km)];  r[1]=yy[sh.nCell(ip,jm,km)];r[2]=yy[sh.nCell(im,jp,km)];  r[3]=yy[sh.nCell(ip,jp,km)];
               r[4]=yy[sh.nCell(im,jm,kp)];  r[5]=yy[sh.nCell(ip,jm,kp)];r[6]=yy[sh.nCell(im,jp,kp)];  r[7]=yy[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<target.cutMin(1)||rMin>=target.cutMax(1))continue;

               r[0]=zz[sh.nCell(im,jm,km)];  r[1]=zz[sh.nCell(ip,jm,km)];r[2]=zz[sh.nCell(im,jp,km)];  r[3]=zz[sh.nCell(ip,jp,km)];
               r[4]=zz[sh.nCell(im,jm,kp)];  r[5]=zz[sh.nCell(ip,jm,kp)];r[6]=zz[sh.nCell(im,jp,kp)];  r[7]=zz[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<target.cutMin(2)||rMin>=target.cutMax(2))continue;

               ind3=i*rec_dim[0]->getStride()+j*rec_dim[1]->getStride()+k*rec_dim[2]->getStride();
               for(l=0;l<orthoInd->size();l++){
                   ind = ind3+orthoInd->at(l);
                   if(data[ind].npix>0){
                        cells_to_select.push_back(ind);
                        n_preselected_pix+=data[ind].npix;
                   }
               }


           }
       }
    }
    delete orthoInd;


}
//
size_t  
CenterpieceRebinning::rebin_dataset4D(const transf_matrix &rescaled_transf, const sqw_pixel *source_pix, size_t nPix,MDDataObjects::MD_image_point *data, double boxMin[],double boxMax[])
{
// set up auxiliary variables and preprocess them. 
double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
       pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;
size_t nPixel_retained(0);


unsigned int nDims = rescaled_transf.nDimensions;
double rotations_ustep[9];
std::vector<double> axis_step_inv(nDims,0),shifts(nDims,0),min_limit(nDims,-1),max_limit(nDims,1);
bool  ignore_something,ignote_all,ignore_nan(this->ignore_nan),ignore_inf(this->ignore_inf);

ignore_something=ignore_nan|ignore_inf;
ignote_all      =ignore_nan&ignore_inf;
if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
}


for(unsigned int ii=0;ii<nDims;ii++){
    axis_step_inv[ii]=1/rescaled_transf.axis_step[ii];
}
for(int ii=0;ii<rescaled_transf.nDimensions;ii++){
    shifts[ii]   =rescaled_transf.trans_bott_left[ii];
    min_limit[ii]=rescaled_transf.cut_min[ii];
    max_limit[ii]=rescaled_transf.cut_max[ii];
}
for(int ii=0;ii<9;ii++){
    rotations_ustep[ii]=rescaled_transf.rotations[ii];
}
int num_OMP_Threads(1);
bool keep_pixels(false);

//int nRealThreads;
size_t i,indl;
int    indX,indY,indZ,indE;
size_t  nDimX(rescaled_transf.stride[0]),nDimY(rescaled_transf.stride[1]),nDimZ(rescaled_transf.stride[2]),nDimE(rescaled_transf.stride[3]); // reduction dimensions; if 0, the dimension is reduced;


// min-max value initialization

pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
//
// work at least for MSV 2008
#ifdef _OPENMP  
omp_set_num_threads(num_OMP_Threads);


#pragma omp parallel default(none), private(i,j0,xt,yt,zt,xt1,yt1,zt1,Et,indX,indY,indZ,indE), \
     shared(actual_pix_range,this->pix,ok,ind, \
     this->nPixels,newsqw), \
     firstprivate(pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin, pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax,\
                  ignote_all,ignore_nan,ignore_inf,ignore_something,transform_energy,
                  ebin_inv,Inf,trf,\
                  nDimX,nDimY,nDimZ,nDimE), \
     reduction(+:nPixel_retained)
#endif
{
//	#pragma omp master
//{
//    nRealThreads= omp_get_num_threads()
//	 mexPrintf(" n real threads %d :\n",nRealThread);}

#pragma omp for schedule(static,1)
    for(i=0;i<nPix;i++){
        sqw_pixel pix=source_pix[i];

      // Check for the case when either data.s or data.e contain NaNs or Infs, but data.npix is not zero.
      // and handle according to options settings.
            if(ignore_something){
                if(ignote_all){
                    if(pix.s==Inf||isNaN(pix.s)||
                    pix.err==Inf ||isNaN(pix.err)){
                            continue;
                    }
                }else if(ignore_nan){
                    if(isNaN(pix.s)||isNaN(pix.err)){
                        continue;
                    }
                }else if(ignore_inf){
                    if(pix.s==Inf||pix.err==Inf){
                        continue;
                    }
                }
            }

      // Transform the coordinates u1-u4 into the new projection axes, if necessary
      //    indx=[(v(1:3,:)'-repmat(trans_bott_left',[size(v,2),1]))*rot_ustep',v(4,:)'];  % nx4 matrix
            xt1=pix.qx    -shifts[0];
            yt1=pix.qy    -shifts[1];
            zt1=pix.qz    -shifts[2];

            // transform energy
            Et=(pix.En    -shifts[3])*axis_step_inv[3];

//  ok = indx(:,1)>=cut_range(1,1) & indx(:,1)<=cut_range(2,1) & indx(:,2)>=cut_range(1,2) & indx(:,2)<=urange_step(2,2) & ...
//       indx(:,3)>=cut_range(1,3) & indx(:,3)<=cut_range(2,3) & indx(:,4)>=cut_range(1,4) & indx(:,4)<=cut_range(2,4);
            if(Et<min_limit[3]||Et>=max_limit[3]){
                continue;
            }

            xt=xt1*rotations_ustep[0]+yt1*rotations_ustep[3]+zt1*rotations_ustep[6];
            if(xt<min_limit[0]||xt>=max_limit[0]){
                continue;
            }

            yt=xt1*rotations_ustep[1]+yt1*rotations_ustep[4]+zt1*rotations_ustep[7];
            if(yt<min_limit[1]||yt>=max_limit[1]){
                continue;
            }

            zt=xt1*rotations_ustep[2]+yt1*rotations_ustep[5]+zt1*rotations_ustep[8];
            if(zt<min_limit[2]||zt>=max_limit[2]) {
                continue;
            }
            nPixel_retained++;



//     indx=indx(ok,:);    % get good indices (including integration axes and plot axes with only one bin)
            indX=(int)floor(xt-min_limit[0]);
            indY=(int)floor(yt-min_limit[1]);
            indZ=(int)floor(zt-min_limit[2]);
            indE=(int)floor(Et-min_limit[3]);
//
            indl  = indX*nDimX+indY*nDimY+indZ*nDimZ+indE*nDimE;
 // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
#pragma omp atomic
            data[indl].s   +=pix.s;  
#pragma omp atomic
            data[indl].err +=pix.err;
#pragma omp atomic
            data[indl].npix++;
#pragma omp atomic
            // this request substantial thinking -- will not do it this way as it is very slow
           // this->pix_array[indl].cell_memPixels.push_back(pix);

//
//    actual_pix_range = [min(actual_pix_range(1,:),min(indx,[],1));max(actual_pix_range(2,:),max(indx,[],1))];  % true range of data
            if(xt<pix_Xmin)pix_Xmin=xt;
            if(xt>pix_Xmax)pix_Xmax=xt;

            if(yt<pix_Ymin)pix_Ymin=yt;
            if(yt>pix_Ymax)pix_Ymax=yt;

            if(zt<pix_Zmin)pix_Zmin=zt;
            if(zt>pix_Zmax)pix_Zmax=zt;

            if(Et<pix_Emin)pix_Emin=Et;
            if(Et>pix_Emax)pix_Emax=Et;

    } // end for i -- imlicit barrier;
#pragma omp critical
    {
        if(boxMin[0]>pix_Xmin/axis_step_inv[0])boxMin[0]=pix_Xmin/axis_step_inv[0];
        if(boxMin[1]>pix_Ymin/axis_step_inv[1])boxMin[1]=pix_Ymin/axis_step_inv[1];
        if(boxMin[2]>pix_Zmin/axis_step_inv[2])boxMin[2]=pix_Zmin/axis_step_inv[2];
        if(boxMin[3]>pix_Emin/axis_step_inv[3])boxMin[3]=pix_Emin/axis_step_inv[3];

        if(boxMax[0]<pix_Xmax/axis_step_inv[0])boxMax[0]=pix_Xmax/axis_step_inv[0];
        if(boxMax[1]<pix_Ymax/axis_step_inv[1])boxMax[1]=pix_Ymax/axis_step_inv[1];
        if(boxMax[2]<pix_Zmax/axis_step_inv[2])boxMax[2]=pix_Zmax/axis_step_inv[2];
        if(boxMax[3]<pix_Emax/axis_step_inv[3])boxMax[3]=pix_Emax/axis_step_inv[3];
    }
} // end parallel region


return nPixel_retained;
}
//
size_t
CenterpieceRebinning::finalise_rebinning(MDDataObjects::MD_image_point *data,size_t data_size)
{
size_t i;
// normalize signal and error of the dnd object;
if(data[0].npix>0){
    data[0].s   /= data[0].npix;
    data[0].err /=(data[0].npix*data[0].npix);
}
// and calculate cells location for pixels;
data[0].chunk_location=0;

// counter for the number of retatined pixels;
size_t nPix = data[0].npix;
for(i=1;i<data_size;i++){   
    data[i].chunk_location=data[i-1].chunk_location+data[i-1].npix; // the next cell starts from the the boundary of the previous one
                                              // plus the number of pixels in the previous cell
    if(data[i].npix>0){
        nPix        +=data[i].npix;
        data[i].s   /=data[i].npix;
        data[i].err /=(data[i].npix*data[i].npix);
    }
};
return nPix;
}//***************************************************************************************


} //namespace MDAlgorithms
} //namespace Mantid