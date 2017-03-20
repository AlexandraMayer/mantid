.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the input workspace in frida 2.0 format.

Limitations
###########

The input workspace must have an instrument.
The x-Axis must have the unit Energy Transfer.
The y-Axis must be a Soectrum Axis or have the unit Momentum Transfer.


Usage
-----

**Example - y-Axis is Momentum Transfer**

.. testcode:: ExSaveYDA-q


    import os

    #Create workspace with X-Unit = DeltaE, Y-Unit = Momentum Transfer, Instrument and required Sample Log Data
    ws = CreateWorkspace(DataX=range(0,7), DataY=range(0,7), UnitX="DeltaE",VerticalAxisUnit="MomentumTransfer",VerticalAxisValues=range(0,1))

    LoadInstrument(ws,False,InstrumentName="TOFTOF")
    AddSampleLog(ws,"proposal_number","3")
    AddSampleLog(ws, "proposal_title", "A")
    AddSampleLog(ws,"experiment_team","Team name")
    AddSampleLog(ws,"temperature","234.56", LogUnit="F")
    AddSampleLog(ws,"Ei","1.23",LogUnit="meV")

    #Create File
    path = os.path.expanduser("~/saveyda.txt")

    #execute algorithm
    SaveYDA(ws,Filename=path)

    print "File exists: ", os.path.isfile(path)

    with open( path, 'r') as f:
       print f.read()


.. testoutput:: ExSaveYDA-q

    File exists:  True
    Meta:
      format: yaml/frida 2.0
      type: generic tabular data
    History:
      - Proposal number 3
      - A
      - Team name
      - data reduced with mantid
    Coord:
      x: {name: w, unit: meV}
      y: {name: "S(q,w)", unit: meV-1}
      z: {name: q, unit: A-1}
    RPar:
      - name: T
        unit: K
        val: 234.56
        stdv: 0
      - name: Ei
        unit: meV
        val: 1.23
        stdv: 0
    Slices:
      - j: 0
        z: [{val: 0}]
        x: [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
        y: [0, 1, 2, 3, 4, 5, 6]

.. testcleanup::ExSaveYDA-q

    os.remove(path)
    DeleteWorkspace(ws)

**Example - y-Axis is Spectrum Axis**

.. testcode:: ExSaveYDA-Spectrum

    import os

    ws = CreateWorkspace(DataX=range(0,7), DataY=range(0,7), UnitX="DeltaE")
    LoadInstrument(ws,False,InstrumentName="TOFTOF")
    AddSampleLog(ws,"proposal_number","3")
    AddSampleLog(ws, "proposal_title", "A")
    AddSampleLog(ws,"experiment_team","Team name")
    AddSampleLog(ws,"temperature","234.56", LogUnit="F")
    AddSampleLog(ws,"Ei","1.23",LogUnit="meV")



    path = os.path.expanduser("~/saveyda.txt")


    SaveYDA(ws,Filename=path)

    print "File exists: ", os.path.isfile(path)

    with open( path, 'r') as f:
       print f.read()

.. testoutput:: ExSaveYDA-Spectrum

    File exists:  True
    Meta:
      format: yaml/frida 2.0
      type: generic tabular data
    History:
      - Proposal number 3
      - A
      - Team name
      - data reduced with mantid
    Coord:
      x: {name: w, unit: meV}
      y: {name: "S(q,w)", unit: meV-1}
      z: {name: 2th, unit: deg}
    RPar:
      - name: T
        unit: K
        val: 234.56
        stdv: 0
      - name: Ei
        unit: meV
        val: 1.23
        stdv: 0
    Slices:
      - j: 0
        z: [{val: 14.14999999999997}]
        x: [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
        y: [0, 1, 2, 3, 4, 5, 6]

.. testcleanup::ExSaveYDA-Spectrum

    os.remove(path)
    DeleteWorkspace(ws)


.. categories::

.. sourcelink::


