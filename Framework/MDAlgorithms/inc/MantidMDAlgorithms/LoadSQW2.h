#ifndef MANTID_MDALGORITHMS_LOADSQW2_H_
#define MANTID_MDALGORITHMS_LOADSQW2_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/BinaryStreamReader.h"

#include <fstream>

namespace Mantid {

// Forward declarations
namespace API {
class ExperimentInfo;
class Progress;
}

namespace MDAlgorithms {

/**
  *
  * Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  * National Laboratory & European Spallation Source
  *
  * This file is part of Mantid.

  * Mantid is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 3 of the License, or
  * (at your option) any later version.

  * Mantid is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.

  * File change history is stored at: <https://github.com/mantidproject/mantid>
  * Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport LoadSQW2 : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadSQW2();
  ~LoadSQW2();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Local typedef for
  typedef DataObjects::MDEventWorkspace<DataObjects::MDEvent<4>, 4>
      SQWWorkspace;

  /// Define the sections of the file
  struct SQWHeader {
    int32_t nfiles;
  };

  void init();
  void exec();
  void cacheInputs();
  void initFileReader();
  SQWHeader readMainHeader();
  void createOutputWorkspace();
  void readAllSPEHeadersToWorkspace(const int32_t nfiles);
  boost::shared_ptr<API::ExperimentInfo> readSingleSPEHeader();
  Kernel::DblMatrix
  calculateOutputTransform(const Kernel::DblMatrix &gonR,
                           const Geometry::OrientedLattice &lattice);
  void skipDetectorSection();
  void readDataSection();
  void skipDataSectionMetadata();
  void readSQWDimensions();
  Geometry::IMDDimension_sptr createQDimension(size_t index, float dimMin,
                                               float dimMax, size_t nbins,
                                               const Kernel::DblMatrix &bmat);
  void transformLimitsToOutputFrame(Kernel::Matrix<float> &urange);
  Geometry::IMDDimension_sptr createEnDimension(float umin, float umax,
                                                size_t nbins);
  void setupBoxController();
  void setupFileBackend(std::string filebackPath);
  void readPixelData();
  void splitAllBoxes();
  void warnIfMemoryInsufficient(int64_t npixtot);
  void addEventFromBuffer(const float *pixel);
  void toOutputFrame(const uint16_t runIndex, float &u1, float &u2, float &u3);
  void finalize();

  std::unique_ptr<std::ifstream> m_file;
  std::unique_ptr<Kernel::BinaryStreamReader> m_reader;
  boost::shared_ptr<SQWWorkspace> m_outputWS;
  std::vector<Kernel::DblMatrix> m_outputTransforms;
  std::unique_ptr<API::Progress> m_progress;
  std::string m_outputFrame;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADSQW2_H_ */
