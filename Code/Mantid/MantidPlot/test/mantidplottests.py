"""
Utility functions for running python test scripts
inside MantidPlot.

Public methods:
    runTests(): to run MantidPlot unit tests
    screenshot(): take a screenshot and save to a report
    
"""
import sys
import os
import unittest
import time
import qti
from PyQt4 import Qt


     
#======================================================================
def _replace_report_text(filename, section, newtext):
    """ Search html report text to 
replace a line <!-- Filename --> etc.
Then, the contents of that section are replaced
@param filename :: full path to .html report
@param section :: string giving the name of the section
@param newtext :: replacement contents of that section. No new lines!
@return the new contents of the entire page 
"""
    # Get the current report contents if any
    if os.path.exists(filename):
        f = open(filename, 'r')
        contents = f.read()
        f.close()
    else:
        contents = ""


    lines = contents.splitlines()
    sections = dict()
    # Find the text in each section
    for line in lines:
        if line.startswith("<!-- "):
            # All lines should!
            n = line.find(" ", 5)
            if n > 0:
                current_section = line[5:n].strip()
                current_text = line[n+3:]
                sections[current_section] = current_text
            
    # Replace the section
    sections[section] = newtext.replace("\n","")
    
    # Make the output
    items = sections.items()
    items.sort()
    output = []
    for (section_name, text) in items:
        output.append("<!-- %s -->%s" % (section_name, text))
    
    # Save the new text
    contents = os.linesep.join(output)
    f = open(filename, 'w')
    f.write(contents)
    f.close()
    



def screenshot(widget, filename, description):
    """ Take a screenshot of the widget for displaying in a html report.
    
    The MANTID_SCREENSHOT_REPORT environment variable must be set 
    to the destination folder. Screenshot taking is skipped otherwise.
    
    @param widget :: QWidget to grab
    @param filename :: Save to this file (no extension!)
    @param description :: Short descriptive text of what the 
            screenshot should look like
    """
    dest = os.getenv('MANTID_SCREENSHOT_REPORT')
    if not dest is None:
        # Create the report directory if needed
        if not os.path.exists(dest):
            os.mkdir(dest)
            
        report = os.path.join(dest, "index.html")
        
        # Find the widget if handled with a proxy
        if hasattr(widget, "_getHeldObject"):
            widget = widget._getHeldObject()
        
        # First save the screenshot
        pix = Qt.QPixmap.grabWidget(widget)
        pix.save(os.path.join(dest, filename+".png"))
        
        # Modify the section in the HTML page
        section_text = '<h2>%s</h2>' % filename
        section_text += '%s<br />' % (description)
        section_text += '<img src="%s.png" alt="%s"></img>' % (filename, description)
        
        _replace_report_text(report, filename, section_text)

def runTests(classname):
    """ Run the test suite in the class.
    Uses the XML runner if the MANTID_SOURCE environment variable was set.
    """
    # Custom code to create and run this single test suite
    suite = unittest.TestSuite()
    suite.addTest( unittest.makeSuite(classname) )
    # Get the XML runner if the environment variable was set
    src = os.getenv('MANTID_SOURCE')
    if src is None:
        runner = unittest.TextTestRunner()
    else:
        sys.path.append( os.path.join(src, "TestingTools/unittest-xml-reporting/src") )
        import xmlrunner
        runner = xmlrunner.XMLTestRunner(output='Testing')
	
    #Run using either runner
    res = runner.run(suite)
    
    # Set Mantid exit code
    if not res.wasSuccessful():
        qti.app.setExitCode(1)
    else:
        qti.app.setExitCode(0)
    
    return res
