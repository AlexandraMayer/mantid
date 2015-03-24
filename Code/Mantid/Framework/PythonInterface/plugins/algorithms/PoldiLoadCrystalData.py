# pylint: disable=no-init,invalid-name,bare-except
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *

from pyparsing import *

import os


class PoldiCompound(object):
    _name = ""
    _spacegroup = ""
    _atomString = ""
    _cellDict = ""

    def __init__(self, name, elements):
        self._name = name

        self.assign(elements)

    def assign(self, elements):
        for c in elements:
            if c[0] == "atoms":
                self._atomString = ';'.join(c[1])
            elif c[0] == "lattice":
                cellNames = ['a', 'b', 'c', 'alpha', 'beta', 'gamma']
                self._cellDict = dict(zip(cellNames, c[1:]))
            elif c[0] == "spacegroup":
                self._spacegroup = c[1]

    def getAtomString(self):
        return self._atomString

    def getCellParameters(self):
        return self._cellDict

    def getSpaceGroup(self):
        return self._spacegroup

    def getName(self):
        return self._name


class PoldiCrystalFileParser(object):
    elementSymbol = Word(alphas, exact=2)
    integerNumber = Word(nums)
    decimalSeparator = Literal('.')
    floatNumber = Combine(
        integerNumber +
        Optional(decimalSeparator + Optional(integerNumber))
    )

    atomLine = Combine(
        elementSymbol + Suppress(White()) +
        delimitedList(floatNumber, delim=White()),
        joinString=' '
    )

    keyValueSeparator = Suppress(Literal(":"))

    atomsGroup = Group(CaselessLiteral("atoms") + keyValueSeparator + nestedExpr(
        opener="{", closer="}",
        content=Combine(
            delimitedList(atomLine, delim='\n'),
            joinString=";")))

    unitCell = Group(CaselessLiteral("lattice") + keyValueSeparator + delimitedList(
        floatNumber, delim=White()))

    spaceGroup = Group(CaselessLiteral("spacegroup") + keyValueSeparator + Word(
        alphanums + "-" + ' '))

    compoundContent = Each([atomsGroup, unitCell, spaceGroup])

    compoundName = Word(alphanums)

    compound = Group(compoundName + Optional(Suppress(White())) + \
                     nestedExpr(opener='{', closer='}', content=compoundContent))

    comment = Suppress(Literal('#') + restOfLine)

    compounds = OneOrMore(compound).ignore(comment)

    def __call__(self, contentString):
        parsedContent = None

        if os.path.isfile(contentString):
            parsedContent = self._parseFile(contentString)
        else:
            parsedContent = self._parseString(contentString)

        return [PoldiCompound(*x[:4]) for x in parsedContent]

    def _parseFile(self, filename):
        return self.compounds.parseFile(filename)

    def _parseString(self, stringContent):
        return self.compounds.parseString(stringContent)


class PoldiLoadCrystalData(PythonAlgorithm):
    _parser = PoldiCrystalFileParser()

    def category(self):
        return "SINQ\\POLDI"

    def name(self):
        return "PoldiLoadCrystalData"

    def summary(self):
        return ("The algorithm reads a POLDI crystal structure file and creates a WorkspaceGroup that contains tables"
                "with the expected reflections.")

    def PyInit(self):
        self.declareProperty(
            FileProperty(name="InputFile",
                         defaultValue="",
                         action=FileAction.Load,
                         extensions=["dat"]),
            doc="A file with POLDI crystal data.")

        self.declareProperty("LatticeSpacingMin", 0.0,
                             direction=Direction.Input,
                             doc="Lowest allowed lattice spacing.")

        self.declareProperty("LatticeSpacingMax", 0.0,
                             direction=Direction.Input,
                             doc="Lowest allowed lattice spacing.")

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace",
                              defaultValue="", direction=Direction.Output),
            doc="WorkspaceGroup with reflection tables.")


    def PyExec(self):
        crystalFileName = self.getProperty("InputFile").value
        compounds = self._parser(crystalFileName)

        dMin = self.getProperty("LatticeSpacingMin").value
        dMax = self.getProperty("LatticeSpacingMax").value

        workspaces = []

        for compound in compounds:
            workspaces.append(self._createPeaksFromCell(compound, dMin, dMax))

        self.setProperty("OutputWorkspace", GroupWorkspaces(workspaces))

    def _createPeaksFromCell(self, compound, dMin, dMax):
        PoldiCreatePeaksFromCell(SpaceGroup=compound.getSpaceGroup(),
                                 Atoms=compound.getAtomString(),
                                 LatticeSpacingMin=dMin,
                                 LatticeSpacingMax=dMax,
                                 OutputWorkspace=compound.getName(),
                                 **compound.getCellParameters())

        return compound.getName()

AlgorithmFactory.subscribe(PoldiLoadCrystalData)