class Function(object):
    """A helper object that simplifies getting and setting parameters of a simple named function."""

    def __init__(self, name, **kwargs):
        """
        Initialise new instance.
        @param name: A valid name registered with the FunctionFactory.
        @param kwargs: Parameters (but not attributes) of this function. To set attributes use `attr` property.
                Example:
                    f = Function('TabulatedFunction', Scaling=2.0)
                    f.attr['Workspace'] = 'workspace_with_data'
        """
        self._name = name
        # Function attributes.
        self._attrib = {}
        # Function parameters.
        self._params = {}
        for param in kwargs:
            self._params[param] = kwargs[param]

        self._ties = {}
        self._constraints = []

    def clone(self):
        """Make a copy of self."""
        from copy import copy
        f = Function(self._name)
        # Make shallow copies of the member collections
        f._attrib = copy(self._attrib)
        f._params = copy(self._params)
        f._ties = copy(self._ties)
        f._constraints = copy(self._constraints)
        return f

    @property
    def name(self):
        """Read only name of this function"""
        return self._name

    @property
    def attr(self):
        return self._attrib

    @property
    def param(self):
        return self._params

    def ties(self, **kwargs):
        """Set ties on the parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(A0 = 0.1, A1 = '2*A0')
        """
        for tie in kwargs:
            self._ties[tie] = kwargs[tie]

    def constraints(self, *args):
        """
        Set constraints for the parameters.

        @param args: A list of constraints. For example:
                constraints('A0 > 0', '0.1 < A1 < 0.9')
        """
        self._constraints += args

    def toString(self):
        """Create function initialisation string"""
        attrib = ['%s=%s' % item for item in self._attrib.items()] + \
                 ['%s=%s' % item for item in self._params.items()]
        if len(attrib) > 0:
            s = 'name=%s,%s' % (self._name, ','.join(attrib))
        else:
            s = 'name=%s' % self._name
        ties = ','.join(['%s=%s' % item for item in self._ties.items()])
        if len(ties) > 0:
            s += ',ties=(%s)' % ties
        constraints = ','.join(self._constraints)
        if len(constraints) > 0:
            s += ',constraints=(%s)' % constraints
        return s

    def paramString(self, prefix):
        attrib = ['%s%s=%s' % ((prefix,) + item) for item in self._attrib.items()] + \
                 ['%s%s=%s' % ((prefix,) + item) for item in self._params.items()]
        return ','.join(attrib)

    def update(self, func):
        """
        Update values of the fitting parameters.
        @param func: A IFunction object containing new parameter values.
        """
        n = func.nParams()
        for i in range(n):
            par = func.parameterName(i)
            self._params[par] = func.getParameterValue(i)


class CompositeProperties(object):
    """
    A dictionary of dictionaries of function properties: attributes or parameters.
    It mimics properties of a CompositeFunction: the key is a function index and the value
    id a map 'param_name' -> param_value.

    Example:
        {
          0: {'Height': 100, 'Sigma': 1.0}, # Parameters of the first function
          1: {'Height': 120, 'Sigma': 2.0}, # Parameters of the second function
          5: {'Height': 300, 'Sigma': 3.0}, # Parameters of the sixth function
          ...
        }
    """

    def __init__(self):
        self._properties = {}

    def __getitem__(self, item):
        """Get a map of properties for a function number <item>"""
        if item not in self._properties:
            self._properties[item] = {}
        return self._properties[item]

    def getSize(self):
        """Get number of maps (functions) defined here"""
        s = self._properties.keys()
        if len(s) > 0:
            return max(s) + 1
        return 0

    def toStringList(self):
        """Format all properties into a list of strings where each string is a comma-separated
        list of name=value pairs.
        """
        prop_list = []
        for i in range(self.getSize()):
            if i in self._properties:
                props = self._properties[i]
                prop_list.append(','.join(['%s=%s' % item for item in props.items()]))
            else:
                prop_list.append('')
        return prop_list

    def toCompositeString(self, prefix, shift=0):
        """Format all properties as a comma-separated list of name=value pairs where name is formatted
        in the CompositeFunction style.

        Example:
            'f0.Height=100,f0.Sigma=1.0,f1.Height=120,f1.Sigma=2.0,f5.Height=300,f5.Sigma=3.0'
        """
        s = ''
        for i in self._properties:
            f = '%sf%s.' % (prefix, i + shift)
            props = self._properties[i]
            if len(s) > 0:
                s += ','
            s += ','.join(['%s%s=%s' % ((f,) + item) for item in props.items()])
        return s[:]


class PeaksFunction(object):
    """A helper object that simplifies getting and setting parameters of a composite function
    containing multiple peaks of the same type.

    The object of this class has no access to the C++ fit function it represents. It means that
    it doesn't know what attributes or parameters the function defines and relies on the user
    to provide correct information.

    @param name: A name of the individual peak function, such as 'Lorentzian' or 'Gaussian'.
        If None then the default function is used (currently 'Lorentzian')
    """
    def __init__(self, name=None):
        """
        Constructor.

        @param name: The name of the function of each peak.  E.g. Gaussian
        """
        # Name of the peaks
        self._name = name if name is not None else 'Lorentzian'
        # Collection of all attributes
        self._attrib = CompositeProperties()
        # Collection of all parameters
        self._params = CompositeProperties()
        # Ties
        self._ties = {}
        # Constraints
        self._constraints = []

    @property
    def name(self):
        """Read only name of the peak function"""
        return self._name

    @property
    def attr(self):
        """Get a dict of all currently set attributes.
        Use this property to set or get an attribute.
        You can only get an attribute that has been previously set via this property.
        """
        return self._attrib

    @property
    def param(self):
        """Get a dict of all currently set parameters
        Use this property to set or get a parameter.
        You can only get a parameter that has been previously set via this property.
        Example:

            fun = PeaksFunction('Gaussian')
            # Set Sigma parameter of the second peak
            peaks.param[1]['Sigma'] = 0.1
            ...
            # Get the value of a previously set parameter
            sigma = peaks.param[1]['Sigma']
            ...
            # Trying to get a value that wasn't set results in an error
            height = peaks[1]['Height'] # error
        """
        return self._params

    def ties(self, tie_dict):
        """Set ties on the peak parameters.

        @param tie_dict: A dictionary of ties: the key is a parameter name in CompositeFunction convention,
            the value is a tie string or a number. For example:
                {'f1.Sigma': 0.1, 'f2.Sigma': '2*f0.Sigma'}
        """
        if not isinstance(tie_dict, dict):
            raise RuntimeError('Tie argument must be a dict.')
        for tie in tie_dict:
            self._ties[tie] = tie_dict[tie]

    def constraints(self, constraints):
        """
        Set constraints for the peak parameters.

        @param constraints: A list of constraints. For example:
                ['f0.Sigma > 0', '0.1 < f1.Sigma < 0.9']
        """
        if not isinstance(constraints, list):
            raise RuntimeError('Constraints argument must be a list.')
        self._constraints += constraints

    def nPeaks(self):
        """Get the number of peaks"""
        n = max(self._attrib.getSize(), self._params.getSize())
        if n == 0:
            raise RuntimeError('PeaksFunction has no defined parameters or attributes.')
        return n

    def toString(self):
        """Create function initialisation string"""
        n = self.nPeaks()
        attribs = self._attrib.toStringList()
        params = self._params.toStringList()
        if len(attribs) < n:
            attribs += [''] * (n - len(attribs))
        if len(params) < n:
            params += [''] * (n - len(params))
        peaks = []
        for i in range(n):
            a = attribs[i]
            p = params[i]
            if len(a) != 0 or len(p) != 0:
                if len(a) == 0:
                    peaks.append('name=%s,%s' % (self._name, p))
                elif len(p) == 0:
                    peaks.append('name=%s,%s' % (self._name, a))
                else:
                    peaks.append('name=%s,%s,%s' % (self._name, a,p))
            else:
                peaks.append('name=%s' % self._name)
        s = ';'.join(peaks)
        if len(self._ties) > 0:
            s += ';%s' % self.tiesString()
        return s

    def paramString(self, prefix='', shift=0):
        """Format a comma-separated list of all peaks attributes and parameters in a CompositeFunction
        style.
        """
        na = self._attrib.getSize()
        np = self._params.getSize()
        if na == 0 and np == 0:
            return ''
        elif na == 0:
            return self._params.toCompositeString(prefix, shift)
        elif np == 0:
            return self._attrib.toCompositeString(prefix, shift)
        else:
            return '%s,%s' % (self._attrib.toCompositeString(prefix, shift),
                              self._params.toCompositeString(prefix, shift))

    def tiesString(self):
        if len(self._ties) > 0:
            return 'ties=(%s)' % ','.join(['%s=%s' % item for item in self._ties.items()])
        return ''

    def constraintsString(self):
        if len(self._constraints) > 0:
            return 'constraints=(%s)' % ','.join(self._constraints)
        return ''


class Background(object):
    """Object representing spectrum background: a sum of a central peak and a
    background.
    """

    def __init__(self, peak=None, background=None):
        """
        Initialise new instance.
        @param peak: An instance of Function class meaning to be the elastic peak.
        @param background: An instance of Function class serving as the background.
        """
        self.peak = peak
        self.background = background

    def clone(self):
        """Make a copy of self."""
        b = Background()
        if self.peak is not None:
            b.peak = self.peak.clone()
        if self.background is not None:
            b.background = self.background.clone()
        return b

    def __mul__(self, n):
        """Make expressions like Background(...) * 8 return a list of 8 identical backgrounds."""
        return [self.clone() for i in range(n)]

    def __rmul__(self, n):
        """Make expressions like 2 * Background(...) return a list of 2 identical backgrounds."""
        return self.__mul__(n)

    def toString(self):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.toString()
        if self.background is None:
            return self.peak.toString()
        return '%s;%s' % (self.peak.toString(), self.background.toString())

    def nameString(self):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.name
        if self.background is None:
            return self.peak.name
        return '"name=%s;name=%s"' % (self.peak.name, self.background.name)

    def paramString(self, prefix):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.paramString(prefix)
        if self.background is None:
            return self.peak.paramString(prefix)
        return '%s,%s' % (self.peak.paramString(prefix + 'f0.'), self.background.paramString(prefix + 'f1.'))

    def update(self, func1, func2=None):
        """
        Update values of the fitting parameters. If both arguments are given
            the first one updates the peak and the other updates the background.

        @param func1: First IFunction object containing new parameter values.
        @param func2: Second IFunction object containing new parameter values.
        """
        if func2 is not None:
            if self.peak is None or self.background is None:
                raise RuntimeError('Background has peak or background undefined.')
            self.peak.update(func1)
            self.background.update(func2)
        elif self.peak is None:
            self.background.update(func1)
        else:
            self.peak.update(func1)
