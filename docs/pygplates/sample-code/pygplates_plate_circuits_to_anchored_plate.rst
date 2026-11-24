.. _pygplates_plate_circuits_to_anchored_plate:

Plate circuits to anchored plate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example prints the plate circuit path from each plate in the
:ref:`plate rotation hierarchy<pygplates_primer_plate_reconstruction_hierarchy>`
to the anchor plate, at a reconstruction time.

The output is similar to the ``Plate Circuits to Anchored Plate`` tab of the
``Total Reconstruction Poles`` dialog in `GPlates <http://www.gplates.org>`_.

.. seealso:: :ref:`pygplates_plate_rotation_hierarchy`

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: load-rotations

The :ref:`plate rotation hierarchy<pygplates_primer_plate_reconstruction_hierarchy>` at a
reconstruction time is held in a :class:`reconstruction tree<pygplates.ReconstructionTree>`.
:meth:`pygplates.RotationModel.get_reconstruction_tree` returns the tree at the reconstruction
time, here 60Ma.

The hierarchy can differ from one reconstruction time to the next, depending on how the rotations
are arranged in the rotation file.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: reconstruction-tree

An :class:`edge<pygplates.ReconstructionTreeEdge>` of the reconstruction tree is the rotation of a
moving plate relative to a fixed plate. The edges form a tree whose root is the anchor plate
(plate ID zero, unless another is asked for).

Every plate in the tree, except the anchor plate, is the moving plate of exactly one edge. So
iterating over all the edges visits every plate. :meth:`pygplates.ReconstructionTree.get_edges`
returns all the edges.

::

    all_edges = reconstruction_tree.get_edges()
    for edge in all_edges:
        ...

For each edge, the script first prints its moving plate ID. This is the plate whose plate circuit
is printed next.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: print-plate-id

The plate circuit path is the sequence of edges from that plate up to the anchor plate. The script
starts at the plate's own edge and follows the path one edge at a time.

::

    edge_in_circuit = edge
    while edge_in_circuit:
        ...

For each edge in the path, the script gets the :ref:`relative<pygplates_primer_relative_total_rotation>`
and :ref:`equivalent<pygplates_primer_equivalent_total_rotation>` total rotations of the edge, with
:meth:`pygplates.ReconstructionTreeEdge.get_relative_total_rotation` and
:meth:`pygplates.ReconstructionTreeEdge.get_equivalent_total_rotation`:

- The relative total rotation is the rotation of the moving plate of the edge relative to its fixed plate.
- The equivalent total rotation is the rotation of the moving plate of the edge relative to the anchor plate.

A *total* rotation is a rotation from present day (0Ma) to the reconstruction time.

:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns the pole and
angle of each rotation.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: relative-and-equivalent-rotations

The script then prints the moving and fixed plate IDs of that edge, and its two rotations.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: print-rotations

:meth:`pygplates.ReconstructionTreeEdge.get_parent_edge` returns the next edge in the path, which
is one step closer to the anchor plate.

An anchor plate edge (an edge whose fixed plate is the anchor plate) has no parent edge, so
``get_parent_edge()`` returns ``None`` and the ``while`` loop stops.

.. sample-code:: pygplates_plate_circuits_to_anchored_plate.py
   :fragment: parent-edge

Output
""""""

::

  Plate ID: 1:
    Plate ID: 1, Fixed Plate ID: 0:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
  
  Plate ID: 2:
    Plate ID: 2, Fixed Plate ID: 901:
      Rotation rel. fixed (parent) plate: lat: 57.429209, lon: -72.529235:, angle:-38.063290
      Equivalent rotation rel. anchored plate: lat: -46.815419, lon: -78.838045:, angle:-9.303734
  
    Plate ID: 901, Fixed Plate ID: 804:
      Rotation rel. fixed (parent) plate: lat: 71.312099, lon: -54.488341:, angle:44.370707
      Equivalent rotation rel. anchored plate: lat: 68.669125, lon: -58.413957:, angle:41.330547
  
    Plate ID: 804, Fixed Plate ID: 802:
      Rotation rel. fixed (parent) plate: lat: -18.150000, lon: -17.850000:, angle:2.129032
      Equivalent rotation rel. anchored plate: lat: -74.349502, lon: -68.326678:, angle:3.731361
  
    Plate ID: 802, Fixed Plate ID: 701:
      Rotation rel. fixed (parent) plate: lat: 10.617614, lon: -47.371326:, angle:10.778033
      Equivalent rotation rel. anchored plate: lat: 62.066424, lon: 9.485588:, angle:-3.331182
  
    Plate ID: 701, Fixed Plate ID: 1:
      Rotation rel. fixed (parent) plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
      Equivalent rotation rel. anchored plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
  
    Plate ID: 1, Fixed Plate ID: 0:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
  
  Plate ID: 3:
    Plate ID: 3, Fixed Plate ID: 0:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
  
  Plate ID: 101:
    Plate ID: 101, Fixed Plate ID: 714:
      Rotation rel. fixed (parent) plate: lat: 81.307187, lon: 3.720675:, angle:19.151434
      Equivalent rotation rel. anchored plate: lat: -53.663182, lon: -64.839189:, angle:-16.897027
  
    Plate ID: 714, Fixed Plate ID: 701:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
  
    Plate ID: 701, Fixed Plate ID: 1:
      Rotation rel. fixed (parent) plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
      Equivalent rotation rel. anchored plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
  
    Plate ID: 1, Fixed Plate ID: 0:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
  
  ...

Here ``lat: 90.000000, lon: 0.000000:, angle:0.000000`` is what
:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns, by default, for
an :meth:`identity rotation<pygplates.FiniteRotation.represents_identity_rotation>`: the north
pole and an angle of zero. An identity rotation has no pole of its own, since it does not rotate.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_reconstruction_hierarchy`, :ref:`pygplates_primer_plate_circuit_paths`
- Reference: :meth:`pygplates.RotationModel.get_reconstruction_tree`, :meth:`pygplates.ReconstructionTree.get_edges`,
  :meth:`pygplates.ReconstructionTreeEdge.get_parent_edge`
- Sample code: :ref:`pygplates_plate_rotation_hierarchy`
