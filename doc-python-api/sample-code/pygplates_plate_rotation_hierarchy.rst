.. _pygplates_plate_rotation_hierarchy:

Obtain hierarchy of plate rotations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example traverses the :ref:`plate rotation hierarchy<pygplates_foundations_plate_reconstruction_hierarchy>`
(for a particular reconstruction time) and prints the equivalent and relative total rotations at each plate.

The output of this example is similar to the output of the ``Total Reconstruction Poles`` dialog
(under the ``Reconstruction Tree`` tab) in `GPlates <http://www.gplates.org>`_.

Sample code
"""""""""""

::

    import pygplates
    
    
    # A function to traverse the sub-tree rooted at a particular plate (the moving plate of 'edge').
    def traverse_sub_tree(edge):
        
        relative_total_rotation = edge.get_relative_total_rotation()
        relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
            relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
        
        equivalent_total_rotation = edge.get_equivalent_total_rotation()
        equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
            equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
        
        print 'Plate ID: %d, Fixed Plate ID: %d:' % (edge.get_moving_plate_id(), edge.get_fixed_plate_id())
        
        print '  Rotation rel. fixed (parent) plate: lat: %f, lon: %f:, angle:%f' % (
            relative_pole_latitude, relative_pole_longitude, relative_angle_degrees)
        
        print '  Equivalent rotation rel. anchored plate: lat: %f, lon: %f:, angle:%f' % (
            equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees)
        
        # Recurse into the children sub-trees.
        for child_edge in edge.get_child_edges():
            traverse_sub_tree(child_edge)
    
    
    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # The reconstruction time (Ma) of the plate hierarchy we're interested in.
    reconstruction_time = 60
    
    # Get the reconstruction tree.
    reconstruction_tree = rotation_model.get_reconstruction_tree(reconstruction_time)
    
    # Get the edges of the reconstruction tree emanating from its root (anchor) plate.
    anchor_plate_edges = reconstruction_tree.get_anchor_plate_edges()
    
    # Iterate over the anchor plate edges and traverse the sub-tree of each edge.
    for anchor_plate_edge in anchor_plate_edges:
        traverse_sub_tree(anchor_plate_edge)


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
| The anchor plate edges represent those edges emanating from the anchor plate and are obtained
  using :meth:`pygplates.ReconstructionTree.get_anchor_plate_edges`.

::

    anchor_plate_edges = reconstruction_tree.get_anchor_plate_edges()

| The anchor plate edges have different moving plate IDs but all have the same fixed plate ID (which is the anchor plate).
| In this way the moving plate of each anchor plate edge is a sub-tree of the entire reconstruction tree.
| Here we traverse the sub-trees corresponding to those anchor plate edges.

::

    for anchor_plate_edge in anchor_plate_edges:
        traverse_sub_tree(anchor_plate_edge)

| A function is defined that traverses the sub-tree rooted at the moving plate of an edge in the reconstruction tree.
| One reason for implementing this as a function is we need to call it recursively (a recursive function
  calls itself) and this is more difficult to achieve without using a function.

::

    def traverse_sub_tree(edge):
        ...

| Get the :ref:`relative<pygplates_foundations_relative_total_rotation>` and
  :ref:`equivalent<pygplates_foundations_equivalent_total_rotation>` total rotations of an edge
  in the reconstruction tree using :meth:`pygplates.ReconstructionTreeEdge.get_relative_total_rotation`
  and :meth:`pygplates.ReconstructionTreeEdge.get_equivalent_total_rotation`.
| The relative rotation is the total rotation of the edge's moving plate relative to its fixed plate.
| The equivalent total rotation is the total rotation of the edge's moving plate relative to anchored plate.
| A *total* rotation means a rotation at the reconstruction time relative to *present day* (0Ma).
| The pole and angle of each rotation is obtained using
  :meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees`.

::

    relative_total_rotation = edge.get_relative_total_rotation()
    relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
        relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
    
    equivalent_total_rotation = edge.get_equivalent_total_rotation()
    equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
        equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())

Print the relative and equivalent total rotations of the moving plate of the reconstruction tree edge.
::

    print 'Plate ID: %d, Fixed Plate ID: %d:' % (edge.get_moving_plate_id(), edge.get_fixed_plate_id())
    
    print '  Rotation rel. fixed (parent) plate: lat: %f, lon: %f:, angle:%f' % (
        relative_pole_latitude, relative_pole_longitude, relative_angle_degrees)
    
    print '  Equivalent rotation rel. anchored plate: lat: %f, lon: %f:, angle:%f' % (
        equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees)

| Just as the anchored plate has one or more anchored plate edges emanating from it,
  the moving plate of a reconstruction tree edge has one or more child edges emanating from it.
  These are obtained using :meth:`pygplates.ReconstructionTreeEdge.get_child_edges`.
| Note that by calling the ``traverse_sub_tree`` function we are calling the same function we are
  already in. This recursive descent enables us to visit all edges and plates in the sub-tree.
| The recursion stops when an edge has no child edges. This means that no other plate moves
  relative to the (moving) plate of that edge.

::

    for child_edge in edge.get_child_edges():
        traverse_sub_tree(child_edge)

And finally the output should look something like:
::

    Plate ID: 1, Fixed Plate ID: 0:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
    Plate ID: 701, Fixed Plate ID: 1:
      Rotation rel. fixed (parent) plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
      Equivalent rotation rel. anchored plate: lat: 23.730000, lon: -42.140000:, angle:-12.530000
    Plate ID: 201, Fixed Plate ID: 701:
      Rotation rel. fixed (parent) plate: lat: 62.238025, lon: -32.673047:, angle:23.349295
      Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
    Plate ID: 202, Fixed Plate ID: 201:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
    Plate ID: 290, Fixed Plate ID: 202:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
    Plate ID: 203, Fixed Plate ID: 201:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 77.493750, lon: 57.067142:, angle:15.711412
    Plate ID: 225, Fixed Plate ID: 201:
      Rotation rel. fixed (parent) plate: lat: -1.520000, lon: -62.240000:, angle:9.500000
      Equivalent rotation rel. anchored plate: lat: 59.149009, lon: -33.687205:, angle:17.238928
    Plate ID: 226, Fixed Plate ID: 225:
      Rotation rel. fixed (parent) plate: lat: 90.000000, lon: 0.000000:, angle:0.000000
      Equivalent rotation rel. anchored plate: lat: 59.149009, lon: -33.687205:, angle:17.238928
    ...

...where ``lat: 90.000000, lon: 0.000000:, angle:0.000000`` is the default representation that
:meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees` returns for an
:meth:`identity rotation<pygplates.FiniteRotation.represents_identity_rotation>` (zero rotation angle).
