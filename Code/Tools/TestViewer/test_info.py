""" Classes describing test projects,
how they are run,
and interpreting the results"""
import time
import datetime
import os
import commands
import tempfile
import shutil
from xml.dom.minidom import parse, parseString
import multiprocessing
from multiprocessing import Pool


#==================================================================================================
class TestResultState:
    """Enumeration..."""
    """ Test was not run since the program started """
    NOT_RUN = 0
    """ Test passed """
    PASSED = 1
    """ Test failed """
    FAILED = 2
    """ Build error ! """
    BUILD_ERROR = 3
    """ Passed the last time it was run, but is out of date now """
    PASSED_OLD = 4
    """ Failed the last time it was run, but is out of date now """
    FAILED_OLD = 5
    """ Compilation last time it was run """
    BUILD_ERROR_OLD = 6


#==================================================================================================
class TestSingle(object):
    """ A single test instance (one test inside one suite) """
    def __init__(self, name):
        self.name = name
        
        # Starting test state
        self.state = TestResultState.NOT_RUN
        
        # Last date and time the test was run
        self.lastrun = None
        
        # Time (in seconds) to execute the test
        self.runtime = 0.0
        
        # Last failure text
        self.failure = ""
        # Line in the file of the failure
        self.failure_line = 0
        
        # Stdout output for that failure 
        self.stdout = "" 
        
    def load_results(self, case):
        """Load the results from a xml Junit file 
        Parameters
            case : a xml.Node object containing the testcase xml results """
        # Get the runtime
        self.runtime = float(case.getAttribute("time"))
        # Assumed passed
        self.state = TestResultState.PASSED
        # Look for failures
        fails = case.getElementsByTagName("failure")
        
        if len(fails)>0:
            self.state = TestResultState.FAILED
            # File and line of failure
            file = fails[0].getAttribute("file")
            self.failure_line = fails[0].getAttribute("line")
            # Get the failure text
            self.failure = fails[0].firstChild.data

        # Get the system output
        systemout = case.getElementsByTagName("system-out")
        if len(systemout) > 0:
            # This is a node containing text (the firstchild) which is a Text node
            self.stdout = systemout[0].firstChild.data
            
            
        
        
    def __repr__(self):
        return "TestSingle(%s): state=%d, lastrun=%s, runtime=%s.\n%s" % (self.name, self.state, self.lastrun, self.runtime, self.stdout)
        

#==================================================================================================
class TestSuite(object):
    """ A suite of tests """

    def __init__(self, name, parent_name, classname, command, xml_file, source_file):
        """ Constructor"""
        # Its own name, e.g. "UnitTest"
        self.name = name
        # Name of the parent project, e.g. "KernelTest"
        self.parent_name = parent_name
        # Full class name, e.g. KernelTest.UnitTest
        self.classname = classname
        # Full command that runs the test suite
        self.command = command
        # Name of the XML file produced when running (no path)
        self.xml_file = xml_file
        # Source file (BlaBlaTest.h) for this suite
        self.source_file = source_file
        # A list of test singles inside this suite
        self.tests = []
        # Is it selected to run?
        self.selected = True
        # Was it made correctly?
        self.built_succeeded = True
        
        
    #----------------------------------------------------------------------------------
    def add_single(self, test_name):
        """ Add a single test to this suite """
        self.tests.append( TestSingle(test_name) )
        
    #----------------------------------------------------------------------------------
    def is_built(self):
        """Returns True if the test build for this suite was successful."""
        return self.built_succeeded
    
    #----------------------------------------------------------------------------------
    def set_build_failed(self, output):
        """Sets that the build failed for all single tests in this suite.
        Parameters:
            output: stdout from the make command
        """
        self.built_succeeded = False
        for test in self.tests:
            test.state = TestResultState.BUILD_ERROR
            test.failure = "Build failure"
            test.stdout = output
        
    #----------------------------------------------------------------------------------
    def run_tests(self):
        """ Runs this test suite, then loads the produced XML file
        and interprets its results.
        This method should be written so that it can be run in parallel. """
        # Present working directory
        pwd = os.getcwd()
        
        # Create a temporary directory just for running this test suite
        tempdir = tempfile.mkdtemp()
        os.chdir(tempdir)
        
        # Execute the test command; wait for it to return
        output = commands.getoutput( self.command )
        
        # Get the output XML filename
        xml_path = os.path.join(tempdir, self.xml_file)
        self.parse_xml(xml_path) 
        
        # Go back to old directory and remove the temp one
        os.chdir(pwd)
        try:
            shutil.rmtree(tempdir)
        except:
            print "Error removing temporary directory ", pathtempdir  
        
        
    #----------------------------------------------------------------------------------
    def find_test(self, test_name):
        """Find and return a TestSingle instance of given name"""
        for test in self.tests:
            if test.name == test_name:
                return test
        return None
        

    #----------------------------------------------------------------------------------
    def parse_xml(self, xml_path):
        """Interpret a jUnit-style XML file
        Parameters
            xml_path :: full path to the produced XML path"""
            
        this_rundate = datetime.datetime.now()
           
        try:
            dom = parse(xml_path)
        except:
            # Empty file, for example? Just return
            return
        
        #print dom.getElementsByTagName(self.name)
        suites = dom.getElementsByTagName("testsuite")
        if len(suites) == 0:
            return
        elif len(suites) > 1:
            for xmlSuite in suites:
                if (suites[0].getAttribute("name") == self.name):
                    break
        else:
            xmlSuite = suites[0]
        
        # Get all the test cases
        xmlCases = xmlSuite.getElementsByTagName("testcase")
        for case in xmlCases:
            classname = case.getAttribute("classname")
            if (classname == self.classname):
                # This is the single test name
                test_name = case.getAttribute("name")
                test = self.find_test(test_name)
                if not test is None:
                    # Save the time
                    test.lastrun = this_rundate
                    test.load_results(case)
            
    #----------------------------------------------------------------------------------
    def __repr__(self):
        return "TestSuite(%s) with %d TestSingle(s).\nCommand=%s\nXML File=%s\nSource file=%s" % (self.name, len(self.tests), self.command, self.xml_file, self.source_file)
        


#==================================================================================================
class TestProject(object):
    """ A sub-project of several test suites, e.g. KernelTest """
    
    #----------------------------------------------------------------------------------
    def __init__(self, name, executable, make_command):
        self.name = name
        
        # Path to the executable command
        self.executable = executable
        
        # Command that will build the given executable
        self.make_command = make_command
        
        # Test suites in this project
        self.suites = []
        
    #----------------------------------------------------------------------------------
    def make(self):
        """Make the project using the saved command """
        # print "making test : %s" % self.make_command
        (status, output) = commands.getstatusoutput(self.make_command)
        if (status != 0):
            print "! Build failure for %s !" % self.name
            # Build failure of some kind!
            for suite in self.suites:
                suite.set_build_failed(output)
        else:
            # Build was successful
            for suite in self.suites:
                suite.build_succeeded = True
        
        
    #----------------------------------------------------------------------------------
    def find_source_file(self, suite_name):
        """ Find the source file corresponding to the given suite in this project
        Returns: the full path to the test file.
        """
        source_base_dir = "/home/8oz/Code/Mantid/Code/Mantid/Framework/"
        project_name = self.name.replace("Test", "")
        source_dir = os.path.join(source_base_dir, project_name)
        return os.path.join( source_dir, "test/" + suite_name + ".h")
    
    
    #----------------------------------------------------------------------------------
    def is_anything_selected(self):
        """Return True if any of the suites are selected."""
        for suite in self.suites:
            if suite.selected:
                return True
        return False
        
        
    #----------------------------------------------------------------------------------
    def populate(self):
        """ Discover the suites and single tests in this test project. """
        self.suites = []
        
        # CXX test simply lists "TestSuite testName"
        last_suite_name = ""
        suite = None
        
        # Get the bare XML file name
        (dir, file) = os.path.split(self.executable)
        xml_file = "TEST-" + file + ".xml"        
        
        output = commands.getoutput(self.executable + " --help-tests")
        # The silly cxxtest makes an empty XML file
        try:
            os.remove(xml_file)
        except:
            pass
                
        lines =  output.split("\n")
        # Look for the The first two lines are headers
        count = 0
        while count < len(lines) and not lines[count].startswith("------------------------------------------------------"):
            count += 1
        count += 1
        if count >= len(lines):
            raise "Error interpreting CXX test output!"
        
        lines = lines[count:]
        
        for line in lines:
            words = line.split()
            if len(words) == 2:
                suite_name = words[0]
                test_name = words[1]
                if suite_name != "" and test_name != "":
                    # Are we making a new suite?
                    if last_suite_name != suite_name:

                        # The class name goes KernelTest.DateAndTimeTest
                        classname = self.name + "." + suite_name
                        source_file = self.find_source_file(suite_name)
                        # Create that suite
                        suite = TestSuite(suite_name, self.name, classname, 
                                          self.executable + " " + suite_name, xml_file, source_file)
                        last_suite_name = suite_name
                        self.suites.append(suite)
                        
                    # Add a single test to whatever suite we are in
                    suite.add_single(test_name)
            else:
                # Could not interpret line
                pass
            
    def __repr__(self):
        return "TestProject(%s)" % (self.name)


#==================================================================================================
#======== Global methods used by parallel processing ================
#==================================================================================================


#==================================================================================================
def run_tests_in_suite(multiple_tests, suite ):
    """Run all tests in a given suite. Method called
    by the multiprocessing Pool.apply_async() method.
    
    Parameters:
        multiple_tests :: a MultipleProjects instance calling this method.
        suite :: the suite to run
    """
    if not multiple_tests is None: 
        if multiple_tests.abort_run: return "Aborted."
    if not suite is None:
        suite.run_tests()
    # Simply return the object back (for use by the callback function)
    return suite
        
#==================================================================================================
def make_test(multiple_tests, project):
    """Make the tests in a given project. Method called
    by the multiprocessing Pool.apply_async() method.
    
    Parameters:
        multiple_tests :: a MultipleProjects instance calling this method.
        project :: the project to make
    Returns:
        the project that was just made. Some values will have been set in it;
        the callback function must replace the old project with this one; because
        the changes happened in the OTHER thread!
    """
    if not multiple_tests is None: 
        if multiple_tests.abort_run: return "Aborted."
    if not project is None:
        project.make()
        return project
    else:
        return None
     
     
     
     
#==================================================================================================
#==================================================================================================
#==================================================================================================
class MultipleProjects(object):
    """ A Class containing a list of all the available test projects.
    This will be made into a single global variable instance. """
    
    #--------------------------------------------------------------------------        
    def __init__(self):
        # The projects contained
        self.projects = []
        # Abort flag
        self.abort_run = False
   
    #--------------------------------------------------------------------------        
    def abort(self):
        """ Set a flag to abort all further calculations. """
        print "... Attempting to abort ..."
        self.abort_run = True
    
    #--------------------------------------------------------------------------        
    def discover_CXX_projects(self, path, source_path):
        """Look for CXXTest projects in the given paths.
        Populates all the test in it."""
        self.projects = []
        dirList=os.listdir(path)
        for fname in dirList:
            # Look for executables ending in Test
            if fname.endswith("Test") and fname.startswith("API"): #!TODO
                make_command = "cd %s ; make %s" % (os.path.join(path, ".."), fname)
                pj = TestProject(fname, os.path.join(path, fname), make_command)
                print "... Populating project %s ..." % fname
                pj.populate()
                self.projects.append(pj)
        

    #--------------------------------------------------------------------------        
    def get_project_named(self, name):
        """Return the TestProject named name; None if not found"""
        for pj in self.projects:
            if pj.name == name:
                return pj
        return None

    #--------------------------------------------------------------------------        
    def replace_project(self, pj):
        """Given a project from another thread, replace the current one with this one.
        Will look for a matching name"""
        for i in xrange(len(self.projects)):
            if self.projects[i].name == pj.name:
                self.projects[i] = pj
                break
            
    #--------------------------------------------------------------------------        
    def replace_suite(self, st):
        """Given a suite from another thread, replace the current one with this one.
        Will look for a matching name"""
        pj = self.get_project_named(st.parent_name)
        if pj is None:
            return
        for i in xrange(len(pj.suites)):
            suite = pj.suites[i]
            if suite.name == st.name:
                pj.suites[i] = st
                break
        
    #--------------------------------------------------------------------------        
    def get_selected_suites(self, selected_only=True):
        """Returns a list of all selected suites. Suites where make failed
        are excluded!
        
        Parameters:
            selected_only :: set to False to return all suites """
        suites = []
        for pj in self.projects:
            for st in pj.suites:
                # print "get_selected_suites: status of ", st.classname, st.is_built()
                if st.is_built() and (st.selected or (not selected_only)):
                    #print "get_selected_suites adding ", st.classname
                    # Suite must be built to be included here!
                    suites.append(st)
        # print "get_selected_suites ", len(suites)
        return suites             
        
    #--------------------------------------------------------------------------        
    def run_tests_computation_steps(self, selected_only=True, make_tests=True):
        """Returns the number of computation steps that will be done with these parameters
        This is used by the GUI to know how to report progress."""
        count = 0
        if make_tests:
            for pj in self.projects:
                if pj.is_anything_selected() or (not selected_only):
                    count += 1
        count += len(self.get_selected_suites(selected_only))
        return count
                                

    #--------------------------------------------------------------------------        
    def run_tests_in_parallel(self, selected_only=True, make_tests=True, parallel=True, callback_func=None):
        """Run tests in parallel using multiprocessing.
        Parameters:
            selected_only: run only the selected suites.
            make_tests: set to True to make the tests before running them.
            parallel: set to True to do them in parallel, false to do linearly 
            callback_func: function that takes as argument the suite that was just finished
                or the project that was just made.
                This function MUST replace the original object in order to synchronize the 
                threads' data.
        """
        self.abort_run = False
        
        start = time.time()
        
        # How many thread in parallel?  one fewer threads than the # of cpus
        num_threads = multiprocessing.cpu_count()-1
        if num_threads < 1: num_threads = 1
                    
        # Now let's run the make test command for each one
        if make_tests:
            pj_to_build = []
            for pj in self.projects:
                if pj.is_anything_selected() or (not selected_only):
                    pj_to_build.append(pj)
                    
            if parallel:
                p = Pool(num_threads)
                for pj in pj_to_build:
                    p.apply_async( make_test, (self, pj, ), callback=callback_func)
                p.close()
                # This will block until completed
                p.join()
            else:
                for pj in pj_to_build:
                    result = make_test( self, pj )
                    if not callback_func is None: callback_func(result)
                    
        
        # Build a long list of all (selected and successfully built) suites
        suites = self.get_selected_suites(selected_only)
            
        if parallel:
            # Make the pool 
            p = Pool( num_threads )
            
            # Call the method that will run each suite
            for suite in suites:
                p.apply_async( run_tests_in_suite, (self, suite, ), callback=callback_func)
            p.close()
            # This will block until completed
            p.join()
            
        else:
            for suite in suites:
                result = run_tests_in_suite( self, suite )
                if not callback_func is None: callback_func(result)
        print "... %s tests %sand completed in %f seconds ..." % (["All", "Selected"][selected_only], ["","built "][parallel],  (time.time() - start))

  
                      
# Global variable containing a list of all the available test projects
global all_tests        
all_tests = MultipleProjects()        

#==================================================================================================
def test_run_print_callback(obj):
    """ Simple callback for running tests. This is called into the MainProcess.
    
    Parameters:
        obj :: the object, either a TestSuite or TestProject that was just calculated
    """
    global all_tests
    if isinstance(obj, TestSuite):
        suite = obj
        print "Done running %s" % suite.classname
        all_tests.replace_suite(suite)
        
    elif isinstance(obj, TestProject):
        pj = obj
        # Replace the project in THIS thread!
        all_tests.replace_project( pj )
        print "Made project %s" % pj.name                
        
        
        
#==================================================================================================
if __name__ == '__main__':
    all_tests.discover_CXX_projects("/home/8oz/Code/Mantid/Code/Mantid/bin/", "/home/8oz/Code/Mantid/Code/Mantid/Framework/")
    all_tests.run_tests_in_parallel(selected_only=False, make_tests=True, 
                          parallel=True, callback_func=test_run_print_callback)
    pj = all_tests.get_project_named("KernelTest")
    assert not pj.suites[0].is_built()
    print pj.suites[0].tests[0].state
#    all_tests.run_tests_in_parallel(selected_only=False, make_tests=True, 
#                          parallel=False, callback_func=test_run_print_callback)


#==================================================================================================
def other_test():
    # Make a sample test project
    kt = TestProject("GeometryTest", "/home/8oz/Code/Mantid/Code/Mantid/bin/GeometryTest", "")
    kt.populate()
    #print kt.suites
    print "----"
    kt.suites[16].run_tests()
        
        
        