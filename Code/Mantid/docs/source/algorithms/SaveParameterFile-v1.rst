.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows instrument parameters to be saved into an
`instrument parameter file <http://mantidproject.org/InstrumentParameterFile>`__.
The parameter file can then be inspected and or modified. It can also be loaded back into
Mantid using the `LoadParameterFile <http://mantidproject.org/LoadParameterFile>`__ algorithm.

The LocationParameters property specifies whether or not to save any calibration parameters
used to adjust the location of any components. Specifically, it will skip "x", "y", "z",
"r-position", "t-position", "p-position", "rotx", "roty", and "rotz" parameters.

Usage
-----

.. testcode::

  import os

  #Create a path in the user's home directory
  filename = os.path.expanduser("~/params.xml")

  #Load a workspace
  ws = Load(Filename = "MAR11001.raw")

  #Save the workspace's instrument's parameters to the given file.
  SaveParameterFile(Workspace = ws, Filename = filename, LocationParameters = False)

.. testcleanup::

   os.remove(filename)

Example Output::

    <?xml version="1.0" encoding="UTF-8"?>
    <parameter-file instrument="InstrumentName" valid-from="1900-01-31T23:59:59">
      <component-link name="ComponentName">
        <parameter name="ParameterName">
          <value val="2.17"/>
        </parameter>
      </component-link>
    </parameter-file>

