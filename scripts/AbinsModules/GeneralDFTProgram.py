import hashlib

# ABINS modules
from IOmodule import IOmodule
from KpointsData import KpointsData
from AtomsData import  AtomsDaTa
from AbinsData import AbinsData
import AbinsParameters

class GeneralDFTProgram(IOmodule):
    """
    A general class which groups all methods which should be inherited or implemented by a DFT program used
    in INS analysis.
    """

    def __init__(self, input_DFT_filename=None):

        super(GeneralDFTProgram, self).__init__(input_filename=input_DFT_filename, group_name=AbinsParameters.DFT_group)

        self._num_k = None
        self._num_atoms = None


    def readPhononFile(self):
        """
        This method is different for different DFT programs. It has to be overridden by inheriting class.
        This method should do the following:

          1) Open file with phonon data (CASTEP: foo.phonon). Name of a file should be stored in self._input_filename.
          Only one dot '.' is expected in the name of a file. There must be no spaces in the name of a file. Extension
          of file (part of a name after '.') is arbitrary.

          2) Method should read from a phonon file an information about frequencies, atomic displacements,
          k-point vectors, weights of k-points and ions.

          3) Method should reconstruct data for symmetry equivalent k-points (protected method _recoverSymmetryPoints).

             **Notice: this step is not implemented now. At the moment only Gamma point calculations are supported.**

          4) Method should determine symmetry equivalent atoms

              **Notice: this step is not implemented now.**

          5) Method should calculate hash of a file with phonon data (protected method _calculateHash).

          6) Method should store phonon data in an hdf file (inherited method save()). The name of an hdf file is
          foo.hdf5 (CASTEP: foo.phonon -> foo.hdf5). In order to save the data to hdf file the following fields
          should be set:

                    self._hdf_filename
                    self._group_name
                    self._attributes
                    self._datasets

              The datasets should be a dictionary with the following entries:

                        "frequencies"  - frequencies for all k-points grouped in one numpy.array

                        "weights"      - weights of all k-points in one numpy.array

                        "k_vectors"    - all k-points in one numpy array

                                         **Notice: both symmetry equivalent and inequivalent points should be stored; at
                                          the moment only Gamma point calculations are supported**

                        "atomic_displacements" - atomic displacements for all atoms and all k-points in one numpy array
                        
                        "unit_cell"      -   numpy array with unit cell vectors

              The following structured datasets should be also defined:

                        "atoms"          - Python list with the information about ions. Each entry in the list is a
                                          dictionary with the following entries:

                                               "symbol" - chemical symbol of the element (for example hydrogen -> H)

                                               "sort"   - defines symmetry equivalent atoms, e.g, atoms with the same
                                                          sort are symmetry equivalent

                                                          **Notice at the moment this parameter is not functional
                                                            in LoadCastep**

                                               "fract_coord" - equilibrium position of atom; it has a form of numpy
                                                               array with three floats

                                               "atom" - number of atom in the unit cell

                                               "mass" - mass of atom


              The attributes should be an dictionary with the following entries:

                        "hash"  - hash of a file with the phonon data. It should be a string representation of hash.

                        "DFT_program" - name of the DFT program which was used to obtain phonon data (for CASTEP -> CASTEP).

                        "filename" - name of input DFT file

          For more details about these fields please look at the documentation of IOmodule class.

        @return: Method should return an object of type AbinsData.

        """
        return None


    def validData(self):
        """
        Checks if input DFT file and content of HDF file are consistent.
        @return: True if consistent, otherwise False.
        """
        current_hash = self._calculateHash()
        saved_hash = None
        try:
            saved_hash = self.load(list_of_attributes=["hash"])
        except ValueError:
            return False # no hash was found

        return current_hash == saved_hash["attributes"]["hash"]


    def loadData(self):
        """
        Loads data from hdf file.
        @return:
        """
        _data = self.load(list_of_numpy_datasets=["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell"],
                          list_of_structured_datasets=["atoms"])
        _datasets = _data["datasets"]
        _atoms_data = _data["structured_datasets"]["atoms"]
        self._num_k = _datasets["k_vectors"].shape[0]
        self._num_atoms = len(_atoms_data)

        _loaded_data = {"frequencies":_datasets["frequencies"],
                        "weights": _datasets["weights"],
                        "k_vectors":_datasets["k_vectors"],
                        "atomic_displacements": _datasets["atomic_displacements"],
                        "unit_cell": _datasets["unit_cell"],
                        "atoms":_atoms_data
                        }

        return self._rearrange_data(data=_loaded_data)


    # Protected methods which should be reused by classes which read DFT phonon data
    def _recoverSymmetryPoints(self, data=None):
        """
        This method reconstructs symmetry equivalent k-points.
        @param data: dictionary with the data for only symmetry inequivalent k-points. This methods
        adds to this dictionary phonon data for symmetry equivalent k-points.
        """

        pass


    def _calculateHash(self):
        """
        This method calculates hash of the phonon file according to SHA-2 algorithm from hashlib library: sha512.
        @return: string representation of hash for phonon file which contains only hexadecimal digits
        """

        buf = 65536  # chop content of phonon file into 64kb chunks to minimize memory consumption for hash creation
        sha = hashlib.sha512()

        with open(self._input_filename, 'rU') as f:
            while True:
                data = f.read(buf)
                if not data:
                    break
                sha.update(data)

        return sha.hexdigest()


    def _rearrange_data(self, data=None):
        """
        This method rearranges data read from phonon DFT file. It converts  masses and frequencies Hartree atomic units.

        @param k_data: dictionary with the data to rearrange
        @return: Returns an object of type AbinsData
        """
        _number_of_phonons = 3 * self._num_atoms
        k_points = KpointsData(num_atoms=self._num_atoms, num_k=self._num_k)

        atoms = AtomsDaTa(num_atoms=self._num_atoms)
        for atom in data["atoms"]:
            atom["mass"] = atom["mass"] * AbinsParameters.m_2_hartree
        atoms.set(data["atoms"])

        k_points.set({"weights": data["weights"],  # 1D [k] (one entry corresponds to weight of one k-point)
                      "k_vectors": data["k_vectors"],  # 2D [k][3] (one entry corresponds to one coordinate of particular k-point)
                      "frequencies": data["frequencies"] * AbinsParameters.cm1_2_hartree,  # 2D  array [k][freq] (one entry corresponds to one frequency for the k-point k)
                      "atomic_displacements": data["atomic_displacements"]}) # 4D array [k][atom_n][freq][3] (one entry corresponds to one coordinate for atom atom_n, frequency  freq and k-point k )
        _data = AbinsData()
        _data.set(k_points_data=k_points, atoms_data=atoms)
        return _data


