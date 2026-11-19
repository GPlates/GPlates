.. _pygplates_primer:

Primer
======

This document covers the main areas of pyGPlates functionality, and some plate tectonic foundations.

Each page of the Primer covers one area, and can be read on its own:

* :ref:`pygplates_primer_rotations` explains how plates move relative to each other through a
  hierarchy of plate IDs, and derives the equivalent and relative, total and stage rotations
  that a :class:`pygplates.RotationModel` answers queries with.
* :ref:`pygplates_primer_topologies` covers topological plate boundaries and networks: resolving
  them through time, calculating plate boundary statistics along them, and reconstructing
  geometries through them (including velocities, strains and scalar values along the way).
* :ref:`pygplates_primer_deformation` explains how a topological network models deformation, from
  its boundary, rigid blocks and triangulation to the strain rates within it and the exponential
  stretching profile of rifts.

.. toctree::
   :maxdepth: 2

   primer/pygplates_primer_rotations
   primer/pygplates_primer_topologies
   primer/pygplates_primer_deformation

..
   The sections above were once all on this page, so links into it (such as
   'pygplates_primer.html#pygplates-primer-topological-model') are published in papers, notebooks and
   other websites. Redirect such a link to the page the section now lives on. The map from anchor to
   page is generated after each build from the sections' labels (see 'write_primer_anchors()' in
   'conf.py.in'), so it follows a section that moves to another page.

.. raw:: html

   <script src="_static/primer_anchors.js"></script>
   <script>
     (function () {
       var anchor = window.location.hash.slice(1);
       if (anchor && window.PRIMER_ANCHORS && PRIMER_ANCHORS.hasOwnProperty(anchor)) {
         window.location.replace(PRIMER_ANCHORS[anchor] + window.location.hash);
       }
     })();
   </script>
