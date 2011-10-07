# Automatic creation of installer source file (.wxs)
import os
import xml
import xml.dom.minidom
import msilib
import md5
import uuid
import string
import platform
import sys
import subprocess

if len(sys.argv) == 3:
    MANTIDRELEASE = sys.argv[1]
    MANTIDRELEASE = MANTIDRELEASE.replace('\\','/')
    WXSDIR = sys.argv[2]
    WXSFILE = WXSDIR.replace('\\','/') + '/msi_input.wxs'

# Hack while we still have scons around in some places
if not os.path.exists(MANTIDRELEASE + '/MantidPlot.exe'):
    sys.exit('Path "%s" does not exist. Incorrect output path specified.' % (MANTIDRELEASE+'/MantidPlot.exe'))

# MantidPlot -v prints the version number
subp = subprocess.Popen([MANTIDRELEASE + '/MantidPlot',  '-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
subout, suberr = subp.communicate()
if suberr != '':
    print 'Could not determine Mantid version from MantidPlot using 1.0.0'
    MantidVersion = '1.0.0'
else:
    MantidVersion = subout.strip() # This contains the date as well, we only want the numbers
    try:
        MantidVersion = MantidVersion.split()[0]
    except IndexError:
        print ('Invalid version format "%s", expecting "x.x.x (DATE)"' % MantidVersion)
        MantidVersion = '1.0.0'
print('Mantid version '+ MantidVersion)

# Architecture
if platform.architecture()[0] == '64bit':
    ARCH = '64'
    upgrade_uuid = '{ae4bb5c4-6b5f-4703-be9a-b5ce662b81ce}'
else:
    ARCH = '32'
    upgrade_uuid = '{E9B6F1A9-8CB7-4441-B783-4E7A921B37F0}'

# Define our source directories
CODEDIR = os.path.abspath(os.path.join(os.path.dirname(__file__), r'..\..\..'))
FRAMEWORKDIR = CODEDIR.replace('\\', '/') + '/Mantid/Framework'
USERALGORITHMSDIR = FRAMEWORKDIR + '/UserAlgorithms'
LIBDIR = CODEDIR + '/Third_Party/lib/win' + ARCH
QTPLUGINDIR = LIBDIR + '/qt_plugins'
    
# To perform a major upgrade, i.e. uninstall the old version if an old one exists, 
# the product and package GUIDs need to change everytime
product_uuid = '{' + str(uuid.uuid1()) + '}'
package_uuid = '{' + str(uuid.uuid1()) + '}'

# Setup a GUID lookup table for each of the components
# These are different for each architecture to ensure removal of the correct one when uninstalling
comp_guid = {}
if ARCH == '32':
    comp_guid['MantidDLLs'] = '{FABC0481-C18D-415e-A0B1-CCB76C35FBE8}'
    comp_guid['QTIPlot'] = '{03ABDE5C-9084-4ebd-9CF8-31648BEFDEB7}'
    comp_guid['Plugins'] = '{EEF0B4C9-DE52-4f99-A8D0-9D3C3941FA73}'
    comp_guid['Documents'] = '{C16B2B59-17C8-4cc9-8A7F-16254EB8B2F4}'
    comp_guid['Logs'] = '{0918C9A4-3481-4f21-B941-983BE21F9674}'
    comp_guid['IncludeFiles'] = '{e86d1741-bc2b-4a9f-a6a7-3f7905aaf3aa}'
    comp_guid['IncludeMantidAPI'] = '{4761DDF6-813C-4470-8852-98CB9A69EBC9}'
    comp_guid['IncludeMantidKernel'] = '{AF40472B-5822-4ff6-8E05-B4DA5224AA87}'
    comp_guid['IncludeMantidNexusCPP'] = '{12cf277b-8224-420c-b54a-8425c0376fc3}'
    comp_guid['Temp'] = '{02D25B60-A114-4f2a-A211-DE88CF648C61}'
    comp_guid['Data'] = '{6D9A0A53-42D5-46a5-8E88-6BB4FB7A5FE1}'
    comp_guid['UserAlgorithms'] = '{A82B4540-3CDB-45fa-A7B3-42F392378D3F}'
    comp_guid['Colormaps'] = '{902DBDE3-42AE-49d3-819D-1C83C18D280A}'
    comp_guid['QtImagePlugins'] = '{6e3c6f03-5933-40b1-9733-1bd71132404c}'
    comp_guid['MantidQtPlugins'] = '{d035e5aa-2815-4869-836d-8fc4b8e7a418}'
else:
    comp_guid['MantidDLLs'] = '{c9748bae-5934-44ab-b144-420589db1623}'
    comp_guid['QTIPlot'] = '{bfe90c00-9f39-4fde-8dbc-17f419210e12}'
    comp_guid['Plugins'] = '{8ef1c4db-c54d-4bb1-8b66-a9421db24faf}'
    comp_guid['Documents'] = '{bb774537-d0c6-4541-93f2-7aa5f5132d21}'
    comp_guid['Logs'] = '{0cdce87e-976a-40a5-a3d5-73dd8bce9e2e}'
    comp_guid['IncludeFiles'] = '{a291f307-477c-4169-9bdb-65f76eb413bf}'
    comp_guid['IncludeMantidAPI'] = '{878ff1f2-7d09-4817-972b-3590c45ea0c9}'
    comp_guid['IncludeMantidKernel'] = '{187317c0-cc23-4a21-bf19-0e347866620c}'
    comp_guid['IncludeMantidNexusCPP'] = '{20327246-8f47-448b-ae2b-8d464c3a69c7}'
    comp_guid['Temp'] = '{212cc3fe-95fb-40d9-a3a7-8421791ac19f}'
    comp_guid['Data'] = '{c9577b5b-75e5-4a4a-b2d5-f4905174627c}'
    comp_guid['UserAlgorithms'] = '{496555f0-f719-4db7-bd8e-5bbcd9fe837d}'
    comp_guid['Colormaps'] = '{9e4a6fc4-39ea-4b8f-ba49-265d6dcfbb4c}'
    comp_guid['QtImagePlugins'] = '{7c1ec169-d331-4b9c-b0e4-3214bcf2cbf4}'
    comp_guid['MantidQtPlugins'] = '{22fa661e-17d5-4e33-8f2c-654c473268c3}'
    
MantidInstallDir = 'MantidInstall'

pfile = open('mantid_version.txt','w')
pfile.write(MantidVersion+'\n')
pfile.write(product_uuid)
pfile.close()

globalFileCount = 0

# Adds directory longName to parent.
# parent is a python variable (not string) representing an xml element
# Id, name, and longName are strings
def addDirectory(Id,name,longName,parent):
    e = doc.createElement('Directory')
    e.setAttribute('Id',Id)
    e.setAttribute('Name',name)
    if name != longName:
        e.setAttribute('LongName',longName)
    parent.appendChild(e)
    return e

def addFile(Id,name,longName,source,vital,parent):
    e = doc.createElement('File')
    e.setAttribute('Id',Id)
    e.setAttribute('Name',name)
    e.setAttribute('LongName',longName)
    e.setAttribute('DiskId','1')
    e.setAttribute('Source',source)
    e.setAttribute('Vital',vital)
    parent.appendChild(e)
    return e

def addFileV(Id,name,longName,source,parent):
    return addFile(Id,name,longName,source,'yes',parent)

def addFileN(Id,name,longName,source,parent):
    return addFile(Id,name,longName,source,'no',parent)

def addComponent(Id,guid,parent):
    e = doc.createElement('Component')
    e.setAttribute('Id',Id)
    e.setAttribute('Guid',guid)
    if ARCH == '64':
        e.setAttribute('Win64','yes')
    parent.appendChild(e)
    return e

# adds all dlls from location to parent.
# rules are applied to exclude debug libraries
# name is a short name to which a number will be added
def addDlls(location,name,parent, exclude_files = []):
    #print 'Include dlls from',os.path.abspath(location);
    sdlls = os.listdir(location);
    i = 0
    for fil in sdlls:
        if fil in exclude_files:
            continue
        lst = fil.split('.')
        l = len(lst)
        if l < 2 or lst[l-1] != 'dll':
            continue
        del lst[l-1]
        fn0 = string.join(lst,'_')
        fn = fn0.replace('-','_')
        if not ((fil.find('-gd-') >= 0) or (fil.find('-gyd-') >= 0) or
                (fil.find('d.dll')>=0 and fil.replace('d.dll','.dll') in sdlls) or
                (fil.find('d4.dll')>=0 and fil.replace('d4.dll','4.dll') in sdlls) or
                (fil.find('_d.dll')>=0 and fil.replace('_d.dll','.dll') in sdlls)):
            #print fil
            addFileV(fn+'DLL',name+str(i),fil,location+'/'+fil,parent)
        i += 1

def addAllFiles(location,name,parent):
    #print 'Include files from',os.path.abspath(location);
    sfiles = os.listdir(location);
    i = 0
    for fil in sfiles:
        #print fil
        fn = fil.replace('-','_')
        fn = fn.replace('+','_')
        fn = fn.replace(' ','_')
        fn = fn.replace('#','_')
        if (fil.find('.svn') < 0 and os.path.isfile(location+'/'+fil)):
            addFileV(name+'_'+fn+'_file',name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addAllFilesExt(location,name,ext,parent):
    #print 'Include files from',os.path.abspath(location);
    sfiles = os.listdir(location);
    i = 0
    for fil in sfiles:
        fn = fil.replace('-','_')
        fn = fn.replace('+','_')
        fn = fn.replace('#','_')
        if (fil.find('.svn') < 0 and fil.endswith('.'+ext) > 0):
            #print fil
            addFileV(name+'_'+fn+'_file',name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addSingleFile(location,fil,name,parent):
    #print 'Include single file'
    location = os.path.abspath(location);
    fn = name.replace('-','_')
    fn = fn.replace('+','_')
    fn = fn.replace('#','_')
    addFileV(fn+'_file',name,fil,location+'/'+fil,parent)

def addFeature(Id,title,description,level,parent,absent='allow',allowAdvertise='yes'):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Title',title)
    e.setAttribute('Description',description)
    e.setAttribute('Level',level)
    e.setAttribute('Absent',absent)
    e.setAttribute('AllowAdvertise',allowAdvertise)
    parent.appendChild(e)
    return e

def addHiddenFeature(Id,parent):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Level','1')
    e.setAttribute('Display','hidden')
    e.setAttribute('Absent','allow')
    e.setAttribute('AllowAdvertise','no')
    parent.appendChild(e)
    return e

def addRootFeature(Id,title,description,level,parent):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Title',title)
    e.setAttribute('Description',description)
    e.setAttribute('Level',level)
    e.setAttribute('Display','expand')
    e.setAttribute('ConfigurableDirectory','INSTALLDIR')
    parent.appendChild(e)
    return e

def addCRef(Id,parent):
    e = doc.createElement('ComponentRef')
    e.setAttribute('Id',Id)
    parent.appendChild(e)

# adds to parent an element tag with dictionary of attributes attr
def addTo(parent,tag,attr):
    e = doc.createElement(tag)
    for name,value in attr.iteritems():
        e.setAttribute(name,value)
    parent.appendChild(e)
    return e

def fileSearch(Id,name,parent):
    p = addTo(parent,'Property',{'Id':Id})
    e = addTo(p,'FileSearch',{'Id':Id+'_search','LongName':name})
    return e
    
def addText(text,parent):
    e = doc.createTextNode(text)
    parent.appendChild(e)
    return e

# Copies files in nested folders from location to parent directory
# Returns a list of component names to be used in addCRefs
def addCompList(Id,location,name,parent, include_suffix=[],exclude_suffix=[]):
    global globalFileCount
    directory = addDirectory(Id+'_dir','dir',name,parent)
    lst = []
    idir = 0
#    ifil = 0
    if ARCH == '32':
        m = md5.new(location)
    else:
        m = md5.new(location + ARCH)
    u = m.hexdigest()
    uuid = '{'+u[0:8]+'-'+u[8:12]+'-'+u[12:16]+'-'+u[16:20]+'-'+u[20:]+'}'
    comp = addComponent(Id,uuid,directory)
    lst.append(Id)
    files = os.listdir(location)
    for fil in files:
        if (fil.find('.svn') < 0 and fil.find('UNIT_TESTING') < 0):
            if ( os.path.isdir(location+'/'+fil) ):
                idir += 1
                lst = lst + addCompList(Id+'_'+str(idir), location+'/'+fil, fil, directory,include_suffix,exclude_suffix)[0]
            else:
                keep = False
                if len(include_suffix) > 0: 
                    for sfx in include_suffix:
                        if fil.endswith(sfx):
                            keep = True
                            break
                else:
                    keep = True
                if len(exclude_suffix) > 0:
                    for sfx in exclude_suffix:
                        if fil.endswith(sfx):
                            keep = False
                            break
                if keep == False:
                    continue
                globalFileCount += 1
                ifil = globalFileCount
                fn = fil.replace(' ','_')
                fn = fil.replace('-','_')
                fn = fn.replace('+','_')
                fn = fn.replace('.','_')
                fn = fn.replace('#','_')
                fileId = 'd'+fn+'_file'+str(ifil)
                fileName = 'f'+str(ifil)
                fileLongName = fil
                addFileV(fileId,fileName,fileLongName,location+'/'+fil,comp)
    return lst,comp
		
def addCRefs(lstId,parent):
    for Id in lstId:
        e = doc.createElement('ComponentRef')
        e.setAttribute('Id',Id)
        parent.appendChild(e)
 
def createPropertiesFile(filename):
    # Field replacements
    replacements = {
    "plugins.directory" : "plugins.directory = ../plugins",
    "mantidqt.plugins.directory" : "mantidqt.plugins.directory = ../plugins/qtplugins/mantid",
    "instrumentDefinition.directory":"instrumentDefinition.directory = ../instrument",
    "parameterDefinition.directory":"parameterDefinition.directory = ../instrument",    
    "requiredpythonscript.directories":"""requiredpythonscript.directories = ../scripts/Crystallography;../scripts/Disordered Materials;../scripts/Engineering;\\
../scripts/Inelastic;../scripts/Large Scale Structures;../scripts/Molecular Spectroscopy;\\
../scripts/Muons;../scripts/Neutrinos;../scripts/SANS;../scripts/""",
    "pythonscripts.directory":"pythonscripts.directory = ../scripts",
    "mantidqt.python_interfaces_directory":"mantidqt.python_interfaces_directory = ../scripts",
    "pythonscripts.directories":"pythonscripts.directories = ../scripts",
    "pythonalgorithms.directories":"pythonalgorithms.directories=../plugins/PythonAlgs",
    "datasearch.directories" : "datasearch.directories = ../data",
    "icatDownload.directory":"icatDownload.directory = ../data",
    "ManagedWorkspace.FilePath" : "ManagedWorkspace.FilePath = ../temp",
    "logging.channels.fileChannel.path" : "logging.channels.fileChannel.path = ../logs/mantid.log"
    }

    template = open(filename,'r')
    original = template.readlines()
    prop_file = open('Mantid.properties','w')
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

    
doc = xml.dom.minidom.Document()
#doc.encoding('Windows-1252')
wix = doc.createElement('Wix')
wix.setAttribute('xmlns','http://schemas.microsoft.com/wix/2003/01/wi')
doc.appendChild(wix)

Product = doc.createElement('Product')
Product.setAttribute('Id',product_uuid)
Product.setAttribute('Codepage','1252')
Product.setAttribute('UpgradeCode',upgrade_uuid)
Product.setAttribute('Version',MantidVersion)
Product.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
Product.setAttribute('Language','1033')
wix.appendChild(Product)

Package = doc.createElement('Package')
Package.setAttribute('Id',package_uuid)
Package.setAttribute('Keywords','Installer')
Package.setAttribute('Description','Mantid Installer')
#Package.setAttribute('Comments','')
Package.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
Package.setAttribute('Languages','1033')
Package.setAttribute('Compressed','yes')
Package.setAttribute('SummaryCodepage','1252')
#Package.setAttribute('InstallPrivileges','limited')
Product.appendChild(Package)

# Architecture specific stuff
if ARCH == '64':
    Product.setAttribute('Name','Mantid ' + MantidVersion + ' (64-bit)')
    Package.setAttribute('InstallerVersion','200')
    Package.setAttribute('Platforms','x64')
else:
    Product.setAttribute('Name','Mantid ' + MantidVersion)
    Package.setAttribute('InstallerVersion','100')
    Package.setAttribute('Platforms','Intel')

Upgrade = addTo(Product,'Upgrade',{'Id':upgrade_uuid})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'no','Property':'PREVIOUSFOUND','Minimum': '1.0.0','IncludeMinimum':'yes','Maximum':MantidVersion,'IncludeMaximum':'no'})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'yes','Property':'NEWERFOUND','Minimum':MantidVersion,'IncludeMinimum':'no'})

addTo(Product,'CustomAction',{'Id':'NoDowngrade','Error':'A later version of [ProductName] is already installed.'})
exeSec = addTo(Product,'InstallExecuteSequence',{})
NoDowngrade = addTo(exeSec,'Custom',{'Action':'NoDowngrade','After':'FindRelatedProducts'})
addText('NEWERFOUND',NoDowngrade)
addTo(exeSec,'RemoveExistingProducts',{'After':'InstallInitialize'})

Media = doc.createElement('Media')
Media.setAttribute('Id','1')
Media.setAttribute('Cabinet','Mantid.cab')
Media.setAttribute('EmbedCab','yes')
Media.setAttribute('DiskPrompt','CD-ROM #1')
Product.appendChild(Media)

Prop = doc.createElement('Property')
Prop.setAttribute('Id','DiskPrompt')
Prop.setAttribute('Value','Mantid Installation')
Product.appendChild(Prop)


# <Property Id="ASSISTANCE_USERS" Value="cur"/>
# <UI Id="UserUI">
# <Dialog Id="MyDialog" Height="100" Width="100" Hidden="yes">
# <Control Id="Next" Type="PushButton" X="236" Y="243" Width="56" Height="17" Default="yes" Text="Next">
          
          # <Publish Property="ALLUSERS" Value="{}">ASSISTANCE_USERS = "cur"</Publish>  <!-- set null value -->
          # <Publish Property="ALLUSERS" Value="1">ASSISTANCE_USERS = "all"</Publish>
          # </Control>

# </Dialog>
# </UI>

# To produce this we need to write the following!
#
# Wix does not allow elements to have null values despite the MS installer tables
# having conditions that include them.
# To get around Wix we have to use a Publish element that can only be attached to a 
# dialog, which we hide here so that no one actually sees anything different
Prop = doc.createElement('Property')
Prop.setAttribute('Id','ALLUSER_PROXY')
Prop.setAttribute('Value','current')
Product.appendChild(Prop)

ui = doc.createElement('UI')
Product.appendChild(ui)
ui.setAttribute('Id', 'HiddenUI')
dialog = doc.createElement('Dialog')
ui.appendChild(dialog)
dialog.setAttribute('Id', 'HiddenDialog')
dialog.setAttribute('Height','100')
dialog.setAttribute('Width','100')
dialog.setAttribute('Hidden', 'yes') # VERY important
control = doc.createElement('Control')
dialog.appendChild(control)
control.setAttribute('Id','Next')
control.setAttribute('Height','17')
control.setAttribute('Width','56')
control.setAttribute('Type', 'PushButton')
control.setAttribute('X','236')
control.setAttribute('Y', '243')
control.setAttribute('Default', 'yes')
publish = doc.createElement('Publish')
control.appendChild(publish)
publish.setAttribute('Property','ALLUSERS')
publish.setAttribute('Value','{}')
addText(r'ASSISTANCE_USERS = "current"',publish)

TargetDir = addDirectory('TARGETDIR','SourceDir','SourceDir',Product)
InstallDir = addDirectory('INSTALLDIR','MInstall',MantidInstallDir,TargetDir)
binDir = addDirectory('MantidBin','bin','bin',InstallDir)

MantidDlls = addComponent('MantidDLLs',comp_guid['MantidDLLs'],binDir)

# Need to create Mantid.properties file. A template exists but some entries point to the incorrect locations so those need modifying
createPropertiesFile(FRAMEWORKDIR + '/Properties/Mantid.properties.template')
addFileV('MantidProperties','Mantid.pro','Mantid.properties','Mantid.properties',MantidDlls)

MantidScript = addFileV('MantidScript','MScr.bat','MantidScript.bat',FRAMEWORKDIR + '/PythonAPI/MantidScript.bat',MantidDlls)
addTo(MantidScript,'Shortcut',{'Id':'startmenuMantidScript','Directory':'ProgramMenuDir','Name':'Script','LongName':'Mantid Script','WorkingDirectory':'MantidBin'})
addFileV('MantidStartup','MStart.py','MantidStartup.py',FRAMEWORKDIR + '/PythonAPI/MantidStartup.py',MantidDlls)
addFileV('MantidPythonAPI_pyd','MPAPI.pyd','MantidPythonAPI.pyd',MANTIDRELEASE + '/MantidPythonAPI.pyd',MantidDlls)
addFileV('MantidAPI','MAPI.dll','MantidAPI.dll',MANTIDRELEASE + '/MantidAPI.dll',MantidDlls)
addFileV('MantidGeometry','MGeo.dll','MantidGeometry.dll',MANTIDRELEASE + '/MantidGeometry.dll',MantidDlls)
addFileV('MantidKernel','MKern.dll','MantidKernel.dll',MANTIDRELEASE + '/MantidKernel.dll',MantidDlls)
addFileV('MantidNexusCPP','MNxsCPP.dll','MantidNexusCPP.dll',MANTIDRELEASE + '/MantidNexusCPP.dll',MantidDlls)

# Add qt API  library
addFileV('MantidQtAPI','MQTAPI.dll','MantidQtAPI.dll',MANTIDRELEASE + '/MantidQtAPI.dll',MantidDlls)
addFileV('MantidWidgets','MWid.dll','MantidWidgets.dll',MANTIDRELEASE + '/MantidWidgets.dll',MantidDlls)

# Add Qt Property Browser
addFileV('QtPropertyBrowser','QTPB.dll','QtPropertyBrowser.dll',MANTIDRELEASE + '/QtPropertyBrowser.dll',MantidDlls)

# The other required third_party libraries, excluding the designer stuff
addDlls(CODEDIR + '/Third_Party/lib/win' + ARCH,'3dDll',MantidDlls,
        exclude_files=['QtDesigner4.dll','QtDesignerComponents4.dll','QtScript4.dll']) # The designer is notnecessary and is just bloat for the installer
# The C runtime
addDlls(CODEDIR + '/Third_Party/lib/win' + ARCH + '/CRT','CRT',MantidDlls)

#------------- Bundled Python installation ---------------
pythonDLLs = addCompList('PythonDLLs',LIBDIR + '/Python27/DLLs','DLLs',binDir)[0]
pythonLib =  addCompList('PythonLib',LIBDIR + '/Python27/Lib','Lib',binDir,exclude_suffix=['_d.pyd'])[0]
pythonScripts = addCompList('PythonScripts',LIBDIR + '/Python27/Scripts','Scripts',binDir)[0]
# Python executable
addFileV('PyEXE', 'Py27.exe','python.exe',LIBDIR + '/Python27/python.exe',MantidDlls)
# Python dll
addFileV('PyDLL', 'Py27.dll','python27.dll',LIBDIR + '/Python27/python27.dll',MantidDlls)
# Our Framework file
addFileV('MtdFramework_py', 'MFWork.py', 'MantidFramework.py', FRAMEWORKDIR + '/PythonAPI/MantidFramework.py', MantidDlls)
addFileV('MtdSimple_py', 'MSimple.py', 'mantidsimple.py', FRAMEWORKDIR + '/PythonAPI/mantidsimple.py', MantidDlls)

# Our old installation would have left a PyQt4 directory lying around, it must be removed or the new bundled python will
# be confusd
addTo(exeSec,'Custom',{'Action':'cleanup','After':'InstallInitialize'})
addTo(Product,'Property',{'Id':'QtExecCmdLine','Value':'"[SystemFolder]\\cmd.exe" /c rmdir /S /Q "[INSTALLDIR]\\bin\\PyQt4"'})
addTo(Product,'CustomAction',{'Id':'cleanup','BinaryKey':'WixCA','DllEntry':'CAQuietExec','Impersonate':'yes', 'Return':'ignore'})
addTo(Product, 'Binary', {'Id':'wixca', 'src':'wixca.dll'})

#------------- Environment settings ---------------------- 
# MantidPATH to point to the bin directory
addTo(MantidDlls,'Environment',{'Id':'SetMtdPath','Name':'MANTIDPATH','Action':'set','Part':'all','Value':'[INSTALLDIR]bin'})
# Also add binary directory to the path
addTo(MantidDlls,'Environment',{'Id':'AddMtdPath','Name':'PATH','Action':'set','Part':'last','Value':'%MANTIDPATH%'})

# ---------------------- Matlab bindings -------------------------
# Only on 32bit windows for the moment
if ARCH == '32':
    addFileV('MantidMatlabAPI','MMAPI.dll','MantidMatlabAPI.dll',MANTIDRELEASE + '/MantidMatlabAPI.dll',MantidDlls)
    Matlab=addCompList('MatlabMFiles',FRAMEWORKDIR + '/MatlabAPI/mfiles','Matlab',binDir)[0]

    #Add mantid_setup file
    setupfile = open('mantid_setup.m','w')
    setupfile.write('mantid=\'./\';\n')
    setupfile.write('addpath(strcat(mantid,\'Matlab\'),strcat(mantid,\'Matlab/MantidGlobal\'));\n')
    setupfile.write('MantidMatlabAPI(\'SimpleAPI\',\'Create\',\'Matlab\');\n')
    setupfile.write('addpath(strcat(mantid,\'Matlab/MantidSimpleAPI\'));\n')
    setupfile.close()
    addFileV('Matlabsetup','mtd_set','mantid_setup.m','mantid_setup.m',MantidDlls)
else:
    Matlab = []
#---------------------------------------------------------------

QTIPlot = addComponent('QTIPlot',comp_guid['QTIPlot'],binDir)
QTIPlotEXE = addFileV('QTIPlotEXE','MPlot.exe','MantidPlot.exe',MANTIDRELEASE + '/MantidPlot.exe',QTIPlot)
startmenuQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'startmenuQTIPlot','Directory':'ProgramMenuDir','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe'})
desktopQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'desktopQTIPlot','Directory':'DesktopFolder','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe','IconIndex':'0'})
    
addFileV('qtiplotrc', 'qtirc.py', 'qtiplotrc.py', CODEDIR + '/Mantid/MantidPlot/qtiplotrc.py', MantidDlls)
addFileV('qtiplotutil', 'qtiUtil.py', 'qtiUtil.py', CODEDIR + '/Mantid/MantidPlot/qtiUtil.py', MantidDlls)
addFileV('mantidplotrc', 'mtdrc.py', 'mantidplotrc.py', CODEDIR + '/Mantid/MantidPlot/mantidplotrc.py', MantidDlls)
addFileV('mantidplot', 'mtdplot.py', 'mantidplot.py', CODEDIR + '/Mantid/MantidPlot/mantidplot.py', MantidDlls)

# Remove files that may have been created
files_to_remove = ['qtiplotrc.pyc','qtiUtil.pyc','mantidplotrc.pyc','mantidplot.pyc','MantidFramework.pyc','MantidHeader.pyc',\
                   'mantidsimple.py', 'mantidsimple.pyc','mtdpyalgorithm_keywords.txt']
for index, name in enumerate(files_to_remove):
    addTo(MantidDlls,'RemoveFile',{'Id':'RemFile_' + str(index),'On':'uninstall','LongName': name, 'Name':name[:8]})

addTo(MantidDlls,'RemoveFile',{'Id':'LogFile','On':'uninstall','Name':'mantid.log'})
addTo(Product,'Icon',{'Id':'MantidPlot.exe','SourceFile':MANTIDRELEASE + '/MantidPlot.exe'})

#plugins
pluginsDir = addDirectory('PluginsDir','plugins','plugins',InstallDir)
Plugins = addComponent('Plugins',comp_guid['Plugins'],pluginsDir)
addFileV('MantidAlgorithms','MAlg.dll','MantidAlgorithms.dll',MANTIDRELEASE + '/MantidAlgorithms.dll',Plugins)
addFileV('MantidWorkflowAlgorithms','MWAlg.dll','MantidWorkflowAlgorithms.dll',MANTIDRELEASE + '/MantidWorkflowAlgorithms.dll',Plugins)
addFileV('MantidDataHandling','MDH.dll','MantidDataHandling.dll',MANTIDRELEASE + '/MantidDataHandling.dll',Plugins)
addFileV('MantidDataObjects','MDO.dll','MantidDataObjects.dll',MANTIDRELEASE + '/MantidDataObjects.dll',Plugins)
addFileV('MantidCurveFitting','MCF.dll','MantidCurveFitting.dll',MANTIDRELEASE + '/MantidCurveFitting.dll',Plugins)
addFileV('MantidICat','MIC.dll','MantidICat.dll',MANTIDRELEASE + '/MantidICat.dll',Plugins)
addFileV('MantidCrystal','MCR.dll','MantidCrystal.dll',MANTIDRELEASE + '/MantidCrystal.dll',Plugins)
addFileV('MantidNexus','MNex.dll','MantidNexus.dll',MANTIDRELEASE + '/MantidNexus.dll',Plugins)
addFileV('MantidMDDataObjects', 'MMDO.dll', 'MantidMDDataObjects.dll',MANTIDRELEASE + '/MantidMDDataObjects.dll',Plugins) 
addFileV('MantidMDAlgorithms', 'MMDA.dll', 'MantidMDAlgorithms.dll',MANTIDRELEASE + '/MantidMDAlgorithms.dll',Plugins) 
addFileV('MantidMDEvents', 'MMDE.dll', 'MantidMDEvents.dll',MANTIDRELEASE + '/MantidMDEvents.dll',Plugins) 

# Python algorithms
pyalgsList = addCompList("PyAlgsDir",FRAMEWORKDIR + "/PythonAPI/PythonAlgorithms","PythonAlgs",pluginsDir,exclude_suffix=['.pyc'])[0]

##
# Qt plugins
#
# Qt requires several image plugins to be able to load icons such as gif, jpeg and these need to live in
# a directory QTPLUGINSDIR/imageformats
#
# We will have the structure
# --[MantidInstall]/plugins/
#         --qtplugins
#               --imageformats/
#               --MantidQtCustom*.dll
##
qtpluginsDir = addDirectory('QtPlugInsDir','qplugdir','qtplugins',pluginsDir)
qtimageformatsDir = addDirectory('QtImageDllsDir','imgdldir','imageformats',qtpluginsDir)
qtimagedlls = addComponent('QtImagePlugins',comp_guid['QtImagePlugins'],qtimageformatsDir)
addDlls(QTPLUGINDIR + '/imageformats', 'imgdll',qtimagedlls)

# Now we need a file in the main Qt library to tell Qt where the plugins are using the qt.conf file
addSingleFile('./','qt.conf','qtcfile', MantidDlls)

# Qt plugins
mtdqtdllDir = addDirectory('MantidQtPluginsDir','mqtdir','mantid',qtpluginsDir)
mtdqtdlls = addComponent('MantidQtPlugins', comp_guid['MantidQtPlugins'], mtdqtdllDir)
addFileV('MantidQtCustomDialogs','MQTCD.dll','MantidQtCustomDialogs.dll',MANTIDRELEASE + '/MantidQtCustomDialogs.dll',mtdqtdlls)
addFileV('MantidQtCustomInterfaces','MQTCInt.dll','MantidQtCustomInterfaces.dll',MANTIDRELEASE + '/MantidQtCustomInterfaces.dll',mtdqtdlls)

documentsDir = addDirectory('DocumentsDir','docs','docs',InstallDir)
Documents = addComponent('Documents',comp_guid['Documents'],documentsDir)
addTo(Documents,'CreateFolder',{})

logsDir = addDirectory('LogsDir','logs','logs',InstallDir)
Logs = addComponent('Logs',comp_guid['Logs'],logsDir)
addTo(Logs,'CreateFolder',{})

#-------------------  Includes  -------------------------------------
includeDir = addDirectory('IncludeDir','include','include',InstallDir)
IncludeFilesComp = addComponent('IncludeFiles',comp_guid['IncludeFiles'],includeDir)
# API
includeMantidAPIDir = addDirectory('IncludeMantidAPIDir','MAPI','MantidAPI',includeDir)
IncludeMantidAPI = addComponent('IncludeMantidAPI',comp_guid['IncludeMantidAPI'],includeMantidAPIDir)
addAllFiles(FRAMEWORKDIR + '/API/inc/MantidAPI','api',IncludeMantidAPI)
# Geometry
includeMantidGeometryDirList = addCompList('IncludeMantidGeometryDirList',FRAMEWORKDIR + '/Geometry/inc/MantidGeometry','MantidGeometry',includeDir)[0]
# Kernel
includeMantidKernelDir = addDirectory('IncludeMantidKernelDir','KER','MantidKernel',includeDir)
IncludeMantidKernel = addComponent('IncludeMantidKernel',comp_guid['IncludeMantidKernel'],includeMantidKernelDir)
addAllFiles(FRAMEWORKDIR + '/Kernel/inc/MantidKernel','ker',IncludeMantidKernel)
# NexusCPP
includeMantidNexusCPPDir = addDirectory('IncludeMantidNexusCPPDir','NEXCPP','MantidNexusCPP',includeDir)
IncludeMantidNexusCPP = addComponent('IncludeMantidNexusCPP',comp_guid['IncludeMantidNexusCPP'],includeMantidNexusCPPDir)
addAllFiles(FRAMEWORKDIR + '/NexusCPP/inc/MantidNexusCPP','nex',IncludeMantidNexusCPP)
# Other includes
# NeXus API header
addFileV('NAPI', 'napi.h', 'napi.h', CODEDIR + '/Third_Party/include/napi.h', IncludeFilesComp)
boostList = addCompList('boost',CODEDIR + '/Third_Party/include/boost','boost',includeDir)[0]
pocoList = addCompList('poco', CODEDIR + '/Third_Party/include/Poco','Poco',includeDir)[0]

#-------------------  end of Includes ---------------------------------------

sconsList = addCompList('scons',CODEDIR + '/Third_Party/src/scons-local','scons-local',InstallDir)[0]

ins_def_dir = CODEDIR + '/Mantid/instrument'
ins_suffix = '.xml'
instrument_ids, instr_comp = addCompList('instrument',ins_def_dir,'instrument',InstallDir, include_suffix=[ins_suffix])
# At r4214 instrument cache files were moved to be written to managed workspace temp directory
# so here we'll check if old files exist next to the instrument definitions and remove them
idf_files = os.listdir(ins_def_dir)
for index, file in enumerate(idf_files):
    if not file.endswith(ins_suffix): 
        continue
    file = file.rstrip(ins_suffix)
    file += ".vtp"
    addTo(instr_comp,'RemoveFile',{'Id':'RmVTP_' + str(index),'On':'both','LongName': file, 'Name':file[:8]})

tempDir = addDirectory('TempDir','temp','temp',InstallDir)
Temp = addComponent('Temp',comp_guid['Temp'],tempDir)
addTo(Temp,'CreateFolder',{})

dataDir = addDirectory('DataDir','data','data',InstallDir)
Data = addComponent('Data',comp_guid['Data'],dataDir)
addTo(Data,'CreateFolder',{})

#----------------- User Algorithms -------------------------------------
UserAlgorithmsDir = addDirectory('UserAlgorithmsDir','UAlgs','UserAlgorithms',InstallDir)
UserAlgorithms = addComponent('UserAlgorithms',comp_guid['UserAlgorithms'],UserAlgorithmsDir)
#all cpp, h and three specific files
addAllFilesExt(USERALGORITHMSDIR,'ualg','cpp',UserAlgorithms)
addAllFilesExt(USERALGORITHMSDIR,'ualg','h',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'build.bat','UA_build.bat',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'createAlg.py','UA_ca.py',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'SConstruct','UA_Scon',UserAlgorithms)
# Mantid
addFileV('MantidKernel_lib','MKernel.lib','MantidKernel.lib',MANTIDRELEASE + '/MantidKernel.lib',UserAlgorithms)
addFileV('MantidGeometry_lib','MGeo.lib','MantidGeometry.lib',MANTIDRELEASE + '/MantidGeometry.lib',UserAlgorithms)
addFileV('MantidAPI_lib','MAPI.lib','MantidAPI.lib',MANTIDRELEASE + '/MantidAPI.lib',UserAlgorithms)
addFileV('MantidDataObjects_lib','MDObject.lib','MantidDataObjects.lib',MANTIDRELEASE + '/MantidDataObjects.lib',UserAlgorithms)
addFileV('MantidCurveFitting_lib','MFit.lib','MantidCurveFitting.lib',MANTIDRELEASE + '/MantidCurveFitting.lib',UserAlgorithms)
addFileV('poco_foundation_lib','poco_f.lib','PocoFoundation.lib',CODEDIR + '/Third_Party/lib/win' + ARCH + '/PocoFoundation.lib',UserAlgorithms)
addFileV('poco_xml_lib','poco_x.lib','PocoXML.lib',CODEDIR + '/Third_Party/lib/win' + ARCH + '/PocoXML.lib',UserAlgorithms)
addFileV('boost_date_time_lib','boost_dt.lib','boost_date_time-vc100-mt-1_43.lib',CODEDIR + '/Third_Party/lib/win' + ARCH + '/boost_date_time-vc100-mt-1_43.lib',UserAlgorithms)


#-------------------------- Scripts directory and all sub-directories ------------------------------------
scriptsList = addCompList("ScriptsDir", CODEDIR + "/Mantid/scripts","scripts",InstallDir)[0]

#-----------------------------------------------------------------------

#-------------------------- Colormaps ------------------------------------
ColormapsDir = addDirectory('ColormapsDir','colors','colormaps',InstallDir)
Colormaps = addComponent('Colormaps',comp_guid['Colormaps'],ColormapsDir)
addAllFiles(CODEDIR + '/Mantid/Installers/colormaps','col',Colormaps)
#-----------------------------------------------------------------------

ProgramMenuFolder = addDirectory('ProgramMenuFolder','PMenu','Programs',TargetDir)
ProgramMenuDir = addDirectory('ProgramMenuDir','Mantid','Mantid',ProgramMenuFolder)

DesktopFolder = addDirectory('DesktopFolder','Desktop','Desktop',TargetDir)

#-----------------------------------------------------------------------

Complete = addRootFeature('Complete','Mantid','The complete package','1',Product)
MantidExec = addFeature('MantidExecAndDlls','Mantid binaries','The main executable.','1',Complete)
addCRef('MantidDLLs',MantidExec)
addCRefs(pythonDLLs, MantidExec)
addCRefs(pythonLib,MantidExec)
addCRefs(pythonScripts,MantidExec)
addCRef('Plugins',MantidExec)
addCRef('UserAlgorithms',MantidExec)
addCRef('Documents',MantidExec)
addCRef('Logs',MantidExec)
addCRefs(scriptsList,MantidExec)
addCRef('Colormaps',MantidExec)
addCRef('Temp',MantidExec)
addCRef('Data',MantidExec)
addCRefs(Matlab,MantidExec)
addCRefs(instrument_ids,MantidExec)
addCRefs(sconsList,MantidExec)
addCRefs(pyalgsList,MantidExec)
addCRef('QtImagePlugins', MantidExec)
addCRef('MantidQtPlugins', MantidExec)

# Header files
Includes = addFeature('Includes','Includes','Mantid and third party header files.','2',Complete)
addCRef('IncludeFiles', Includes)
addCRef('IncludeMantidAPI',Includes)
addCRefs(includeMantidGeometryDirList,Includes)
addCRef('IncludeMantidKernel',Includes)
addCRef('IncludeMantidNexusCPP',Includes)

addCRefs(boostList,Includes)
addCRefs(pocoList,Includes)

QTIPlotExec = addFeature('QTIPlotExec','MantidPlot','MantidPlot','1',MantidExec)
addCRef('QTIPlot',QTIPlotExec)
addTo(Product,'UIRef',{'Id':'WixUI_FeatureTree'})
addTo(Product,'UIRef',{'Id':'WixUI_ErrorProgressText'})

# Output the file so that the next step in the chain can process it
f = open(WXSFILE,'w')
doc.writexml(f,newl="\r\n")
f.close()
