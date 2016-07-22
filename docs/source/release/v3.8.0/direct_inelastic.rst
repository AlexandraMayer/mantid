========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` has an option to skip safety checks. This improves the speed when acting on workspace groups.

- A qtiGenie method *export_masks* was brought to Mantid as :ref:`ExportSpectraMask <algm-ExportSpectraMask>` Python algorithm and got documentation, unit tests and Python GUI.
  The algorithm allows to export list of masked workspace spectra and save these spectra as ISIS *.msk* file. 
  The export mask procedure is often used by instrument scientists in ISIS, and they had to initialize qtiGenie to do this operation before these changes. 

- :ref:`MaskDetectors <algm-MaskDetectors>` modified to work on a grouped workspace, so if a spectra of the mask workspace is masked, the 
 spectra of the target workspace, with the detector group containing the masked detector become masked. This allows to use *.xml* masks, prodiced by 
 :ref:`SaveMask <algm-SaveMask>` algorithm on the grouped workspaces, obtained from ISIS instruments.  
 
- :ref:`LoadMask <algm-LoadMask>` modified to accept a workspace-source of spectra-detector map when spectra mask is provided.
  This allow users to use old legacy *.msk* files as source of mask workspaces usable with modified :ref:`MaskDetectors <algm-MaskDetectors>` algorithm
  and to use old spectra masks on a workspaces with different grouping and spectra-detector maps.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Crystal Field
-------------

- A fitting function was added (:ref:`CrystalFieldMultiSpectrum <func-CrystalFieldMultiSpectrum>`) that fits crystal field parameters to multiple spectra simultaneously.


