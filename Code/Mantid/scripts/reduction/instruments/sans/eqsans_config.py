"""
    Parser for EQSANS configuration file
"""
import os
import re
from reduction.instruments.sans.sns_instrument import EQSANS

class EQSANSConfig(object):
    
    def __init__(self, file_path=None):
        
        if file_path is not None and not os.path.isfile(file_path):
            raise RuntimeError, "Provided path is not valid: %s" % str(file_path)
        
        ## File path
        if file_path is not None and not os.path.isfile(file_path):
            raise RuntimeError, "Provided path is not valid: %s" % str(file_path)
        self._file_path = file_path
        
        self.reset()
        if file_path is not None:
            self._process_file()
    
    def reset(self):
        """
            Reset the data members
        """
        ## Rectangular masks
        self.rectangular_masks = []
        
        ## TOF band to discard at the beginning of the frame
        self.low_TOF_cut = 0
        
        ## TOF band to discard at the end of the frame
        self.high_TOF_cut = 0
        
        # Beam center
        self.center_x = None
        self.center_y = None
        
        # Sample-detector distance
        self.sample_detector_dist = 0
        
        # Prompt pulse width
        self.prompt_pulse_width = 20
    
    def _process_file(self):
        """
            Read and process the configuration file
        """
        self.reset()
        f = open(self._file_path)
        for line in f.readlines():
            
            # Looking for rectangular mask
            # Rectangular mask         = 7, 0; 7, 255
            #FIXME: Elliptical masks are treat as rectangular masks until implemented
            if not line.strip().startswith("#") and  (line.lower().find("rectangular mask")>=0 \
                or line.find("Elliptical mask")>=0):
                coord = re.search("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)[ ]*[ ;,][ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)", line)
                if coord is not None:
                    try:
                        x1 = int(coord.group(1))
                        y1 = int(coord.group(2))
                        x2 = int(coord.group(3))
                        y2 = int(coord.group(4))
                        self.rectangular_masks.append([x1,x2,y1,y2])
                    except:
                        # Badly formed line, skip it
                        pass
                    
            # Looking for TOF band to cut from each side of the frame
            if not line.strip().startswith("#") and line.lower().find("tof edge discard")>=0:
                cut = re.search("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)", line)
                if cut is not None:
                    self.low_TOF_cut = float(cut.group(1))
                    self.high_TOF_cut = float(cut.group(2))

            # Looking for beam center
            #Spectrum center            = 89.6749, 129.693 [pixel]
            if line.lower().find("spectrum center")>=0:
                ctr = re.search("=[ ]*([0-9]+.[0-9]*)[ ]*[ ,][ ]*([0-9]+.[0-9]+)", line)
                if ctr is not None:
                    self.center_x = float(ctr.group(1))
                    self.center_y = float(ctr.group(2))

            # Sample-detector distance
            #Sample to detector        = 4000 [mm]
            if line.lower().find("sample to detector")>=0:
                dist = re.search("=[ ]*([0-9]+.?[0-9]*)", line)
                if dist is not None:
                    self.sample_detector_dist = dist.group(1)

            # Prompt pulse width
            #Prompt pulse halfwidth          = 20 [microseconds]
            if line.lower().find("prompt pulse")>=0:
                width = re.search("=[ ]*([0-9]+.?[0-9]*)", line)
                if width is not None:
                    self.prompt_pulse_width = dist.group(1)
