import h5py
import numpy
import subprocess
import shutil

class IOmodule(object):
    """
    Class for ABINS I/O HDF file operations.
    """
    def __init__(self, input_filename=None, group_name=None):

        if  isinstance(input_filename, str):

            self._input_filename = input_filename

            # extract name of file from its path.
            begin=0
            while input_filename.find("/") != -1:
                begin = input_filename.find("/") + 1
                input_filename = input_filename[begin:]

            if input_filename == "": raise ValueError("Name of the file cannot be an empty string!")

            _cropped_input_filename = input_filename
        else: raise ValueError("Invalid name of hdf file. String was expected!")

        if  isinstance(group_name, str): self._group_name = group_name
        else: raise ValueError("Invalid name of the group. String was expected!")

        core_name = _cropped_input_filename[0:_cropped_input_filename.find(".")]
        self._hdf_filename = core_name + ".hdf5" # name of hdf file
        self._attributes = {} # attributes for group
        self._numpy_datasets = {} # numpy datasets for group; they are expected to be numpy arrays
        self._structured_datasets = {} # complex data sets which have the form of Python dictionaries or list of Python
                                       # dictionaries

        # Fields which have a form of empty dictionaries have to be set by an inheriting class.


    def eraseHDFfile(self):
        """
        Erases content of hdf file.
        """

        with h5py.File(self._hdf_filename, 'w') as hdf_file:
            pass


    def addAttribute(self, name=None, value=None):
        """
        Adds attribute to the dictionary with other attributes.
        @param name: name of the attribute
        @param value: value of the attribute. More about attributes at: http://docs.h5py.org/en/latest/high/attr.html
        """
        self._attributes[name] = value

    def addStructuredDataset(self, name=None, value=None):
        """
        Adds data in the form of dictionary or a list of dictionaries into the collection of structured datasets.
        @param name: name of dataset
        @param value: dictionary or list of dictionaries is expected
        """
        self._structured_datasets[name] = value

    def addNumpyDataset(self, name=None, value=None):
        """
        Adds dataset in the form of numpy array to the dictionary with the collection of other datasets.
        @param name: name of dataset
        @param value: value of dataset. Numpy array is expected. More about dataset at:
        http://docs.h5py.org/en/latest/high/dataset.html
        """
        self._numpy_datasets[name] = value

    def _save_attributes(self, group=None):
        """
        Saves attributes to hdf file.
        @param group: group to which attributes should be saved.
        """
        for name in self._attributes:
            if isinstance(self._attributes[name], (numpy.int64, int, numpy.float64, str, bytes)):
                group.attrs[name] = self._attributes[name]
            else:
                raise ValueError("Invalid value of attribute. String, int or bytes was expected!")

    def _recursively_save_structured_data_to_group(self, hdf_file=None, path=None, dic=None):
        """
        Helper function for saving structured data into an hdf file.
        @param hdf_file: hdf file object
        @param path: absolute name of the group
        @param dic:  dictionary to be added
        """

        for key, item in dic.items():
            folder = path + key
            if isinstance(item, (numpy.int64, int, numpy.float64, float, str, bytes)):
                if folder in hdf_file:
                    del hdf_file[folder]
                hdf_file[folder] = item
            elif isinstance(item, numpy.ndarray):
                if folder in hdf_file:
                    del hdf_file[folder]
                hdf_file.create_dataset(name=folder, data=item, compression="gzip", compression_opts=9)
            elif isinstance(item, dict):
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=folder + '/', dic=item)
            else:
                raise ValueError('Cannot save %s type'%type(item))


    def _save_structured_datasets(self, hdf_file=None, group_name=None):
        """
        Saves structured data in the form of dictionary or list of dictionaries.
        @param hdf_file: hdf file to which data should be saved
        @param group_name: name of the main group.

        """

        for item in self._structured_datasets:
            if isinstance(self._structured_datasets[item], list):
                num_el = len(self._structured_datasets[item])
                for el in range(num_el):
                    self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=group_name + "/" + item + "/%s/" % el, dic=self._structured_datasets[item][el])
            elif isinstance(self._structured_datasets[item], dict):
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=group_name + "/" + item + "/", dic=self._structured_datasets[item])
            else:
                raise ValueError('Invalid structured dataset. Cannot save %s type'%type(item))


    def _save_numpy_datasets(self, group=None):
        """
        Saves datasets to hdf file.
        @param group: group to which datasets should be saved.
        """
        for name in self._numpy_datasets:
            if isinstance(self._numpy_datasets[name], numpy.ndarray):
                if name in group:
                    del group[name]
                    group.create_dataset(name=name, data=self._numpy_datasets[name], compression="gzip", compression_opts=9)
                else:
                    group.create_dataset(name=name, data=self._numpy_datasets[name], compression="gzip", compression_opts=9)
            else:
                raise ValueError("Invalid dataset. Numpy array was expected!")


    def save(self):
        """
        Saves datasets and attributes to an hdf file.
        """

        with h5py.File(self._hdf_filename, 'a') as hdf_file:
            if not self._group_name in hdf_file:
                hdf_file.create_group(self._group_name)
            group = hdf_file[self._group_name]

            if len(self._structured_datasets.keys())>0: self._save_structured_datasets(hdf_file=hdf_file, group_name=self._group_name)
            if len(self._attributes.keys())>0: self._save_attributes(group=group)
            if len(self._numpy_datasets.keys())>0: self._save_numpy_datasets(group=group)

        # Repack if possible to reclaim disk space
        try:
            subprocess.check_call(["h5repack","-i%s"%self._hdf_filename, "-otemphgfrt.hdf5"])
            shutil.move("temphgfrt.hdf5", self._hdf_filename)
        except OSError:
         pass # repacking failed: no h5repack installed in the system... but we proceed


    def _list_of_str(self, list_str=None):
        """
        Checks if all elements of the list are strings.
        @param list_str: list to check
        @return: True if each entry in the list is a string, otherwise False
        """
        if list_str is None:
            return False

        if not  (isinstance(list_str, list) and all([isinstance(list_str[item], str) for item in range(len(list_str))])):
            raise ValueError("Invalid list of items to load!")

        return True


    def _load_attributes(self, list_of_attributes=None, group=None):
        """
        Loads collection of attributes from the given group.
        @param list_of_attributes:
        @param group:
        @return:
        """

        results = {}
        for item in list_of_attributes:
            results[item] = self._load_attribute(name=item, group=group)

        return results


    def _load_attribute(self, name=None, group=None):
        """
        Loads attribute.
        @param group: group in hdf file
        @param name: name of attribute
        @return:  value of attribute
        """
        if not name in group.attrs:
            raise ValueError("Attribute %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return group.attrs[name]


    def _load_numpy_datasets(self, list_of_numpy_datasets=None, group=None):
        """
        Loads collection datasets from the given group.
        @param group:  group in hdf file
        @param list_of_numpy_datasets: list with names of numpy datasets to be loaded
        @return: dictionary with collection of datasets.
        """

        results = {}
        for item in list_of_numpy_datasets:
            results[item] = self._load_numpy_dataset(name=item, group=group)

        return results


    def _load_numpy_dataset(self, name=None, group=None):
        """
        Loads dataset.
        @param name: name of dataset (dataset is expected to be a numpy array)
        @param group: group in hdf file
        @return: value of dataset
        """
        if not name in group:
            raise ValueError("Dataset %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return numpy.copy(group[name])


    def _load_structured_datasets(self, hdf_file=None, list_of_structured_datasets=None, group=None):
        """
        Loads structured dataset which has a form of Python dictionary directly from hdf file.
        @param hdf_file: hdf file object from which data should be loaded
        @param list_of_structured_datasets:
        @param group:
        @return:
        """

        results={}
        for item in list_of_structured_datasets:
            results[item] = self._load_structured_dataset(hdf_file=hdf_file, name=item, group=group)

        return results


    def _get_subgrp_name(self, path=None):
        """
        Extracts name of the particular subgroup from the absolute name.
        @param path: absolute  name of subgroup
        @return: name of subgroup
        """
        reversed_path = path[::-1]
        end = reversed_path.find("/")
        return reversed_path[:end]


    def _convert_unicode_to_string_core(self, item=None):
        """
        Convert atom element from unicode to str
        @param item: converts unicode to item
        @return: converted element
        """
        assert isinstance(item, unicode)
        return str(item).replace("u'", "'")


    def _convert_unicode_to_str(self, objectToCheck=None):
        """
        Converts unicode to Python str, works for nested dicts and lists (recursive algorithm).

        @param objectToCheck: dictionary, or list with names which should be converted from unicode to string.
        """

        if isinstance(objectToCheck, list):
            for i in range(len(objectToCheck)):
                objectToCheck[i] = self._convert_unicode_to_str(objectToCheck[i])

        elif isinstance(objectToCheck, dict):
            for item in objectToCheck:
                if isinstance(item, unicode):

                    decoded_item = self._convert_unicode_to_string_core(item)
                    item_dict = objectToCheck[item]
                    del objectToCheck[item]
                    objectToCheck[decoded_item] = item_dict
                    item = decoded_item

                objectToCheck[item] = self._convert_unicode_to_str(objectToCheck[item])

        # unicode element
        elif isinstance(objectToCheck, unicode):
            objectToCheck = self._convert_unicode_to_string_core(objectToCheck)

        return objectToCheck


    def _load_structured_dataset(self, hdf_file=None, name=None, group=None):
        """
        Loads one structured dataset.
        @param hdf_file:  hdf file object from which structured dataset should be loaded.
        @param name:  name of dataset
        @param group: name of the main group
        @return:
        """
        if not isinstance(name, str): raise ValueError("Invalid name of the dataset!")

        if name in group:
           _hdf_group= group[name]
        else:
            raise ValueError("Invalid name of the dataset!")


        if all([self._get_subgrp_name(path=_hdf_group[el].name).isdigit() for el in _hdf_group.keys()]):
            _structured_dataset_list = []
            # here we make an assumption about keys which have a numeric values; we assume that always : 1, 2, 3... Max
            _num_keys = len(_hdf_group.keys())
            for item in range(_num_keys):
                _structured_dataset_list.append(self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=_hdf_group.name+"/%s"%item))
            return self._convert_unicode_to_str(objectToCheck=_structured_dataset_list)
        else:
            return self._convert_unicode_to_str(objectToCheck=self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=_hdf_group.name+"/"))


    def _recursively_load_dict_contents_from_group(self, hdf_file=None, path=None):
        """
        Loads structure dataset which has form of Python dictionary.
        @param hdf_file:  hdf file object from which dataset is loaded
        @param path: path to dataset in hdf file
        @return: dictionary which was loaded from hdf file

        """
        ans = {}
        for key, item in hdf_file[path].items():
            if isinstance(item, h5py._hl.dataset.Dataset):
                ans[key] = item.value
            elif isinstance(item, h5py._hl.group.Group):
                ans[key] = self._recursively_load_dict_contents_from_group(hdf_file, path + key + '/')
        return ans


    def load(self, list_of_attributes=None, list_of_numpy_datasets=None, list_of_structured_datasets=None):
        """
        Loads all necessary data.
        @param list_of_attributes: list of attributes to load (list of strings with names of attributes)
        @param list_of_numpy_datasets: list of datasets to load. It is a list of strings with names of datasets.
                                       Datasets have a form of numpy arrays.
        @param list_of_structured_datasets: list of structured datasets. It is a list of strings with names of datasets.
                                            Structured datasets have a form of Python dictionary or list of Python
                                            dictionaries.
        @return: dictionary with both datasets and attributes

        """

        results = {}
        with h5py.File(self._hdf_filename, 'r') as hdf_file:

            if not self._group_name in hdf_file: raise ValueError("No group %s in hdf file!"% self._group_name)

            group = hdf_file[self._group_name]

            if self._list_of_str(list_str=list_of_structured_datasets):
                results["structured_datasets"] = self._load_structured_datasets(hdf_file=hdf_file,
                                                                               list_of_structured_datasets=list_of_structured_datasets,
                                                                               group=group)
            if self._list_of_str(list_str=list_of_attributes):
                results["attributes"] = self._load_attributes(list_of_attributes=list_of_attributes, group=group)

            if self._list_of_str(list_str=list_of_numpy_datasets):
                results["datasets"] = self._load_numpy_datasets(list_of_numpy_datasets=list_of_numpy_datasets, group=group)

        return results

