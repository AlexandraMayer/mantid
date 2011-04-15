#ifndef Geometry_Matrix_h
#define Geometry_Matrix_h

#include "MantidKernel/System.h"
#include <vector>
namespace Mantid
{

  /**
  \brief Classes for geometric handlers
  \version 1.0
  \author S. Ansell
  \date February 2006

  */
  namespace Geometry
  {

     class V3D;
    /**
    \class Matrix
    \brief Numerical Matrix class
    \version 1.0
    \author S. Ansell
    \date August 2005

    Holds a matrix of variable type
    and size. Should work for real and
    complex objects. Carries out eigenvalue
    and inversion if the matrix is square
    */

    template<typename T>
    class DLLExport Matrix
    {
    private:

      int nx;      ///< Number of rows    (x coordinate)
      int ny;      ///< Number of columns (y coordinate)

      T** V;                      ///< Raw data

      void deleteMem();           ///< Helper function to delete memory
      void lubcmp(int*,int&);     ///< starts inversion process
      void lubksb(int const*,double*);
      void rotate(double const,double const,
        int const,int const,int const,int const);

    public:

      Matrix(int const =0,int const  =0, bool const makeIdentity=false);
      Matrix(const std::vector<T>&,const std::vector<T>&);
      Matrix(const Matrix<T>&);
      Matrix<T>& operator=(const Matrix<T>&);
      ~Matrix();

      /// const Array accessor
      const T* operator[](const int A) const { return V[A]; }
      /// Array accessor
      T* operator[](const int A) { return V[A]; }

      Matrix<T>& operator+=(const Matrix<T>&);       ///< Basic addition operator
      Matrix<T> operator+(const Matrix<T>&);         ///< Basic addition operator

      Matrix<T>& operator-=(const Matrix<T>&);       ///< Basic subtraction operator
      Matrix<T> operator-(const Matrix<T>&);         ///< Basic subtraction operator

      Matrix<T> operator*(const Matrix<T>&) const;    ///< Basic matrix multiply
      std::vector<T> operator*(const std::vector<T>&) const; ///< Multiply M*Vec
      V3D operator*(const V3D&) const; ///< Multiply M*Vec
      Matrix<T> operator*(const T&) const;              ///< Multiply by constant

      Matrix<T>& operator*=(const Matrix<T>&);            ///< Basic matrix multipy
      Matrix<T>& operator*=(const T&);                 ///< Multiply by constant
      Matrix<T>& operator/=(const T&);                 ///< Divide by constant

      bool operator!=(const Matrix<T>&) const;
      bool operator==(const Matrix<T>&) const;
      T item(const int a,const int b) const { return V[a][b]; }   ///< disallows access

      void print() const;
      void write(std::ostream&,int const =0) const;
      std::string str() const;

	  // returns this matrix in 1D vector representation
	  std::vector<T> get_vector()const;
	  // explicit conversion into the vector
	  operator std::vector<T>()const{std::vector<T> tmp=this->get_vector(); return tmp;}
	  //
	  void setColumn(int nCol,const std::vector<T> &newColumn);
	  void setRow(int nRow,const std::vector<T> &newRow);
      void zeroMatrix();      ///< Set the matrix to zero
      void identityMatrix();
      void normVert();         ///< Vertical normalisation
      T Trace() const;         ///< Trace of the matrix

      std::vector<T> Diagonal() const;                  ///< Returns a vector of the diagonal
      Matrix<T> fDiagonal(const std::vector<T>&) const;    ///< Forward multiply  D*this
      Matrix<T> bDiagonal(const std::vector<T>&) const;    ///< Backward multiply this*D

      void setMem(int const,int const);

	  
      /// Access matrix sizes
      std::pair<int,int> size() const { return std::pair<int,int>(nx,ny); }

      /// Return the number of rows in the matrix
      size_t numRows() const { return static_cast<size_t>(nx); }

      /// Return the number of columns in the matrix
      size_t numCols() const { return static_cast<size_t>(ny); }

      /// Return the largest matrix size
      int Ssize() const { return (nx>ny) ? ny : nx; }

      void swapRows(int const,int const);        ///< Swap rows (first V index)
      void swapCols(int const,int const);        ///< Swap cols (second V index)

      T Invert();                           ///< LU inversion routine
      std::vector<T> Faddeev(Matrix<T>&);      ///< Polynomanal and inversion by Faddeev method.
      void averSymmetric();                 ///< make Matrix symmetric
      int Diagonalise(Matrix<T>&,Matrix<T>&) const;  ///< (only Symmetric matrix)
      void sortEigen(Matrix<T>&);                    ///< Sort eigenvectors
      Matrix<T> Tprime() const;                      ///< Transpose the matrix
      Matrix<T>& Transpose();                        ///< Transpose the matrix

      T factor();                       ///< Calculate the factor
      T determinant() const;            ///< Calculate the determinant

      int GaussJordan(Matrix<T>&);      ///< Create a Gauss-Jordan Invertion
      T compSum() const;

    };

    template<typename T>
    DLLExport std::ostream&
      operator<<(std::ostream&,const Geometry::Matrix<T>&);

    typedef Mantid::Geometry::Matrix<double> MantidMat;

  }  // NAMESPACE Geometry


}  // NAMESPACE Mantid

// template<typename X>
// std::ostream& operator<<(std::ostream&,const Geometry::Matrix<X>&);

#endif














