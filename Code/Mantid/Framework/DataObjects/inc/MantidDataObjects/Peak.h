#ifndef MANTID_DATAOBJECTS_PEAK_H_
#define MANTID_DATAOBJECTS_PEAK_H_

#include "MantidAPI/IPeak.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"


namespace Mantid
{
namespace DataObjects
{

  /** Structure describing a single-crystal peak
   * 
   * @author Janik Zikovsky
   * @date 2011-04-15 13:24:07.963491
   */
  class DLLExport Peak: public API::IPeak
  {
  public:
    /// Allow PeakColumn class to directly access members.
    friend class PeakColumn;

    Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, Mantid::Geometry::V3D QSampleFrame, double detectorDistance=1.0);
    Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, Mantid::Geometry::V3D QSampleFrame, Mantid::Geometry::Matrix<double> goniometer, double detectorDistance=1.0);
    Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength);
    Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Geometry::V3D HKL);
    Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Geometry::V3D HKL, Mantid::Geometry::Matrix<double> goniometer);

    // Copy constructor is compiler-provided.
    //    Peak(const Peak & other);
    Peak(const API::IPeak & ipeak);
    virtual ~Peak();

    void setInstrument(Mantid::Geometry::IInstrument_const_sptr inst);

    int getDetectorID() const;
    void setDetectorID(int m_DetectorID);
    Mantid::Geometry::IDetector_const_sptr getDetector() const;
    Mantid::Geometry::IInstrument_const_sptr getInstrument() const;

    int getRunNumber() const;
    void setRunNumber(int m_RunNumber);

    double getH() const;
    double getK() const;
    double getL() const;
    Mantid::Geometry::V3D getHKL();
    void setH(double m_H);
    void setK(double m_K);
    void setL(double m_L);
    void setHKL(double H, double K, double L);
    void setHKL(Mantid::Geometry::V3D HKL);

    Mantid::Geometry::V3D getQLabFrame() const;
    Mantid::Geometry::V3D getQSampleFrame() const;

    void setQSampleFrame(Mantid::Geometry::V3D QSampleFrame, double detectorDistance=1.0);
    void setQLabFrame(Mantid::Geometry::V3D QLabFrame, double detectorDistance=1.0);

    void setWavelength(double wavelength);
    double getWavelength() const;
    double getDSpacing() const;
    double getTOF() const;

    double getInitialEnergy() const;
    double getFinalEnergy() const;
    void setInitialEnergy(double m_InitialEnergy);
    void setFinalEnergy(double m_FinalEnergy);

    double getIntensity() const;
    double getSigmaIntensity() const;

    void setIntensity(double m_Intensity);
    void setSigmaIntensity(double m_SigmaIntensity);

    double getBinCount() const;
    void setBinCount(double m_BinCount);

    Mantid::Geometry::Matrix<double> getGoniometerMatrix() const;
    void setGoniometerMatrix(Mantid::Geometry::Matrix<double> m_GoniometerMatrix);

    std::string getBankName() const;
    int getRow() const;
    int getCol() const;

    Mantid::Geometry::V3D getDetPos() const;
    double getL1() const;
    double getL2() const;

  protected:
    /// Shared pointer to the instrument (for calculating some values )
    Mantid::Geometry::IInstrument_const_sptr m_inst;

    /// Detector pointed to
    Mantid::Geometry::IDetector_const_sptr m_det;

    /// Name of the parent bank
    std::string m_BankName;

    /// ID of the detector
    int m_DetectorID;

    /// H of the peak
    double m_H;

    /// K of the peak
    double m_K;

    /// L of the peak
    double m_L;

    /// Integrated peak intensity
    double m_Intensity;

    /// Error (sigma) on peak intensity
    double m_SigmaIntensity;

    /// Count in the bin at the peak
    double m_BinCount;

    /// Initial energy of neutrons at the peak
    double m_InitialEnergy;

    /// Final energy of the neutrons at peak (normally same as m_InitialEnergy)
    double m_FinalEnergy;

    /// Orientation matrix of the goniometer angles.
    Mantid::Geometry::Matrix<double> m_GoniometerMatrix;

    /// Inverse of the goniometer rotation matrix; used to go from Q in lab frame to Q in sample frame
    Mantid::Geometry::Matrix<double> m_InverseGoniometerMatrix;

    /// Originating run number for this peak
    int m_RunNumber;

    /// Cached row in the detector
    int m_Row;

    /// Cached column in the detector
    int m_Col;

    /// Cached source position
    Mantid::Geometry::V3D sourcePos;
    /// Cached sample position
    Mantid::Geometry::V3D samplePos;
    /// Cached detector position
    Mantid::Geometry::V3D detPos;

  };


} // namespace Mantid
} // namespace DataObjects

#endif  /* MANTID_DATAOBJECTS_PEAK_H_ */
