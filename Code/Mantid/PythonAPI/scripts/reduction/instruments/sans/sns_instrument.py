"""
    Instrument class for EQSANS reduction
"""
from reduction import Instrument

class EQSANS(Instrument):
    """
        HFIR SANS instrument description
    """
    def __init__(self, instrument_id="EQSANS") :
        self._NAME = instrument_id
        ## Number of detector pixels in X
        self.nx_pixels = 192
        ## Number of detector pixels in Y
        self.ny_pixels = 256
        ## Pixel size in mm
        self.pixel_size_x = 5.5
        self.pixel_size_y = 4.297
        ## Nominal sample-detector distance
        sample_detector_distance = 0.0
        
    def get_masked_pixels(self, nx_low, nx_high, ny_low, ny_high):
        """
            Generate a list of masked pixels.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector
        """
        masked_x = range(0, nx_low)
        masked_x.extend(range(self.nx_pixels-nx_high, self.nx_pixels))

        masked_y = range(0, ny_low)
        masked_y.extend(range(self.ny_pixels-ny_high, self.ny_pixels))
        
        masked_pts = []
        for y in masked_y:
            masked_pts.extend([ [y,x] for x in range(self.nx_pixels) ])
        for x in masked_x:
            masked_pts.extend([ [y,x] for y in range(ny_low, self.ny_pixels-ny_high) ])
        
        return masked_pts
        
    def get_masked_detectors(self, masked_pixels):
        """
            Returns a list of masked detector channels from the list of pixels.
            This is very instrument-dependent, because it depends on the actual
            mapping of the detector pxiels to Mantid detector IDs.
        """
        return NotImplemented
        
        
