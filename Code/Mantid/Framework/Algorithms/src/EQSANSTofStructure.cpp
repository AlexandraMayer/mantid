//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EQSANSTofStructure.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSTofStructure)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSTofStructure::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
      "Workspace to apply the TOF correction to");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "Workspace to store the corrected data in");
  declareProperty("FrameSkipping", false, "Frame skipping option" );
}

void EQSANSTofStructure::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const bool frame_skipping = getProperty("FrameSkipping");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = inputWS->getNumberHistograms();

  Progress progress(this,0.0,1.0,numHists);

  // Get TOF offset
  double frame_tof0 = getTofOffset(inputWS, frame_skipping);

  // Calculate the frame width
  const double frequency = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData("frequency"))->getStatistics().mean;
  double tof_frame_width = 1.0e6/frequency;
  double tmp_frame_width = frame_skipping ? tof_frame_width * 2.0 : tof_frame_width;

  double frame_offset=0.0;
  if (frame_tof0 >= tmp_frame_width) frame_offset = tmp_frame_width * ( (int)( frame_tof0/tmp_frame_width ) );

  // Find the new binning first
  //dataX = mantid.getMatrixWorkspace(input_ws).readX(0).copy()
  const MantidVec& XIn = inputWS->readX(0);
  const int nTOF = XIn.size();

  // Loop through each bin
  int cutoff = 0;
  double threshold = frame_tof0-frame_offset;
  for (int i=0; i<nTOF; i++)
  {
      if (XIn[i] < threshold) cutoff = i;
  }

  g_log.information() << "Cutoff " << cutoff << " at " << threshold << std::endl;
  g_log.information() << "Frame offset " << frame_offset << std::endl;
  g_log.information() << "Tmp width " << tmp_frame_width << std::endl;

  // Since we are swapping the low-TOF and high-TOF regions around the cutoff value,
  // there is the potential for having an overlap between the two regions. We exclude
  // the region beyond a single frame by considering only the first 1/60 sec of the
  // TOF histogram. (Bins 1 to 1666, as opposed to 1 to 2000)
  int tof_bin_range = (int)(100000.0/frequency);
  //int tof_bin_range = nTOF;

  g_log.information() << "Low TOFs: old = [" << (cutoff+1) << ", " << (tof_bin_range-2) << "]  ->  new = [0, " << (tof_bin_range-3-cutoff) << "]" << std::endl;
  g_log.information() << "High bin boundary of the Low TOFs: old = " << tof_bin_range-1 << "; new = " << (tof_bin_range-2-cutoff) << std::endl;
  g_log.information() << "High TOFs: old = [0, " << (cutoff-1) << "]  ->  new = [" << (tof_bin_range-1-cutoff) << ", " << (tof_bin_range-2) << "]" << std::endl;
  g_log.information() << "Overlap: new = [" << (tof_bin_range-1) << ", " << (nTOF-2) << "]" << std::endl;

  // Loop through the spectra and apply correction
  for (int ispec = 0; ispec < numHists; ispec++)
  {
    // Keep a copy of the input data since we may end up overwriting it
    // if the input workspace is equal to the output workspace.
    // This is necessary since we are shuffling around the TOF bins.
    MantidVec YCopy = MantidVec(inputWS->readY(ispec));
    MantidVec& YIn = YCopy;
    MantidVec ECopy = MantidVec(inputWS->readE(ispec));
    MantidVec& EIn = ECopy;

    MantidVec& XOut = outputWS->dataX(ispec);
    MantidVec& YOut = outputWS->dataY(ispec);
    MantidVec& EOut = outputWS->dataE(ispec);

    // Move up the low TOFs
    for (int i=0; i<cutoff; i++)
    {
      XOut[i+tof_bin_range-1-cutoff] = XIn[i] + frame_offset + tmp_frame_width;
      YOut[i+tof_bin_range-1-cutoff] = YIn[i];
      EOut[i+tof_bin_range-1-cutoff] = EIn[i];
    }

    // Get rid of extra bins
    for (int i=tof_bin_range-1; i<nTOF-1; i++)
    {
      XOut[i] = XOut[i-1]+10.0;
      YOut[i] = 0.0;
      EOut[i] = 0.0;
    }
    XOut[nTOF-1] = XOut[nTOF-2]+10.0;

    // Move down the high TOFs
    for (int i=cutoff+1; i<tof_bin_range-1; i++)
    {
      XOut[i-cutoff-1] = XIn[i] + frame_offset;
      YOut[i-cutoff-1] = YIn[i];
      EOut[i-cutoff-1] = EIn[i];
    }
    // Don't forget the low boundary
    XOut[tof_bin_range-2-cutoff] = XIn[tof_bin_range-1] + frame_offset;

    // Zero out the cutoff bin, which no longer makes sense
    YOut[tof_bin_range-2-cutoff] = 0.0;
    EOut[tof_bin_range-2-cutoff] = 0.0;

    progress.report();
  }
}

double EQSANSTofStructure::getTofOffset(MatrixWorkspace_const_sptr inputWS, bool frame_skipping)
{
  //# Storage for chopper information read from the logs
  double chopper_set_phase[4] = {0,0,0,0};
  double chopper_speed[4] = {0,0,0,0};
  double chopper_actual_phase[4] = {0,0,0,0};
  double chopper_wl_1[4] = {0,0,0,0};
  double chopper_wl_2[4] = {0,0,0,0};
  double frame_wl_1 = 0;
  double frame_srcpulse_wl_1 = 0;
  double frame_wl_2 = 0;
  double chopper_srcpulse_wl_1[4] = {0,0,0,0};
  double chopper_frameskip_wl_1[4] = {0,0,0,0};
  double chopper_frameskip_wl_2[4] = {0,0,0,0};
  double chopper_frameskip_srcpulse_wl_1[4] = {0,0,0,0};

  double tof_frame_width = 1.0e6/REP_RATE;

  double tmp_frame_width = tof_frame_width;
  if (frame_skipping) tmp_frame_width *= 2.0;

  // Choice of parameter set
  int m_set = 0;
  if (frame_skipping) m_set = 1;

  bool first = true;
  bool first_skip = true;
  double frameskip_wl_1 = 0;
  double frameskip_srcpulse_wl_1 = 0;
  double frameskip_wl_2 = 0;


  for( int i=0; i<4; i++)
  {
    // Read chopper information
    std::ostringstream phase_str;
    phase_str << "Phase" << i+1;
    chopper_set_phase[i] = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData(phase_str.str()))->getStatistics().mean;
    std::ostringstream speed_str;
    speed_str << "Speed" << i+1;
    chopper_speed[i] = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData(speed_str.str()))->getStatistics().mean;

    // Only process choppers with non-zero speed
    if (chopper_speed[i]<=0) continue;

    chopper_actual_phase[i] = chopper_set_phase[i] - CHOPPER_PHASE_OFFSET[m_set][i];

    while (chopper_actual_phase[i]<0) chopper_actual_phase[i] += tmp_frame_width;

    double x1 = ( chopper_actual_phase[i]- ( tmp_frame_width * 0.5*CHOPPER_ANGLE[i]/360. ) ); // opening edge
    double x2 = ( chopper_actual_phase[i]+ ( tmp_frame_width * 0.5*CHOPPER_ANGLE[i]/360. ) ); // closing edge
    if (!frame_skipping) // not skipping
    {
      while (x1<0)
      {
        x1+=tmp_frame_width;
        x2+=tmp_frame_width;
      }
    }

    if (x1>0)
    {
        chopper_wl_1[i]= 3.9560346 * x1 / CHOPPER_LOCATION[i];
        chopper_srcpulse_wl_1[i]= 3.9560346 * ( x1-chopper_wl_1[i]*PULSEWIDTH ) / CHOPPER_LOCATION[i];
    }
    else chopper_wl_1[i]=chopper_srcpulse_wl_1[i]=0.;

    if (x2>0) chopper_wl_2[i]= 3.9560346 * x2 / CHOPPER_LOCATION[i];
    else chopper_wl_2[i]=0.;

    if (first)
    {
        frame_wl_1=chopper_wl_1[i];
        frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i];
        frame_wl_2=chopper_wl_2[i];
        first=false;
    }
    else
    {
        if (frame_skipping && i==2) // ignore chopper 1 and 2 forthe shortest wl.
        {
            frame_wl_1=chopper_wl_1[i];
            frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i];
        }
        if (frame_wl_1<chopper_wl_1[i]) frame_wl_1=chopper_wl_1[i];
        if (frame_wl_2>chopper_wl_2[i]) frame_wl_2=chopper_wl_2[i];
        if (frame_srcpulse_wl_1<chopper_srcpulse_wl_1[i]) frame_srcpulse_wl_1=chopper_srcpulse_wl_1[i];
    }

    if (frame_skipping)
    {
        if (x1>0)
        {
            x1 += tof_frame_width;    // skipped pulse
            chopper_frameskip_wl_1[i]= 3.9560346 * x1 / CHOPPER_LOCATION[i];
            chopper_frameskip_srcpulse_wl_1[i]= 3.9560346 * ( x1-chopper_wl_1[i]*PULSEWIDTH ) / CHOPPER_LOCATION[i];
        }
        else chopper_wl_1[i]=chopper_srcpulse_wl_1[i]=0.;

        if (x2>0)
        {
            x2 += tof_frame_width;
            chopper_frameskip_wl_2[i]= 3.9560346 * x2 / CHOPPER_LOCATION[i];
        }
        else chopper_wl_2[i]=0.;

        if (i<2 && chopper_frameskip_wl_1[i] > chopper_frameskip_wl_2[i]) continue;

        if (first_skip)
        {
            frameskip_wl_1=chopper_frameskip_wl_1[i];
            frameskip_srcpulse_wl_1=chopper_frameskip_srcpulse_wl_1[i];
            frameskip_wl_2=chopper_frameskip_wl_2[i];
            first_skip=false;
        }
        else
        {
            if (i==2)   // ignore chopper 1 and 2 forthe longest wl.
              frameskip_wl_2=chopper_frameskip_wl_2[i];

            if (chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] && frameskip_wl_1<chopper_frameskip_wl_1[i])
                frameskip_wl_1=chopper_frameskip_wl_1[i];

            if (chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] && frameskip_srcpulse_wl_1<chopper_frameskip_srcpulse_wl_1[i])
                frameskip_srcpulse_wl_1=chopper_frameskip_srcpulse_wl_1[i];

            if (frameskip_wl_2>chopper_frameskip_wl_2[i]) frameskip_wl_2=chopper_frameskip_wl_2[i];
        }
    }
  }

  if (frame_wl_1>=frame_wl_2)    // too many frames later. So figure it out
  {
    double n_frame[4] = {0,0,0,0};
    double c_wl_1[4] = {0,0,0,0};
    double c_wl_2[4] = {0,0,0,0};
    bool passed=false;

    while (!passed && n_frame[0]<99)
    {
        frame_wl_1=c_wl_1[0] = chopper_wl_1[0] + 3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];
        frame_wl_2=c_wl_2[0] = chopper_wl_2[0] + 3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];

        for ( int i=0; i<4; i++ )
        {
            n_frame[i] = n_frame[i-1] - 1;
            passed=false;

            while (n_frame[i] - n_frame[i-1] < 10)
            {
                n_frame[i] += 1;
                c_wl_1[i] = chopper_wl_1[i] + 3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];
                c_wl_2[i] = chopper_wl_2[i] + 3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];

                if (frame_wl_1 < c_wl_2[i] and frame_wl_2> c_wl_1[i])
                {
                    passed=true;
                    break;
                }
                if (frame_wl_2 < c_wl_1[i])
                    break; // over shot
            }

            if (!passed)
            {
                n_frame[0] += 1;
                break;
            }
            else
            {
                if (frame_wl_1<c_wl_1[i]) frame_wl_1=c_wl_1[i];
                if (frame_wl_2>c_wl_2[i]) frame_wl_2=c_wl_2[i];
            }
        }
    }

    if (frame_wl_2 > frame_wl_1)
    {
        int n = 3;
        if (c_wl_1[2] > c_wl_1[3]) n = 2;

        frame_srcpulse_wl_1=c_wl_1[n] - 3.9560346 * c_wl_1[n] * PULSEWIDTH / CHOPPER_LOCATION[n];

        for ( int i=0; i<4; i++ )
        {
            chopper_wl_1[i] = c_wl_1[i];
            chopper_wl_2[i] = c_wl_2[i];
            if (frame_skipping)
            {
                chopper_frameskip_wl_1[i] = c_wl_1[i] +  3.9560346 * 2.* tof_frame_width / CHOPPER_LOCATION[i];
                chopper_frameskip_wl_2[i] = c_wl_2[i] +  3.9560346 * 2.* tof_frame_width / CHOPPER_LOCATION[i];
                if (i==0)
                {
                    frameskip_wl_1 = chopper_frameskip_wl_1[i];
                    frameskip_wl_2 = chopper_frameskip_wl_2[i];
                }
                else
                {
                    if (frameskip_wl_1<chopper_frameskip_wl_1[i]) frameskip_wl_1=chopper_frameskip_wl_1[i];
                    if (frameskip_wl_2>chopper_frameskip_wl_2[i]) frameskip_wl_2=chopper_frameskip_wl_2[i];
                }
            }
        }
    }
    else frame_srcpulse_wl_1=0.0;
  }
  // Get source and detector locations
  //TODO: get component name as parameter
  double source_z = inputWS->getInstrument()->getSource()->getPos().Z();
  double detector_z = inputWS->getInstrument()->getComponentByName("detector1")->getPos().Z();

  double source_to_detector = (detector_z - source_z)*1000.0;
  double frame_tof0 = frame_srcpulse_wl_1 / 3.9560346 * source_to_detector;

  g_log.information() << "TOF offset = " << frame_tof0 << " microseconds" << std::endl;
  g_log.information() << "Band defined by T1-T4 " << frame_wl_1 << " " << frame_wl_2 << std::endl;
  g_log.information() << "Chopper    Actual Phase    Lambda1    Lambda2" << std::endl;
  for ( int i=0; i<4; i++)
      g_log.information() << i << "    " << chopper_actual_phase[i] << "  " << chopper_wl_1[i] << "  " << chopper_wl_2[i] << std::endl;
  return frame_tof0;
}

} // namespace Algorithms
} // namespace Mantid

