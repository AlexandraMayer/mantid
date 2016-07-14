import numpy as np
import re
from mantid.kernel import ConfigService

# This is to make sure that Lorentzians get evaluated properly
ConfigService.setString('curvefitting.peakRadius', str(100))

# RegEx pattern matching a composite function parameter name, eg f2.Sigma.
fn_pattern = re.compile('f(\\d+)\\.(.+)')

# RegEx pattern matching a composite function parameter name, eg f2.Sigma. Multi-spectrum case.
fn_ms_pattern = re.compile('f(\\d+)\\.f(\\d+)\\.(.+)')


def MakeWorkspace(x, y):
    """Create a workspace that doesn't appear in the ADS"""
    from mantid.api import AlgorithmManager
    alg = AlgorithmManager.createUnmanaged('CreateWorkspace')
    alg.initialize()
    alg.setChild(True)
    alg.setProperty('DataX', x)
    alg.setProperty('DataY', y)
    alg.setProperty('OutputWorkspace', 'dummy')
    alg.execute()
    return alg.getProperty('OutputWorkspace').value


class CrystalField(object):
    """Calculates the crystal fields for one ion"""

    ion_nre_map = {'Ce': 1, 'Pr': 2, 'Nd': 3, 'Pm': 4, 'Sm': 5, 'Eu': 6, 'Gd': 7,
                   'Tb': 8, 'Dy': 9, 'Ho': 10, 'Er': 11, 'Tm': 12, 'Yb': 13}

    allowed_symmetries = ['C1', 'Ci', 'C2', 'Cs', 'C2h', 'C2v', 'D2', 'D2h', 'C4', 'S4', 'C4h',
                          'D4', 'C4v', 'D2d', 'D4h', 'C3', 'S6', 'D3', 'C3v', 'D3d', 'C6', 'C3h',
                          'C6h', 'D6', 'C6v', 'D3h', 'D6h', 'T', 'Td', 'Th', 'O', 'Oh']

    default_peakShape = 'Gaussian'

    default_spectrum_size = 200

    field_parameter_names = ['BmolX','BmolY','BmolZ','BextX','BextY','BextZ',
                             'B20','B21','B22','B40','B41','B42','B43','B44','B60','B61','B62','B63','B64','B65','B66',
                             'IB21','IB22','IB41','IB42','IB43','IB44','IB61','IB62','IB63','IB64','IB65','IB66']

    def __init__(self, Ion, Symmetry, **kwargs):
        """
        Constructor.

        @param Ion: A rare earth ion. Possible values:
                    Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb

        @param Symmetry: Symmetry of the field. Possible values:
                         C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3,
                         S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh

        @param kwargs: Other field parameters and attributes. Acceptable values include:
                        ToleranceEnergy:     energy tolerance,
                        ToleranceIntensity:  intensity tolerance,
                        ResolutionModel: A resolution model.

                        Field parameters:

                        BmolX: The x-component of the molecular field,
                        BmolY: The y-component of the molecular field,
                        BmolZ: The z-component of the molecular field,
                        BextX: The x-component of the external field,
                        BextY: The y-component of the external field,
                        BextZ: The z-component of the external field,
                        B20: Real part of the B20 field parameter,
                        B21: Real part of the B21 field parameter,
                        B22: Real part of the B22 field parameter,
                        B40: Real part of the B40 field parameter,
                        B41: Real part of the B41 field parameter,
                        B42: Real part of the B42 field parameter,
                        B43: Real part of the B43 field parameter,
                        B44: Real part of the B44 field parameter,
                        B60: Real part of the B60 field parameter,
                        B61: Real part of the B61 field parameter,
                        B62: Real part of the B62 field parameter,
                        B63: Real part of the B63 field parameter,
                        B64: Real part of the B64 field parameter,
                        B65: Real part of the B65 field parameter,
                        B66: Real part of the B66 field parameter,
                        IB21: Imaginary part of the B21 field parameter,
                        IB22: Imaginary part of the B22 field parameter,
                        IB41: Imaginary part of the B41 field parameter,
                        IB42: Imaginary part of the B42 field parameter,
                        IB43: Imaginary part of the B43 field parameter,
                        IB44: Imaginary part of the B44 field parameter,
                        IB61: Imaginary part of the B61 field parameter,
                        IB62: Imaginary part of the B62 field parameter,
                        IB63: Imaginary part of the B63 field parameter,
                        IB64: Imaginary part of the B64 field parameter,
                        IB65: Imaginary part of the B65 field parameter,
                        IB66: Imaginary part of the B66 field parameter,


                        Each of the following parameters can be either a single float or an array of floats.
                        They are either all floats or all arrays of the same size.

                        IntensityScaling: A scaling factor for the intensity of each spectrum.
                        FWHM: A default value for the full width at half maximum of the peaks.
                        Temperature: A temperature "of the spectrum" in Kelvin
        """
        from .function import PeaksFunction
        self._ion = Ion
        self._symmetry = Symmetry
        self._toleranceEnergy = 1e-10
        self._toleranceIntensity = 1e-3
        self._fieldParameters = {}
        self._fieldTies = {}
        self._fieldConstraints = []
        self._temperature = None
        self._FWHM = None
        self._intensityScaling = 1.0
        self._resolutionModel = None

        self.peaks = PeaksFunction()
        self.background = None

        for key in kwargs:
            if key == 'ToleranceEnergy':
                self._toleranceEnergy = kwargs[key]
            elif key == 'ToleranceIntensity':
                self._toleranceIntensity = kwargs[key]
            elif key == 'IntensityScaling':
                self._intensityScaling = kwargs[key]
            elif key == 'FWHM':
                self._FWHM = kwargs[key]
            elif key == 'ResolutionModel':
                self._resolutionModel = kwargs[key]
            elif key == 'Temperature':
                self._temperature = kwargs[key]
            else:
                # Crystal field parameters
                self._fieldParameters[key] = kwargs[key]

        # Eigensystem
        self._dirty_eigensystem = True
        self._eigenvalues = None
        self._eigenvectors = None
        self._hamiltonian = None

        # Peak lists
        self._dirty_peaks = True
        self._peakList = None

        # Spectra
        self._dirty_spectra = True
        self._spectra = {}
        self._plot_window = {}

        self._setDefaultTies()

    def makePeaksFunction(self, i):
        """Form a definition string for the CrystalFieldPeaks function
        @param i: Index of a spectrum.
        """
        temperature = self._getTemperature(i)
        s = 'name=CrystalFieldPeaks,Ion=%s,Symmetry=%s,Temperature=%s' % (self._ion, self._symmetry, temperature)
        s += ',ToleranceEnergy=%s,ToleranceIntensity=%s' % (self._toleranceEnergy, self._toleranceIntensity)
        s += ',%s' % ','.join(['%s=%s' % item for item in self._fieldParameters.items()])
        return s

    def makeSpectrumFunction(self, i=0):
        """Form a definition string for the CrystalFieldSpectrum function
        @param i: Index of a spectrum.
        """
        from .function import Background
        temperature = self._getTemperature(i)
        s = 'name=CrystalFieldSpectrum,Ion=%s,Symmetry=%s,Temperature=%s' % (self._ion, self._symmetry, temperature)
        s += ',ToleranceEnergy=%s,ToleranceIntensity=%s' % (self._toleranceEnergy, self._toleranceIntensity)
        s += ',PeakShape=%s' % self.getPeak(i).name
        if self._FWHM is not None:
            s += ',FWHM=%s' % self._getFWHM(i)
        s += ',%s' % ','.join(['%s=%s' % item for item in self._fieldParameters.items()])
        peaks = self.getPeak(i)
        params = peaks.paramString('', 0)
        if len(params) > 0:
            s += ',%s' % params
        ties = peaks.tiesString()
        if len(ties) > 0:
            s += ',%s' % ties
        constraints = peaks.constraintsString()
        if len(constraints) > 0:
            s += ',%s' % constraints
        if self.background is not None:
            if isinstance(self.background, Background):
                bs = self.background.toString()
            else:
                bs = self.background[i].toString()
            s = '%s;%s' % (bs, s)
        ties = self.getFieldTies()
        if len(ties) > 0:
            s += ',ties=(%s)' % ties
        constraints = self.getFieldConstraints()
        if len(constraints) > 0:
            s += ',constraints=(%s)' % constraints
        return s

    def makeMultiSpectrumFunction(self):
        """Form a definition string for the CrystalFieldMultiSpectrum function"""
        s = 'name=CrystalFieldMultiSpectrum,Ion=%s,Symmetry=%s' % (self._ion, self._symmetry)
        s += ',ToleranceEnergy=%s,ToleranceIntensity=%s' % (self._toleranceEnergy, self._toleranceIntensity)
        s += ',PeakShape=%s' % self.getPeak().name
        if self.background is not None:
            s += ',Background=%s' % self.background[0].nameString()
        s += ',Temperatures=(%s)' % ','.join(map(str, self._temperature))
        if self._FWHM is not None:
            s += ',FWHMs=(%s)' % ','.join(map(str, self._FWHM))
        s += ',%s' % ','.join(['%s=%s' % item for item in self._fieldParameters.items()])

        if self.background is not None:
            i = 0
            for background in self.background:
                bs = background.paramString('f%s.f0.' % i)
                if len(bs) > 0:
                    s += ',%s' % bs
                i += 1
        i = 0
        for peaks in self.peaks:
            ps = peaks.paramString('f%s.' % i, 1)
            if len(ps) > 0:
                s += ',%s' % ps
            i += 1
        ties = self.getFieldTies()
        if len(ties) > 0:
            s += ',ties=(%s)' % ties
        constraints = self.getFieldConstraints()
        if len(constraints) > 0:
            s += ',constraints=(%s)' % constraints
        return s

    @property
    def Ion(self):
        """Get value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        ion = cf.Ion
        """
        return self._ion

    @Ion.setter
    def Ion(self, value):
        """Set new value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Ion = 'Pr'
        """
        if value not in self.ion_nre_map.keys():
            msg = 'Value %s is not allowed for attribute Ion.\nList of allowed values: %s' %\
                  (value, ', '.join(self.ion_nre_map.keys()))
            raise RuntimeError(msg)
        self._ion = value
        self._dirty_eigensystem = True
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def Symmetry(self):
        """Get value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        symm = cf.Symmetry
        """
        return self._symmetry

    @Symmetry.setter
    def Symmetry(self, value):
        """Set new value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Symmetry = 'Td'
        """
        if value not in self.allowed_symmetries:
            msg = 'Value %s is not allowed for attribute Symmetry.\nList of allowed values: %s' % \
                  (value, ', '.join(self.allowed_symmetries))
            raise RuntimeError(msg)
        self._symmetry = value
        self._dirty_eigensystem = True
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def ToleranceEnergy(self):
        """Get energy tolerance"""
        return self._toleranceEnergy

    @ToleranceEnergy.setter
    def ToleranceEnergy(self, value):
        """Set energy tolerance"""
        self._toleranceEnergy = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def ToleranceIntensity(self):
        """Get intensity tolerance"""
        return self._toleranceIntensity

    @ToleranceIntensity.setter
    def ToleranceIntensity(self, value):
        """Set intensity tolerance"""
        self._toleranceIntensity = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def IntensityScaling(self):
        return self._intensityScaling

    @IntensityScaling.setter
    def IntensityScaling(self, value):
        self._intensityScaling = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def Temperature(self):
        return self._temperature

    @Temperature.setter
    def Temperature(self, value):
        self._temperature= value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def FWHM(self):
        return self._FWHM

    @FWHM.setter
    def FWHM(self, value):
        self._FWHM = value
        self._dirty_spectra = True

    @property
    def param(self):
        return self._fieldParameters

    def ties(self, **kwargs):
        """Set ties on the field parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(B20 = 0.1, IB23 = '2*B23')
        """
        for tie in kwargs:
            self._fieldTies[tie] = kwargs[tie]

    def constraints(self, *args):
        """
        Set constraints for the field parameters.

        @param args: A list of constraints. For example:
                constraints('B00 > 0', '0.1 < B43 < 0.9')
        """
        self._fieldConstraints += args

    def setPeaks(self, name):
        from .function import PeaksFunction
        """Define the shape of the peaks and create PeakFunction instances."""
        if self._temperature is None or not isinstance(self._temperature, list):
            self.peaks = PeaksFunction(name)
        else:
            self.peaks = [PeaksFunction(name) for t in self._temperature]

    def setBackground(self, peak=None, background=None):
        from .function import Function, Background
        if isinstance(self._temperature, list):
            n = len(self._temperature)
            self.background = n * Background(peak=peak, background=background)
        else:
            self.background = Background(peak=peak, background=background)


    def getPeak(self, i=0):
        if isinstance(self.peaks, list):
            return self.peaks[i]
        else:
            return self.peaks

    def getEigenvalues(self):
        self._calcEigensystem()
        return self._eigenvalues

    def getEigenvectors(self):
        self._calcEigensystem()
        return self._eigenvectors

    def getHamiltonian(self):
        self._calcEigensystem()
        return self._hamiltonian

    def getPeakList(self, i=0):
        """Get the peak list for spectrum i as a numpy array"""
        self._calcPeaksList(i)
        peaks = np.array([self._peakList.column(0), self._peakList.column(1)])
        return peaks

    def getSpectrum(self, i=0, workspace=None, ws_index=0):
        """
        Get the i-th spectrum calculated with the current field and peak parameters.

        Alternatively can be called getSpectrum(workspace, ws_index). Spectrum index i is assumed zero.

        Examples:

            cf.getSpectrum() # Return the first spectrum calculated on a generated set of x-values.
            cf.getSpectrum(1, ws, 5) # Calculate the second spectrum using the x-values from the 6th spectrum
                                     # in workspace ws.
            cf.getSpectrum(ws) # Calculate the first spectrum using the x-values from the 1st spectrum
                               # in workspace ws.
            cf.getSpectrum(ws, 3) # Calculate the first spectrum using the x-values from the 4th spectrum
                                  # in workspace ws.

        @param i: Index of a spectrum to get.
        @param workspace: A workspace to base on. If not given the x-values of the output spectrum will be
                          generated.
        @param ws_index:  An index of a spectrum from workspace to use.
        @return: A tuple of (x, y) arrays
        """
        if self._dirty_spectra:
            self._spectra = {}
            self._dirty_spectra = False

        ws = workspace
        # Allow to call getSpectrum with a workspace as the first argument.
        if not isinstance(i, int):
            if ws is not None:
                if not isinstance(ws, int):
                    raise RuntimeError('Spectrum index is expected to be int. Got %s' % i.__class__.__name__)
                ws_index = ws
            ws = i
            i = 0

        # Workspace is given, always calculate
        if ws is None:
            x = None
        elif isinstance(ws, list) or isinstance(ws, np.ndarray):
            x = ws
        else:
            return self._calcSpectrum(i, ws, ws_index)

        if x is None:
            if i in self._spectra:
                return self._spectra[i]
            else:
                x_min, x_max = self._calc_xmin_xmax(i)
                x = np.linspace(x_min, x_max, self.default_spectrum_size)

        y = np.zeros_like(x)
        ws = MakeWorkspace(x, y)
        self._spectra[i] = self._calcSpectrum(i, ws, 0)
        return self._spectra[i]

    def plot(self, i=0, workspace=None, ws_index=0):
        """Plot a spectrum. Parameters are the same as in getSpectrum(...)"""
        from mantid.simpleapi import CreateWorkspace
        from mantidplot import plotSpectrum
        x, y = self.getSpectrum(i, workspace, ws_index)
        ws_name = 'CrystalField_%s' % self._ion

        if isinstance(i, int):
            if workspace is None:
                if i > 0:
                    ws_name += '_%s' % i
                ws = CreateWorkspace(x, y, OutputWorkspace=ws_name)
                plot_window = self._plot_window[i] if i in self._plot_window else None
                self._plot_window[i] = plotSpectrum(ws, 0, window=plot_window, clearWindow=True)
            else:
                ws_name += '_%s' % workspace
                if i > 0:
                    ws_name += '_%s' % i
                ws = CreateWorkspace(x, y, OutputWorkspace=ws_name)
                plotSpectrum(ws, 0)
        else:
            ws_name += '_%s' % i
            ws = CreateWorkspace(x, y, OutputWorkspace=ws_name)
            plotSpectrum(ws, 0)

    def _setDefaultTies(self):
        for name in self.field_parameter_names:
            if name not in self._fieldParameters:
                self._fieldTies[name] = '0'

    def getFieldTies(self):
        ties = ['%s=%s' % item for item in self._fieldTies.items()]
        return ','.join(ties)

    def getFieldConstraints(self):
        return ','.join(self._fieldConstraints)

    def updateParameters(self, func):
        """
        Update values of the field and peaks parameters.
        @param func: A IFunction object containing new parameter values.
        """
        n = func.nParams()
        for i in range(n):
            par = func.parameterName(i)
            value = func.getParameterValue(i)
            if par == 'IntensityScaling':
                self._intensityScaling = value
            else:
                m = re.match(fn_pattern, par)
                if m:
                    i = int(m.group(1))
                    par = m.group(2)
                    self.peaks.param[i][par] = value
                else:
                    self._fieldParameters[par] = value


    def update(self, func):
        """
        Update values of the fitting parameters.
        @param func: A IFunction object containing new parameter values.
        """
        from mantid.api import CompositeFunction
        if isinstance(func, CompositeFunction):
            nf = len(func)
            if nf == 3:
                self.background.update(func[0], func[1])
                self.updateParameters(func[2])
            elif nf == 2:
                self.background.update(func[0])
                self.updateParameters(func[1])
            else:
                raise RuntimeError('CompositeFunuction cannot have more than 3 components.')
        else:
            self.updateParameters(func)

    def update_multi(self, func):
        """
        Update values of the fitting parameters in case of a multi-spectrum function.
        @param func: A IFunction object containing new parameter values.
        """
        n = func.nParams()
        for i in range(n):
            par = func.parameterName(i)
            value = func.getParameterValue(i)
            m = re.match(fn_ms_pattern, par)
            if m:
                ispec = int(m.group(1))
                ipeak = int(m.group(2))
                par = m.group(3)
                if ipeak == 0:
                    background = self.background[ispec]
                    mb = re.match(fn_pattern, par)
                    if mb:
                        i = int(mb.group(1))
                        par = mb.group(2)
                        if i == 0:
                            background.peak.param[par] = value
                        else:
                            background.background.param[par] = value
                    else:
                        if background.peak is not None:
                            background.peak.param[par] = value
                        elif background.background is not None:
                            background.background.param[par] = value
                        else:
                            raise RuntimeError('Background is undefined in CrystalField instance.')
                else:
                    self.peaks[ispec].param[ipeak - 1][par] = value
            else:
                self._fieldParameters[par] = value

    def __add__(self, other):
        return CrystalFieldMulti(self, other)

    def _getTemperature(self, i):
        """Get temperature value for i-th spectrum."""
        if self._temperature is None:
            raise RuntimeError('Temperature must be set.')
        if isinstance(self._temperature, float) or isinstance(self._temperature, int):
            if i != 0:
                raise RuntimeError('Cannot evaluate spectrum %s. Only 1 temperature is given.' % i)
            return float(self._temperature)
        else:
            n = len(self._temperature)
            if i >= -n and i < n:
                return float(self._temperature[i])
            else:
                raise RuntimeError('Cannot evaluate spectrum %s. Only %s temperatures are given.' % (i, n))

    def _getFWHM(self, i):
        """Get default FWHM value for i-th spectrum."""
        if self._FWHM is None:
            raise RuntimeError('Default FWHM must be set.')
        if isinstance(self._FWHM, float) or isinstance(self._FWHM, int):
            # if i != 0 assume that value for all spectra
            return float(self._FWHM)
        else:
            n = len(self._FWHM)
            if i >= -n and i < n:
                return float(self._FWHM[i])
            else:
                raise RuntimeError('Cannot get FWHM for spectrum %s. Only %s FWHM are given.' % (i, n))

    def _getPeaksFunction(self, i):
        if isinstance(self.peaks, list):
            return self.peaks[i]
        return self.peaks

    def _calcEigensystem(self):
        """Calculate the eigensystem: energies and wavefunctions.
        Also store them and the hamiltonian.
        Protected method. Shouldn't be called directly by user code.
        """
        if self._dirty_eigensystem:
            from energies import energies
            nre = self.ion_nre_map[self._ion]
            self._eigenvalues, self._eigenvectors, self._hamiltonian = energies(nre, **self._fieldParameters)
            self._dirty_eigensystem = False

    def _calcPeaksList(self, i):
        """Calculate a peak list for spectrum i"""
        if self._dirty_peaks:
            from mantid.api import AlgorithmManager
            alg = AlgorithmManager.createUnmanaged('EvaluateFunction')
            alg.initialize()
            alg.setChild(True)
            alg.setProperty('Function', self.makePeaksFunction(i))
            del alg['InputWorkspace']
            alg.setProperty('OutputWorkspace', 'dummy')
            alg.execute()
            self._peakList = alg.getProperty('OutputWorkspace').value

    def _calcSpectrum(self, i, workspace, ws_index):
        """Calculate i-th spectrum.

        @param i: Index of a spectrum
        @param workspace: A workspace used to evaluate the spectrum function.
        @param ws_index:  An index of a spectrum in workspace to use.
        """
        from mantid.api import AlgorithmManager
        import numpy as np
        alg = AlgorithmManager.createUnmanaged('EvaluateFunction')
        alg.initialize()
        alg.setChild(True)
        alg.setProperty('Function', self.makeSpectrumFunction(i))
        alg.setProperty("InputWorkspace", workspace)
        alg.setProperty('WorkspaceIndex', ws_index)
        alg.setProperty('OutputWorkspace', 'dummy')
        alg.execute()
        out = alg.getProperty('OutputWorkspace').value
        # Create copies of the x and y because `out` goes out of scope when this method returns
        # and x and y get deallocated
        return np.array(out.readX(0)), np.array(out.readY(1))

    def _calc_xmin_xmax(self, i):
        """Calculate the x-range containing interesting features of a spectrum (for plotting)
        @param i: If an integer is given then calculate the x-range for the i-th spectrum.
                  If None given get the range covering all the spectra.
        @return: Tuple (xmin, xmax)
        """
        peaks = self.getPeakList(i)
        x_min = np.min(peaks[0])
        x_max = np.max(peaks[0])
        # TODO: dx probably should depend on peak widths
        dx = np.abs(x_max - x_min) * 0.1
        if x_min < 0:
            x_min -= dx
        x_max += dx
        return x_min, x_max


class CrystalFieldMulti(object):
    """CrystalFieldMulti represents crystal field of multiple ions."""

    def __init__(self, *args):
        if isinstance(args, tuple):
            self.args = args
        else:
            self.args = (self.args,)
        self._ties = {}

    def makeSpectrumFunction(self):
        fun = ';'.join([a.makeSpectrumFunction() for a in self.args])
        ties = self.getTies()
        if len(ties) > 0:
            fun += ',ties=(%s)' % ties
        return fun

    def ties(self, **kwargs):
        """Set ties on the parameters."""
        for tie in kwargs:
            self._ties[tie] = kwargs[tie]

    def getTies(self):
        ties = ['%s=%s' % item for item in self._ties.items()]
        return ','.join(ties)

    def getSpectrum(self, i=0, workspace=None, ws_index=0):
        if workspace is not None:
            x, y = self.args[0].getSpectrum(i, workspace, ws_index)
            for a in self.args[1:]:
                _, yy = a.getSpectrum(i, workspace, ws_index)
                y += yy
            return x, y
        x_min = 0.0
        x_max = 0.0
        for a in self.args:
            xmin, xmax = a._calc_xmin_xmax(i)
            if xmin < x_min:
                x_min = xmin
            if xmax > x_max:
                x_max = xmax
        x = np.linspace(x_min, x_max, CrystalField.default_spectrum_size)
        _, y = self.args[0].getSpectrum(i, x, ws_index)
        for a in self.args[1:]:
            _, yy = a.getSpectrum(i, x, ws_index)
            y += yy
        return x, y

    def update(self, func):
        n = func.nFunctions()
        assert n == len(self.args)
        for i in range(n):
            self.args[i].update(func[i])

    def __add__(self, other):
        if isinstance(other, CrystalFieldMulti):
            return CrystalFieldMulti(*(self.args + other.args))
        else:
            return CrystalFieldMulti(*(self.args.append(other.args)))

    def __len__(self):
        return len(self.args)

    def __getitem__(self, item):
        return self.args[item]


class CrystalFieldFit(object):
    """
    Object that controls fitting.
    """

    def __init__(self, Model=None, Temperature=None, FWHM=None, InputWorkspace=None):
        self.model = Model
        if Temperature is not None:
            self.model.Temperature = Temperature
        if FWHM is not None:
            self.model.FWHM = FWHM
        self._input_workspace = InputWorkspace
        self._output_workspace_base_name = 'fit'

    def fit(self):
        """
        Run Fit algorithm. Update function parameters.
        """
        if isinstance(self._input_workspace, list):
            return self._fit_multi()
        else:
            return self._fit_single()

    def _fit_single(self):
        """
        Fit when the model has a single spectrum.
        """
        from mantid.api import AlgorithmManager
        fun = self.model.makeSpectrumFunction()
        alg = AlgorithmManager.createUnmanaged('Fit')
        alg.initialize()
        alg.setProperty('Function', fun)
        alg.setProperty('InputWorkspace', self._input_workspace)
        alg.setProperty('Output', 'fit')
        alg.execute()
        f = alg.getProperty('Function').value
        self.model.update(f)

    def _fit_multi(self):
        """
        Fit when the model has multiple spectra.
        """
        from mantid.api import AlgorithmManager
        fun = self.model.makeMultiSpectrumFunction()
        alg = AlgorithmManager.createUnmanaged('Fit')
        alg.initialize()
        alg.setProperty('Function', fun)
        alg.setProperty('InputWorkspace', self._input_workspace[0])
        i = 1
        for ws in self._input_workspace[1:]:
            alg.setProperty('InputWorkspace_%s' % i, ws)
            i += 1
        alg.setProperty('Output', 'fit')
        alg.execute()
        f = alg.getProperty('Function').value
        self.model.update_multi(f)
