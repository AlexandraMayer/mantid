#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_complex_math.h>

namespace Mantid {
namespace Geometry {
SymmetryElement::SymmetryElement() : m_hmSymbol() {}

void SymmetryElement::setHMSymbol(const std::string &symbol) {
  m_hmSymbol = symbol;
}

SymmetryElementIdentity::SymmetryElementIdentity() : SymmetryElement() {}

void SymmetryElementIdentity::init(const SymmetryOperation &operation) {

  if (operation.order() != 1) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to construct SymmetryElement 1.");
  }

  setHMSymbol("1");
}

SymmetryElementInversion::SymmetryElementInversion()
    : SymmetryElement(), m_inversionPoint() {}

void SymmetryElementInversion::init(const SymmetryOperation &operation) {
  SymmetryOperation op =
      SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z");

  if (operation.matrix() != op.matrix()) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to initialize SymmetryElement -1.");
  }

  setHMSymbol("-1");
  setInversionPoint(operation.vector() / 2);
}

void SymmetryElementInversion::setInversionPoint(const V3R &inversionPoint) {
  m_inversionPoint = inversionPoint;
}

SymmetryElementWithAxis::SymmetryElementWithAxis() : SymmetryElement() {}

void SymmetryElementWithAxis::setAxis(const V3R &axis) {
  if (axis == V3R(0, 0, 0)) {
    throw std::invalid_argument("Axis cannot be 0.");
  }

  m_axis = axis;
}

V3R SymmetryElementWithAxis::determineTranslation(
    const SymmetryOperation &operation) const {

  Kernel::IntMatrix translationMatrix(3, 3, false);

  for (size_t i = 0; i < operation.order(); ++i) {
    translationMatrix += (operation ^ i).matrix();
  }

  return (translationMatrix * operation.vector()) *
         RationalNumber(1, static_cast<int>(operation.order()));
}

gsl_matrix *getGSLMatrix(const Kernel::IntMatrix &matrix) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(matrix.numRows(), matrix.numCols());

  for (size_t r = 0; r < matrix.numRows(); ++r) {
    for (size_t c = 0; c < matrix.numCols(); ++c) {
      gsl_matrix_set(gslMatrix, r, c, static_cast<double>(matrix[r][c]));
    }
  }

  return gslMatrix;
}

gsl_matrix *getGSLIdentityMatrix(size_t rows, size_t cols) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(rows, cols);

  gsl_matrix_set_identity(gslMatrix);

  return gslMatrix;
}

V3R
SymmetryElementWithAxis::determineAxis(const Kernel::IntMatrix &matrix) const {
  gsl_matrix *eigenMatrix = getGSLMatrix(matrix);
  gsl_matrix *identityMatrix =
      getGSLIdentityMatrix(matrix.numRows(), matrix.numCols());

  gsl_eigen_genv_workspace *eigenWs = gsl_eigen_genv_alloc(matrix.numRows());

  gsl_vector_complex *alpha = gsl_vector_complex_alloc(3);
  gsl_vector *beta = gsl_vector_alloc(3);
  gsl_matrix_complex *eigenVectors = gsl_matrix_complex_alloc(3, 3);

  gsl_eigen_genv(eigenMatrix, identityMatrix, alpha, beta, eigenVectors,
                 eigenWs);

  double determinant = matrix.determinant();

  std::vector<double> eigenVector(3, 0.0);

  for (size_t i = 0; i < matrix.numCols(); ++i) {
    double eigenValue = GSL_REAL(gsl_complex_div_real(
        gsl_vector_complex_get(alpha, i), gsl_vector_get(beta, i)));

    if (fabs(eigenValue - determinant) < 1e-9) {
      for (size_t j = 0; j < matrix.numRows(); ++j) {
        double element = GSL_REAL(gsl_matrix_complex_get(eigenVectors, j, i));

        eigenVector[j] = element;
      }
    }
  }

  gsl_matrix_free(eigenMatrix);
  gsl_matrix_free(identityMatrix);
  gsl_eigen_genv_free(eigenWs);
  gsl_vector_complex_free(alpha);
  gsl_vector_free(beta);
  gsl_matrix_complex_free(eigenVectors);

  double min = 1.0;
  for (size_t i = 0; i < eigenVector.size(); ++i) {
    double absoluteValue = fabs(eigenVector[i]);
    if (absoluteValue != 0.0 &&
        (eigenVector[i] < min && (absoluteValue - fabs(min)) < 1e-9)) {
      min = eigenVector[i];
    }
  }

  V3R axis;
  for (size_t i = 0; i < eigenVector.size(); ++i) {
    axis[i] = static_cast<int>(round(eigenVector[i] / min));
  }

  return axis;
}

SymmetryElementRotation::SymmetryElementRotation()
    : SymmetryElementWithAxis() {}

void SymmetryElementRotation::init(const SymmetryOperation &operation) {
  UNUSED_ARG(operation);
}

SymmetryElementMirror::SymmetryElementMirror() : SymmetryElementWithAxis() {}

void SymmetryElementMirror::init(const SymmetryOperation &operation) {
  UNUSED_ARG(operation);
}

} // namespace Geometry
} // namespace Mantid
