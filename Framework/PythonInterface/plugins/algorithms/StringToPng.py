#pylint: disable=no-init,invalid-name
import mantid,sys

class StringToPng(mantid.api.PythonAlgorithm):

    def category(self):
        """ Category
        """
        return "DataHandling\\Plots"

    def name(self):
        """ Algorithm name
        """
        return "StringToPng"

    def summary(self):
        return "Creates an image file containing a string."

    def checkGroups(self):
        return False

    def PyInit(self):
        #declare properties
        self.declareProperty("String","", mantid.kernel.StringMandatoryValidator(),"String to plot")
        self.declareProperty(mantid.api.FileProperty('OutputFilename', '', action=mantid.api.FileAction.Save, extensions = ["png"]),
                             doc='Name of the image file to savefile.')

    def PyExec(self):
        ok2run=''
        try:
            import matplotlib
        except ImportError:
            ok2run='Problem importing matplotlib'
        if ok2run!='':
            raise RuntimeError(ok2run)
        matplotlib=sys.modules['matplotlib']
        matplotlib.use("agg")
        import matplotlib.pyplot as plt
        fig=plt.figure(figsize=(.1,.1))
        ax1=plt.axes(frameon=False)
        ax1.text(0.,1,self.getProperty("String").valueAsStr.decode('string_escape'),va="center",fontsize=16)
        ax1.axes.get_xaxis().set_visible(False)
        ax1.axes.get_yaxis().set_visible(False)
        plt.show()
        filename = self.getProperty("OutputFilename").value
        plt.savefig(filename,bbox_inches='tight')
        plt.close(fig)

mantid.api.AlgorithmFactory.subscribe(StringToPng)
