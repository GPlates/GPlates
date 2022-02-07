.. _pygplates_plate_circuits_to_anchored_plate:

Plate circuits to anchored plate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds and prints the plate circuit paths from all plates in the
:ref:`plate rotation hierarchy<pygplates_foundations_plate_reconstruction_hierarchy>`
to the anchored plate (for a particular reconstruction time).

The output of this example is similar to the output of the ``Total Reconstruction Poles`` dialog
(under the ``Plate Circuits to Anchored Plate`` tab) in `GPlates <http://www.gplates.org>`_.

.. seealso:: :ref:`pygplates_plate_rotation_hierarchy`

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates
    
    
    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # The reconstruction time (Ma) of the plate hierarchy we're interested in.
    reconstruction_time = 60
    
    # Get the reconstruction tree.
    reconstruction_tree = rotation_model.get_reconstruction_tree(reconstruction_time)
    
    # Get all the edges of the reconstruction tree.
    all_edges = reconstruction_tree.get_edges()
    
    # Iterate over all the edges.
    for edge in all_edges:
        
        print 'Plate ID: %d:' % edge.get_moving_plate_id()
        
        # Traverse the plate circuit from the current plate to the anchored plate.
        edge_in_circuit = edge
        while edge_in_circuit:
            
            relative_total_rotation = edge_in_circuit.get_relative_total_rotation()
            relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
                relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
            
            equivalent_total_rotation = edge_in_circuit.get_equivalent_total_rotation()
            equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
                equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
            
            print '  Plate ID: %d, Fixed Plate ID: %d:' % (
                edge_in_circuit.get_moving_plate_id(), edge_in_circuit.get_fixed_plate_id())
            
            print '    Rotation rel. fixed (parent) plate: lat: %f, lon: %f:, angle:%f' % (
                relative_pole_latitude, relative_pole_longitude, relative_angle_degrees)
            
            print '    Equivalent rotation rel. anchored plate: lat: %f, lon: %f:, angle:%f' % (
                equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees)
            
            # Blank line.
            print
            
            # Follow the plate circuit one step closer to the anchored plate.
            edge_in_circuit = edge_in_circuit.get_parent_edge()


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

| The :ref:`plate rotation hierarchy<pygplates_foundations_plate_reconstruction_hierarchy>`
  is encapsulated in a :class:`reconstruction tree<pygplates.ReconstructionTree>` which we obtain
  from the :class:`rotation model<pygplates.RotationModel>` using
  :meth:`pygplates.RotationModel.get_reconstruction_tree` and the desired reconstruction time.
| The hierarchy can change from one reconstruction time to the next depending on how the rotations
  are arranged in the rotation file(s).

::

    reconstruction_tree = rotation_model.get_reconstruction_tree(reconstruction_time)

| An edge in a :ref:`plate rotation hierarchy<pygplates_foundations_plate_reconstruction_hierarchy>`
  represents the rotation of a moving plate relative to a fixed plate. These edges are arranged in
  a tree-like structure (hierarchy) rooted at the anchor plate (usually plate ID zero).
| We get all the edges in the reconstruction tree because all the plates in the reconstruction tree
  (except the anchor plate) are in fact the moving plates of all the edges.
| All the edges are obtained using :meth:`pygplates.ReconstructionTree.get_edges`.

::

    all_edges = reconstruction_tree.get_edges()
    for edge in all_edges:
        ...

Print the (moving) plate ID corresponding to the current edge before we print its plate circuit to the anchored plate.
::

    print 'Plate ID: %d:' % edge.get_moving_plate_id()

Iterate over the edges in the plate circuit path between the current ``edge`` and the anchored plate.
::

    edge_in_circuit = edge
    while edge_in_circuit:
        ...

| Get the :ref:`relative<pygplates_foundations_relative_total_rotation>` and
  :ref:`equivalent<pygplates_foundations_equivalent_total_rotation>` total rotations of the current edge
  in the plate circuit path using :meth:`pygplates.ReconstructionTreeEdge.get_relative_total_rotation`
  and :meth:`pygplates.ReconstructionTreeEdge.get_equivalent_total_rotation`.
| The relative rotation is the total rotation of the edge's moving plate relative to its fixed plate.
| The equivalent total rotation is the total rotation of the edge's moving plate relative to anchored plate.
| A *total* rotation means a rotation at the reconstruction time relative to *present day* (0Ma).
| The pole and angle of each rotation is obtained using
  :meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees`.

::

    relative_total_rotation = edge_in_circuit.get_relative_total_rotation()
    relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
        relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
    
    equivalent_total_rotation = edge_in_circuit.get_equivalent_total_rotation()
    equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
        equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())

Print the relative and equivalent total rotations of the moving plate of the current edge in the plate circuit path.
::

    print '  Plate ID: %d, Fixed Plate ID: %d:' % (
        edge_in_circuit.get_moving_plate_id(), edge_in_circuit.get_fixed_plate_id())
    
    print '    Rotation rel. fixed (parent) plate: lat: %f, lon: %f:, angle:%f' % (
        relative_pole_latitude, relative_pole_longitude, relative_angle_degrees)
    
    print '    Equivalent rotation rel. anchored plate: lat: %f, lon: %f:, angle:%f' % (
        equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees)
    
    print

| Follow the plate circuit one step closer to the anchored plate using
  :meth:`pygplates.ReconstructionTreeEdge.get_parent_edge`.
| The ``while`` loop stops when an edge has no parent edge. This means we've reached an anchored plate edge
  (an edge whose fixed plate is the anchored plate).

::

    edge_in_circuit = edge_in_circuit.get_parent_edge()

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

...where ``lat: 90.000000, lon: 0.000000:, angle:0.000000`` is the default representation that
:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns for an
:meth:`identity rotation<pygplates.FiniteRotation.represents_identity_rotation>` (zero rotation angle).
