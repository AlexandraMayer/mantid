import os
from MantidFramework import *
from mantidsimple import *

def find_file(filename=None, startswith=None, data_dir=None):
    """
        Returns a list of file paths for the search criteria.
        @param filename: exact name of a file. The first file found will be returned.
        @param startswith: string that files should start with.
        @param data_dir: additional directory to search
    """
    # Files found
    files_found = []
    
    # List of directory to look into
    # The preferred way to operate is to have a symbolic link in a work directory,
    # so look in the current working directory first
    search_dirs = [os.getcwd()]
    # The second best location would be with the data itself
    if data_dir is not None:
        search_dirs.append(data_dir)
    # The standard place would be the location of the configuration files on the SNS mount
    search_dirs.append("/SNS/EQSANS/shared/instrument_configuration/")
    search_dirs.extend(ConfigService().getDataSearchDirs())
    
    # Look for specific file
    if filename is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            file_path = os.path.join(os.path.normcase(d), filename)
            if os.path.isfile(file_path):
                files_found.append(file_path)
                # If we are looking for a specific file, return right after we find the first
                if startswith is None:
                    return files_found

    # Look for files that starts with a specific string
    if startswith is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            files = os.listdir(d)
            for file in files:
                if file.startswith(startswith):
                    file_path = os.path.join(os.path.normcase(d), file)
                    files_found.append(file_path)

    return files_found

def find_data(file, instrument=''):
    """
        Finds a file path for the specified data set, which can either be:
            - a run number
            - an absolute path
            - a file name
        @param file: file name or part of a file name
        @param instrument: if supplied, FindNeXus will be tried as a last resort
    """
    # First, assume a file name
    file = str(file)
    instrument = str(instrument)
    file_path = FileFinder.getFullPath(file)
    if file_path is not None and len(file_path)>0:
        return file_path

    # Second, assume a run number    
    try:
        # FileFinder doesn't like dashes...
        instrument=instrument.replace('-','')
        f = FileFinder.findRuns(instrument+file)
        if os.path.isfile(f[0]):
            return f[0]
    except:
        # FileFinder couldn't make sense of the the supplied information
        pass

    # If we didn't find anything, raise an exception
    raise RuntimeError, "Could not find a file for [%s]" % str(file)
