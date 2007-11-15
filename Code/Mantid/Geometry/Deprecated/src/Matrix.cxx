#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <complex>
#include <vector>

#include "AuxException.h"
#include "mathSupport.h"
#include "Matrix.h"

template<typename T>
std::ostream& 
Geometry::operator<<(std::ostream& of,const Geometry::Matrix<T>& A)
  /*!
    External Friend :: outputs point to a stream 
    \param A :: Matrix to write out
    \param of :: output stream
    \returns The output stream (of)
  */
{
  of<<std::endl;
  A.write(of,5);
  return of;
}

namespace Geometry
{

template<typename T>
Matrix<T>::Matrix(const int nrow,const int ncol)
  : nx(0),ny(0),V(0)
  /*!
    Constructor with pre-set sizes. Matrix is zeroed
    \param nrow :: number of rows
    \param ncol :: number of columns
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(nrow,ncol);
  zeroMatrix();
}

template<typename T>
Matrix<T>::Matrix(const std::vector<T>& A,const std::vector<T>& B)
  : nx(0),ny(0),V(0)
  /*!
    Constructor to take two vectors and multiply them to 
    construct a matrix. (assuming that we have columns x row
    vector.
    \param A :: Column vector to multiply
    \param B :: Row vector to multiply
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.size(),B.size());
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      V[i][j]=A[i]*B[j];
  
}



template<typename T>
Matrix<T>::Matrix(const Matrix<T>& A)
  : nx(0),ny(0),V(0)
  /*! 
    Simple copy constructor
    \param A :: Object to copy
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.nx,A.ny);
  if (nx*ny)
    {
      for(int i=0;i<nx;i++)
	for(int j=0;j<ny;j++)
	  V[i][j]=A.V[i][j];
    }
}


template<typename T>
Matrix<T>&
Matrix<T>::operator=(const Matrix<T>& A)
  /*! 
    Simple assignment operator 
    \param A :: Object to copy
  */
{
  if (&A!=this)
    {
      setMem(A.nx,A.ny);
      if (nx*ny)
	for(int i=0;i<nx;i++)
	  for(int j=0;j<ny;j++)
	    V[i][j]=A.V[i][j];
    }
  return *this;
}

template<typename T>
Matrix<T>::~Matrix()
  /*!
    Delete operator :: removes memory for 
    matrix
  */
{
  deleteMem();
}


template<typename T>
Matrix<T>&
Matrix<T>::operator+=(const Matrix<T>& A)
   /*! 
     Matrix addition THIS + A  
     If the size is different then 0 is added where appropiate
     Matrix A is not expanded.
     \param A :: Matrix to add
     \returns Matrix(this + A)
   */
{
  const int Xpt((nx>A.nx) ? A.nx : nx);
  const int Ypt((ny>A.ny) ? A.ny : ny);
  for(int i=0;i<Xpt;i++)
    for(int j=0;j<Ypt;j++)
      V[i][j]+=A.V[i][j];

  return *this;
}

template<typename T>
Matrix<T>&
Matrix<T>::operator-=(const Matrix<T>& A)
   /*! 
     Matrix subtractoin THIS - A  
     If the size is different then 0 is added where appropiate
     Matrix A is not expanded.
     \param A :: Matrix to add
     \returns Ma
   */
{
  const int Xpt((nx>A.nx) ? A.nx : nx);
  const int Ypt((ny>A.ny) ? A.ny : ny);
  for(int i=0;i<Xpt;i++)
    for(int j=0;j<Ypt;j++)
      V[i][j]-=A.V[i][j];

  return *this;
}

template<typename T>
Matrix<T>
Matrix<T>::operator+(const Matrix<T>& A)
   /*! 
     Matrix addition THIS + A  
     If the size is different then 0 is added where appropiate
     Matrix A is not expanded.
     \param A :: Matrix to add
     \returns Matrix(this + A)
   */
{
  Matrix<T> X(*this);
  return X+=A;
}

template<typename T>
Matrix<T>
Matrix<T>::operator-(const Matrix<T>& A)
   /*! 
     Matrix subtraction THIS - A  
     If the size is different then 0 is subtracted where 
     appropiate. This matrix determines the size 
     \param A :: Matrix to add
     \returns Matrix(this + A)
   */
{
  Matrix<T> X(*this);
  return X-=A;
}

template<typename T>
Matrix<T>
Matrix<T>::operator*(const Matrix<T>& A) const
  /*! 
    Matrix multiplication THIS * A  
    \param A :: Matrix to multiply by  (this->row must == A->columns)
    \throw MisMatch<int> if there is a size mismatch.
    \return Matrix(This * A) 
 */
{
  if (ny!=A.nx)
    throw ColErr::MisMatch<int>(ny,A.nx,"Matrix::operator*(Matrix)");
  Matrix<T> X(nx,A.ny);
  for(int i=0;i<nx;i++)
    for(int j=0;j<A.ny;j++)
      for(int kk=0;kk<ny;kk++)
	X.V[i][j]+=V[i][kk]*A.V[kk][j];
  return X;
}

template<typename T>
std::vector<T>
Matrix<T>::operator*(const std::vector<T>& Vec) const
  /*! 
    Matrix multiplication THIS * Vec to produce a vec  
    \param Vec :: size of vector > this->nrows
    \throw MisMatch<int> if there is a size mismatch.
    \return Matrix(This * Vec)
  */
{
  std::vector<T> Out;
  if (ny>static_cast<int>(Vec.size()))
    throw ColErr::MisMatch<int>(ny,Vec.size(),"Matrix::operator*(Vec)");

  Out.resize(nx);
  for(int i=0;i<nx;i++)
    {
      Out[i]=0;
      for(int j=0;j<ny;j++)
	Out[i]+=V[i][j]*Vec[j];
    }
  return Out;
}

template<typename T>
Matrix<T>
Matrix<T>::operator*(const T& Value) const
   /*! 
     Matrix multiplication THIS * Value  
     \param Value :: Scalar to multiply by
     \return V * (this)
   */
{
  Matrix<T> X(*this);
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      X.V[i][j]*=Value;
  return X;
}

template<typename T>
Matrix<T>&
Matrix<T>::operator*=(const Matrix<T>& A)
   /*! 
     Matrix multiplication THIS *= A  
     Note that we call operator* to avoid the problem
     of changing matrix size.
    \param A :: Matrix to multiply by  (this->row must == A->columns)
    \return This *= A 
   */
{
  if (ny!=A.nx)
    throw ColErr::MisMatch<int>(ny,A.nx,"Matrix*=(Matrix<T>)");
  // This construct to avoid the problem of changing size
  *this = this->operator*(A);
  return *this;
}

template<typename T>
Matrix<T>&
Matrix<T>::operator*=(const T& Value)
   /*! 
     Matrix multiplication THIS * Value  
     \param Value :: Scalar to multiply matrix by
     \return *this
   */
{
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      V[i][j]*=Value;
  return *this;
}

template<typename T>
Matrix<T>&
Matrix<T>::operator/=(const T& Value)
   /*! 
     Matrix divishio THIS / Value  
     \param Value :: Scalar to multiply matrix by
     \return *this
   */
{
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      V[i][j]/=Value;
  return *this;
}

template<typename T>
int
Matrix<T>::operator==(const Matrix<T>& A) const
  /*! 
    Element by element comparison within tolerance.
    Tolerance means that the value must be > tolerance
    and less than (diff/max)>tolerance 

    Always returns 0 if the Matrix have different sizes
    \param A :: matrix to check.
    \return 1 on success 
  */
{
  const double Tolerance(1e-8);
  if (&A!=this)       // this == A == always true
    {
      if(A.nx!=nx || A.ny!=ny)
	return 0;

      double maxS(0.0);
      double maxDiff(0.0);   // max di
      for(int i=0;i<nx;i++)
	for(int j=0;j<ny;j++)
	  {
	    const T diff=(V[i][j]-A.V[i][j]);
	    if (fabs(diff)>maxDiff)
	      maxDiff=fabs(diff);
	    if (fabs(V[i][j])>maxS)
	      maxS=fabs(V[i][j]);
	  }
      if (maxDiff<Tolerance)
	return 1;
      if (maxS>1.0 && (maxDiff/maxS)<Tolerance)
	return 1;
      return 0;
    }
  //this == this is true
  return 1;
}

template<typename T>
void
Matrix<T>::deleteMem()
  /*!
    Deletes the memory held in matrix 
  */
{
  if (V)
    {
      delete [] *V;
      delete [] V;
      V=0;
    }
  nx=0;
  ny=0;
  return;
}

template<typename T>
void
Matrix<T>::setMem(const int a,const int b)
  /*! 
    Sets the memory held in matrix 
    \param a :: number of rows
    \param b :: number of columns
  */
{
  if (a==nx && b==ny) 
    return;

  deleteMem();
  if (a<=0 || b<=0)
    return;

  nx=a;
  ny=b;  
  if (nx*ny)
    {
      T* tmpX=new T[nx*ny];
      V=new T*[nx];
      for (int i=0;i<nx;i++)
	V[i]=tmpX + (i*ny);
    }
  return;
}  

template<typename T> 
void
Matrix<T>::swapRows(const int RowI,const int RowJ)
  /*! 
    Swap rows I and J
    \param RowI :: row I to swap
    \param RowJ :: row J to swap
  */
{
  if (nx*ny && RowI<nx && RowJ<nx &&
      RowI!=RowJ) 
    {
      for(int k=0;k<ny;k++)
        {
	  T tmp=V[RowI][k];
	  V[RowI][k]=V[RowJ][k];
	  V[RowJ][k]=tmp;
	}
    }
  return;
}

template<typename T> 
void
Matrix<T>::swapCols(const int colI,const int colJ)
  /*! 
    Swap columns I and J 
    \param colI :: col I to swap
    \param colJ :: col J to swap
  */
{
  if (nx*ny && colI<ny && colJ<ny &&
      colI!=colJ) 
    {
      for(int k=0;k<nx;k++)
        {
	  T tmp=V[k][colI];
	  V[k][colI]=V[k][colJ];
	  V[k][colJ]=tmp;
	}
    }
  return;
}

template<typename T> 
void
Matrix<T>::zeroMatrix()
  /*! 
    Zeros all elements of the matrix 
  */
{
  if (nx*ny)
    for(int i=0;i<nx;i++)
      for(int j=0;j<ny;j++)
	V[i][j]=static_cast<T>(0);
  return;
}

template<typename T> 
void
Matrix<T>::identityMatrix()
  /*!
    Makes the matrix an idenity matrix.
    Zeros all the terms outside of the square
  */
{
  if (nx*ny)
    for(int i=0;i<nx;i++)
      for(int j=0;j<ny;j++)
	V[i][j]=(j==i) ? 1 : 0;
  return;
}

template<typename T>
void
Matrix<T>::rotate(const double tau,const double s,const int i,const int j,
		  const int k,const int m)
  /*!
    Applies a rotation to a particular point of tan(theta)=tau
    \param tau :: tan(theta) 
    \param s :: sin(theta)
    \param i ::  first index (xpos) 
    \param j ::  first index (ypos) 
    \param k ::  second index (xpos) 
    \param m ::  second index (ypos) 
   */
{
  const T gg=V[i][j];
  const T hh=V[k][m]; 
  V[i][j]=static_cast<T>(gg-s*(hh+gg*tau));
  V[k][m]=static_cast<T>(hh+s*(gg-hh*tau));
  return;
}

template<typename T>
Matrix<T>
Matrix<T>::fDiagonal(const std::vector<T>& Dvec) const
  /*!
    Construct a matrix based on 
    A * This, where A is made into a diagonal 
    matrix.
    \param Dvec :: diagonal matrix (just centre points)
    \returns a matrix multiplication
  */
{
  // Note:: nx,ny zeroed so setMem always works
  if (static_cast<int>(Dvec.size())!=nx)
    {
      std::ostringstream cx;
      cx<<"Matrix::fDiagonal Size: "<<Dvec.size()<<" "<<nx<<" "<<ny;
      throw ColErr::ExBase(0,cx.str());
    }
  Matrix<T> X(Dvec.size(),ny);
  for(int i=0;i<static_cast<int>(Dvec.size());i++)
    for(int j=0;j<ny;j++)
      X.V[i][j]=Dvec[i]*V[i][j];
  return X;
}

template<typename T>
Matrix<T>
Matrix<T>::bDiagonal(const std::vector<T>& Dvec) const
  /*!
    Construct a matrix based on 
    This * A, where A is made into a diagonal 
    matrix.
    \param Dvec :: diagonal matrix (just centre points)
  */
{
  // Note:: nx,ny zeroed so setMem always works
  if (static_cast<int>(Dvec.size())!=ny)
    {
      std::ostringstream cx;
      cx<<"Error Matrix::bDiaognal size:: "<<Dvec.size()
	<<" "<<nx<<" "<<ny;
      throw ColErr::ExBase(0,cx.str());
    }

  Matrix<T> X(nx,Dvec.size());
  for(int i=0;i<nx;i++)
    for(int j=0;j<static_cast<int>(Dvec.size());j++)
      X.V[i][j]=Dvec[j]*V[i][j];
  return X;
}

template<typename T>
Matrix<T>&
Matrix<T>::Transpose()
  /*! 
    Transpose the matrix : 
    Has a inplace transpose for a square matrix case.
  */
{
  if (!nx*ny)
    return *this;
  if (nx==ny)   // inplace transpose
    {
      for(int i=0;i<nx;i++)
	for(int j=i+1;j<ny;j++)
	  {
	    T tmp=V[i][j];
	    V[i][j]=V[j][i];
	    V[j][i]=tmp;
	  }
      return *this;
    }
  // irregular matrix
  // get some memory
  T* tmpX=new T[ny*nx];
  T** Vt=new T*[ny];
  for (int i=0;i<ny;i++)
    Vt[i]=tmpX + (i*nx);
  
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      Vt[j][i]=V[i][j];
  // remove old memory
  const int tx=nx;
  const int ty=ny;
  deleteMem();  // resets nx,ny
  // replace memory
  V=Vt;
  nx=ty;
  ny=tx;

  return *this;
}

template<>
int
Matrix<int>::GaussJordan(Geometry::Matrix<int>&)
  /*!
    Not valid for Integer
  */
{
  return 0;
}

template<typename T>
int
Matrix<T>::GaussJordan(Matrix<T>& B)
  /*!
    Invert this Matrix and solve the 
    form such that if A.x=B then  solve to generate x.  
    This requires that B is B[A.nx][Any]
    The result is placed back in B
   */
{
  // check for input errors
  if (nx!=ny || B.nx!=nx)
    return -1;
  
  // pivoted rows 
  std::vector<int> pivoted(nx);
  fill(pivoted.begin(),pivoted.end(),0);

  std::vector<int> indxcol(nx);   // Colunm index
  std::vector<int> indxrow(nx);   // row index

  int irow(0),icol(0);
  for(int i=0;i<nx;i++)
    {
      // Get Biggest non-pivoted item
      double bigItem=0.0;         // get point to pivot over
      for(int j=0;j<nx;j++)
	{
	  if (pivoted[j]!= 1)  //check only non-pivots
	    {
	      for(int k=0;k<nx;k++)
		if (!pivoted[k])
		  {
		    if (fabs(V[j][k]) >=bigItem)
		      {
			bigItem=fabs(V[j][k]);
			irow=j;
			icol=k;
		      }
		  }
	    }
	  else if (pivoted[j]>1)
	    throw ColErr::ExBase(j,"Error doing G-J elem on a singular matrix");
	}


      pivoted[icol]++;
      // Swap in row/col to make a diagonal item 
      if (irow != icol)
	{
	  swapRows(irow,icol);
	  B.swapRows(irow,icol);
	}
      indxrow[i]=irow;
      indxcol[i]=icol;

      if (V[icol][icol] == 0.0)
	{
	  std::cerr<<"Error doing G-J elem on a singular matrix"<<std::endl;
	  return 1;
	}

      const double pivDiv= 1.0/V[icol][icol];
      V[icol][icol]=1;
      for(int l=0;l<nx;l++) 
	V[icol][l] *= pivDiv;
      for(int l=0;l<B.ny;l++)
	B.V[icol][l] *=pivDiv;

      for(int ll=0;ll<nx;ll++)
	if (ll!=icol)
	  {
	    const double div_num=V[ll][icol];
	    V[ll][icol]=0.0;
	    for(int l=0;l<nx;l++) 
	      V[ll][l] -= V[icol][l]*div_num;
	    for(int l=0;l<B.ny;l++)
	      B.V[ll][l] -= B.V[icol][l]*div_num;
	  }
    }
  // Un-roll interchanges
  for(int l=nx-1;l>=0;l--)
    if (indxrow[l] !=indxcol[l])
      swapCols(indxrow[l],indxcol[l]);
  return 0;  
}

template<typename T>
std::vector<T>
Matrix<T>::Faddeev(Matrix<T>& InvOut)
  /*!
    Return the polynominal for the matrix
    and the inverse. 
    The polybnomial is such that
    \f[
      det(sI-A)=s^n+a_{n-1}s^{n-1} \dots +a_0
    \f]
    \param InvOut ::: output 
  */
{
  if (nx!=ny)
    throw ColErr::MisMatch<int>(nx,ny,"Matrix::Faddev(Matrix)");

  Matrix<T>& A(*this);
  Matrix<T> B(A);
  Matrix<T> Ident(nx,ny);


  T tVal=B.Trace();                     // Trace of the matrix
  std::vector<T> Poly;
  Poly.push_back(1);
  Poly.push_back(tVal);

  for(int i=0;i<nx-2;i++)   // skip first (just copy) and last (to keep B-1)
    {
      B=A*B - Ident*tVal;
      tVal=B.Trace();
      Poly.push_back(tVal/(i+1));
    }
  // Last on need to keep B;
  InvOut=B;
  B=A*B - Ident*tVal;
  tVal=B.Trace();
  Poly.push_back(tVal/nx);
  
  InvOut-= Ident* (-Poly[nx-1]);
  InvOut/= Poly.back();
  return Poly;
}

template<typename T>
T
Matrix<T>::Invert()
  /*!
    If the Matrix is square then invert the matrix
    using LU decomposition
    \returns Determinate (0 if the matrix is singular)
  */
{
  if (nx!=ny && nx<1)
    return 0;
  
  int* indx=new int[nx];   // Set in lubcmp

  double* col=new double[nx];
  int d;
  Matrix<T> Lcomp(*this);
  Lcomp.lubcmp(indx,d);
	
  double det=static_cast<double>(d);
  for(int j=0;j<nx;j++)
    det *= Lcomp.V[j][j];
  
  for(int j=0;j<nx;j++)
    {
      for(int i=0;i<nx;i++)
	col[i]=0.0;
      col[j]=1.0;
      Lcomp.lubksb(indx,col);
      for(int i=0;i<nx;i++)
	V[i][j]=static_cast<T>(col[i]);
    } 
  delete [] indx;
  delete [] col;
  return static_cast<T>(det);
}


template<typename T>
T
Matrix<T>::determinant() const
  /*!
    Calculate the derminant of the matrix
    \return Determinant of matrix.
  */
{
  if (nx!=ny)
    throw ColErr::MisMatch<int>(nx,ny,
	"Determinate error :: Matrix is not NxN");

  Matrix<T> Mt(*this);         //temp copy
  T D=Mt.factor();
  return D;
}

template<typename T>
T
Matrix<T>::factor()
  /*! 
     Gauss jordan diagonal factorisation 
     The diagonal is left as the values, 
     the lower part is zero.
  */
{
  if (nx!=ny || nx<1)
    throw ColErr::ExBase(0,"Matirx::fractor Matrix is not NxN");

  double Pmax;
  double deter=1.0;
  for(int i=0;i<nx-1;i++)       //loop over each row
    {
      int jmax=i;
      Pmax=fabs(V[i][i]);
      for(int j=i+1;j<nx;j++)     // find max in Row i
        {
	  if (fabs(V[i][j])>Pmax)
            {
	      Pmax=fabs(V[i][j]);
	      jmax=j;
	    }
	}
      if (Pmax<1e-8)         // maxtrix signular 
        {
	  std::cerr<<"Matrix Singlular"<<std::endl;
	  return 0;
	}
      // Swap Columns
      if (i!=jmax) 
        {
	  swapCols(i,jmax);
	  deter*=-1;            //change sign.
	}
      // zero all rows below diagonal
      Pmax=V[i][i];
      deter*=Pmax;
      for(int k=i+1;k<nx;k++)  // row index
        {
	  const double scale=V[k][i]/Pmax;
	  V[k][i]=static_cast<T>(0);
	  for(int q=i+1;q<nx;q++)   //column index
	    V[k][q]-=static_cast<T>(scale*V[i][q]);
	}
    }
  deter*=V[nx-1][nx-1];
  return static_cast<T>(deter);
}

template<typename T>
void 
Matrix<T>::normVert()
  /*!
    Normalise EigenVectors
    Assumes that they have already been calculated
  */
{
  for(int i=0;i<nx;i++)
    {
      T sum=0;
      for(int j=0;j<ny;j++)
	sum+=V[i][j]*V[i][j];
      sum=static_cast<T>(sqrt(sum));
      for(int j=0;j<ny;j++)
	V[i][j]/=sum;
    }
  return;
}

template<typename T>
T
Matrix<T>::compSum() const
  /*!
    Add up each component sums for the matrix
    \return \f$ \sum_i \sum_j V_{ij}^2 \f$
   */
{
  T sum(0);
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      sum+=V[i][j]*V[i][j];
  return sum;
}

template<typename T>
void 
Matrix<T>::lubcmp(int* rowperm,int& interchange)
  /*!
    Find biggest pivit and move to top row. Then
    divide by pivit.
    \param interchange :: odd/even nterchange (+/-1)
    \param rowperm :: row permuations
  */
{
  int imax(0),j,k;
  double sum,dum,big,temp;

  if (nx!=ny || nx<2)
    {
      std::cerr<<"Error with lubcmp"<<std::endl;
      return;
    }
  double *vv=new double[nx];
  interchange=1;
  for(int i=0;i<nx;i++)
    {
      big=0.0;
      for(j=0;j<nx;j++)
        if ((temp=fabs(V[i][j])) > big) 
	  big=temp;

      if (big == 0.0) 
	return;
      vv[i]=1.0/big;
    }

  for (j=0;j<nx;j++)
    {
      for(int i=0;i<j;i++)
        {
          sum=V[i][j];
          for(k=0;k<i;k++)
            sum-= V[i][k] * V[k][j];
          V[i][j]=static_cast<T>(sum);
        }
      big=0.0;
      imax=j;
      for (int i=j;i<nx;i++)
        {
          sum=V[i][j];
          for (k=0;k<j;k++)
            sum -= V[i][k] * V[k][j];
          V[i][j]=static_cast<T>(sum);
          if ( (dum=vv[i] * fabs(sum)) >=big)
            {
              big=dum;
              imax=i;
            }
        }

      if (j!=imax)
        {
          for(k=0;k<nx;k++)
            {                     //Interchange rows
              dum=V[imax][k];
              V[imax][k]=V[j][k];
              V[j][k]=static_cast<T>(dum);
            }
          interchange *= -1;
          vv[imax]=static_cast<T>(vv[j]);
        }
      rowperm[j]=imax;
      
      if (V[j][j] == 0.0) 
        V[j][j]=static_cast<T>(1e-14);
      if (j != nx-1)
        {
          dum=1.0/(V[j][j]);
          for(int i=j+1;i<nx;i++)
            V[i][j] *= static_cast<T>(dum);
        }
    }
  delete [] vv;
  return;
}

template<typename T>
void 
Matrix<T>::lubksb(const int* rowperm,double* b)
  /*!
    Impliments a separation of the Matrix
    into a triangluar matrix
  */
{
  int ii= -1;
 
  for(int i=0;i<nx;i++)
    {
      int ip=rowperm[i];
      double sum=b[ip];
      b[ip]=b[i];
      if (ii != -1) 
        for (int j=ii;j<i;j++) 
          sum -= V[i][j] * b[j];
      else if (sum) 
        ii=i;
      b[i]=sum;
    }

  for (int i=nx-1;i>=0;i--)
    {
      double sum=static_cast<T>(b[i]);
      for (int j=i+1;j<nx;j++)
        sum -= V[i][j] * b[j];
      b[i]=sum/V[i][i];
    }
  return;
}

template<typename T>
void
Matrix<T>::averSymmetric()
  /*!
    Simple function to take an average symmetric matrix
    out of the Matrix
  */
{
  const int minSize=(nx>ny) ? ny : nx;
  for(int i=0;i<minSize;i++)
    for(int j=i+1;j<minSize;j++)
      {
	V[i][j]=(V[i][j]+V[j][i])/2;
	V[j][i]=V[i][j];
      }
  return;
}

template<typename T> 
std::vector<T>
Matrix<T>::Diagonal() const
  /*!
    Returns the diagonal form as a vector
    \returns Diagonal elements
  */
{
  const int Msize=(ny>nx) ? nx : ny;
  std::vector<T> Diag(Msize);
  for(int i=0;i<Msize;i++)
    Diag[i]=V[i][i];
  return Diag;
}

template<typename T> 
T
Matrix<T>::Trace() const
  /*!
    Calculates the trace of the matrix
    \returns Trace of matrix 
  */
{
  const int Msize=(ny>nx) ? nx : ny;
  T Trx=0;
  for(int i=0;i<Msize;i++)
    Trx+=V[i][i];
  return Trx;
}

template<typename T> 
void
Matrix<T>::sortEigen(Matrix<T>& DiagMatrix) 
  /*!
    Sorts the eigenvalues into increasing
    size. Moves the EigenVectors correspondingly
    \param DiagMatrix :: matrix of the EigenValues
  */
{
  if (ny!=nx || nx!=DiagMatrix.nx || nx!=DiagMatrix.ny)
    {
      std::cerr<<"Matrix not Eigen Form"<<std::endl;
    }
  std::vector<int> index;
  std::vector<T> X=DiagMatrix.Diagonal();
  indexSort(X,index);
  Matrix<T> EigenVec(*this);
  for(int Icol=0;Icol<nx;Icol++)
    {
      for(int j=0;j<nx;j++)
	V[j][Icol]=EigenVec[j][index[Icol]];
      DiagMatrix[Icol][Icol]=X[index[Icol]];
    }

  return;
}

template<typename T>
int 
Matrix<T>::Diagonalise(Matrix<T>& EigenVec,Matrix<T>& DiagMatrix) const
  /*!
    Attempt to diagonalise the matrix IF symmetric
    \param EigenVec :: (output) the Eigenvectors matrix 
    \param DiagMatrix  :: the diagonal matrix of eigenvalues 
    \returns :: 1  on success 0 on failure
  */
{
  double theta,tresh,tanAngle,cosAngle,sinAngle;
  if(nx!=ny || nx<1)
    {
      std::cerr<<"Matrix not square"<<std::endl;
      return 0;
    }
  for(int i=0;i<nx;i++)
    for(int j=i+1;j<nx;j++)
      if (fabs(V[i][j]-V[j][i])>1e-6)
        {
	  std::cerr<<"Matrix not symmetric"<<std::endl;
	  std::cerr<< (*this);
	  return 0;
	}
    
  Matrix<T> A(*this);
  //Make V an identity matrix
  EigenVec.setMem(nx,nx);
  EigenVec.identityMatrix();
  DiagMatrix.setMem(nx,nx);
  DiagMatrix.zeroMatrix();

  std::vector<double> Diag(nx);
  std::vector<double> B(nx);
  std::vector<double> ZeroComp(nx);
  //set b and d to the diagonal elements o A
  for(int i=0;i<nx;i++)
    {
      Diag[i]=B[i]= A.V[i][i];
      ZeroComp[i]=0;
    }

  int iteration=0;
  double sm; 
  for(int i=0;i<100;i++)        //max 50 iterations
    {
      sm=0.0;           // sum of off-diagonal terms
      for(int ip=0;ip<nx-1;ip++)
	for(int iq=ip+1;iq<nx;iq++)
	  sm+=fabs(A.V[ip][iq]);

      if (sm==0.0)           // Nothing to do return...
	{
	  // Make OUTPUT -- D + A
	  // sort Output::
	  std::vector<int> index;
//	  indexSort(Diag,index);
//	  for(int ii=0;ii<nx;ii++)
//	    {
//	      DiagMatrix.V[ii][ii]=static_cast<T>(Diag[index[ii]]);
//	      EigenVector

	  for(int ix=0;ix<nx;ix++)
	    DiagMatrix.V[ix][ix]=static_cast<T>(Diag[ix]);
	  return 1;
	}

      // Threshold large for first 5 sweeps
      tresh= (i<6) ? 0.2*sm/(nx*nx) : 0.0;

      for(int ip=0;ip<nx-1;ip++)
	{
	  for(int iq=ip+1;iq<nx;iq++)
	    {
	      double g=100.0*fabs(A.V[ip][iq]);
	      // After 4 sweeps skip if off diagonal small
	      if (i>6 && 
		  static_cast<float>(fabs(Diag[ip]+g))==static_cast<float>(fabs(Diag[ip])) &&
		  static_cast<float>(fabs(Diag[iq]+g))==static_cast<float>(fabs(Diag[iq])))
		A.V[ip][iq]=0;

	      else if (fabs(A.V[ip][iq])>tresh)
		{
		  double h=Diag[iq]-Diag[ip];
		  if (static_cast<float>((fabs(h)+g)) == static_cast<float>(fabs(h)))
		    tanAngle=A.V[ip][iq]/h;      // tanAngle=1/(2theta)
		  else
		    {
		      theta=0.5*h/A.V[ip][iq];
		      tanAngle=1.0/(fabs(theta)+sqrt(1.0+theta*theta));
		      if (theta<0.0)
			tanAngle = -tanAngle;
		    }
		  cosAngle=1.0/sqrt(1+tanAngle*tanAngle);
		  sinAngle=tanAngle*cosAngle;
		  double tau=sinAngle/(1.0+cosAngle);
		  h=tanAngle*A.V[ip][iq];
		  ZeroComp[ip] -= h;
		  ZeroComp[iq] += h;
		  Diag[ip] -= h;
		  Diag[iq] += h;
		  A.V[ip][iq]=0;
		  // Rotations 0<j<p
		  for(int j=0;j<ip;j++)
		    A.rotate(tau,sinAngle,j,ip,j,iq);
		  for(int j=ip+1;j<iq;j++)
		    A.rotate(tau,sinAngle,ip,j,j,iq);
		  for(int j=iq+1;j<nx;j++)
		    A.rotate(tau,sinAngle,ip,j,iq,j);
		  for(int j=0;j<nx;j++)
		    EigenVec.rotate(tau,sinAngle,j,ip,j,iq);
		  iteration++;
		}
	    }
	}   
      for(int j=0;j<nx;j++)
	{
	  B[j]+=ZeroComp[j];
	  Diag[j]=B[j];
	  ZeroComp[j]=0.0;
	}
	    
    }
  std::cerr<<"Error :: Iterations are a problem"<<std::endl;
  return 0;
}

template<typename T>
void
Matrix<T>::print() const
  /*! 
    Simple print out routine 
   */
{
  write(std::cout,10);
  return;
}

template<typename T>
void
Matrix<T>::write(std::ostream& Fh,const int blockCnt) const
  /*!
    Write out function for blocks of 10 Columns 
    \param Fh :: file stream for output
    \param blockCnt :: number of columns per line (0 == full)
  */
{

  std::ios::fmtflags oldFlags=Fh.flags();
  Fh.setf(std::ios::floatfield,std::ios::scientific);
  const int blockNumber((blockCnt>0) ? blockCnt : ny);  
  int BCnt(0);

  do
    {
      const int ACnt=BCnt;
      BCnt+=blockNumber;
      if (BCnt>ny)
	BCnt=ny;

      if (ACnt)
	Fh<<" ----- "<<ACnt<<" "<<BCnt<<" ------ "<<std::endl;
      for(int i=0;i<nx;i++)
        {
	  for(int j=ACnt;j<BCnt;j++)
	    Fh<<std::setw(10)<<V[i][j]<<"  ";
	  Fh<<std::endl;
	}
    } while(BCnt<ny);

  Fh.flags(oldFlags);
  return;
}

template<typename T>
std::string
Matrix<T>::str() const
  /*!
    Convert the matrix into a simple linear string expression 
    \returns String value of output
  */
{
  std::ostringstream cx;
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      {
	cx<<std::setprecision(6)<<V[i][j]<<" ";
      }
  return cx.str();
}

template class Matrix<double>;
template class Matrix<int>;

};
