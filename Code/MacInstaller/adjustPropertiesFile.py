#!/bin/python
#
# Copies the default Mantid.properties file, adjusting directories where
# necessary.
#

# Field replacements
replacements = {
    "plugins.directory":"plugins.directory = ../../plugins",
    "instrumentDefinition.directory":"instrumentDefinition.directory = ../../instrument",
    "requiredpythonscript.directories":"""requiredpythonscript.directories = ../../scripts/Crystallography;../../scripts/Disordered Materials;../../scripts/Engineering;\\
../../scripts/Excitations;../../scripts/Large Scale Structures;../../scripts/Molecular Spectroscopy;\\
../../scripts/Muons;../../scripts/Neutrinos;../../scripts/SANS""",
    "pythonscripts.directory":"pythonscripts.directory = ../../scripts",
    "pythonscripts.directories":"pythonscripts.directories = ../../scripts",
    "pythonalgorithms.directories":"../../plugins/PythonAlgs"
}

template = open('../Mantid/Properties/Mantid.properties','r')
original = template.readlines()
prop_file = open('MantidPlot.app/Contents/MacOS/Mantid.properties','w')
continuation = False
nlines = len(original)
index = 0
while( index < nlines ):
    line = original[index]
    key = ""
    for rep in replacements.iterkeys():
        if line.startswith(rep):
            key = rep
            break
    if key != "":
        prop_file.write(replacements[key] + "\n")
        # Skip any backslashed lines
        while line.rstrip().endswith("\\") and index < nlines:
            index += 1
            line = original[index]
    else:
        prop_file.write(line)
    index += 1
    
template.close()
prop_file.close()
