"""*WIKI* 


Creates a cal file from a mask workspace: the masked out detectors (Y == 0 in mask workspace) will be combined into group 1.



*WIKI*"""
from MantidFramework import *

class MaskWorkspaceToCalFile(PythonAlgorithm):

	def category(self):
		return "General"

	def name(self):
		return "MaskWorkspaceToCalFile"


	def PyInit(self):
		self.declareWorkspaceProperty("InputWorkspace", "", Direction = Direction.Input, Description = 'Input mask workspace')
		self.declareFileProperty("OutputFile","", FileAction.Save, ['cal'],Description="The file to contain the results")

		self.declareProperty("Invert", False, Description="If True, masking is inverted in the input workspace. Default: False")
		
	def PyExec(self):
		#extract settings
		inputWorkspace = self.getProperty("InputWorkspace")
		outputFileName = self.getProperty("OutputFile")
		invert = self.getProperty("Invert")

		#check for consistency
		if inputWorkspace.getNumberBins() < 1:
			raise RuntimeError('The input workspace is empty.')

		calFile = open(outputFileName,"w")
		#write a header
		instrumentName = inputWorkspace.getInstrument().getName()
		calFile.write('# '+instrumentName+' detector file\n')
		calFile.write('# Format: number      UDET       offset       select    group\n')
		#save the grouping
		for i in range(inputWorkspace.getNumberHistograms()):
			if (inputWorkspace.readY(i)[0] == 0) == (not invert):
				group = 1
			else:
				group = 0
			det = inputWorkspace.getDetector(i)
			detID = det.getID()
			calFile.write(self.FormatLine(i,detID,0.0,1,group))
		calFile.close()


	def FormatLine(self,number,UDET,offset,select,group):
		line = "{0:9d}{1:16d}{2:16.7f}{3:9d}{4:9d}\n".format(number,UDET,offset,select,group)
		return line

#############################################################################################

mtd.registerPyAlgorithm(MaskWorkspaceToCalFile())
