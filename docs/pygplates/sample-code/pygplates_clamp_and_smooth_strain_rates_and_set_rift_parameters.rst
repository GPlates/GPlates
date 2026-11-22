.. _pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters:

Clamp and smooth strain rates, and set rift parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example changes how the strain rates of deforming networks are calculated, using
:class:`pygplates.ResolveTopologyParameters` and the rift parameters of a network feature.

The strain rate at a point in a deforming network comes from the triangulation of the network, in
which each triangle has one strain rate (see :ref:`pygplates_query_network_triangulation_and_rigid_blocks`).
Three things about it can be changed:

- *Clamping* limits the strain rate of every triangle to a maximum. Depending on how a network was
  built, some of its triangles can have excessive strain rates.
- *Smoothing* is how the strain rate at a point is interpolated from the triangles around it.
- *Rift parameters* say that a network is one side of a rift between two plates. The strain rate then
  increases exponentially across the network, from the un-stretched side of the rift to the rift
  axis, where otherwise it would be much the same all the way across.

All three change whatever is calculated from strain rates as well, such as the accumulated strain,
crustal thickness and tectonic subsidence of points reconstructed through the networks
(see :ref:`pygplates_reconstruct_strain_and_strain_rate` and
:ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`).

The first example compares strain rates with and without clamping, and with each kind of smoothing.
The second gives the networks of a rift their rift parameters, and compares the strain rates across
the rift.

.. contents::
   :local:
   :depth: 2

Clamp and smooth strain rates
+++++++++++++++++++++++++++++

Data files
""""""""""

``topologies.gpml``
    Topological features that include deforming networks. The parameters in this example only
    affect deforming networks.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features that the deforming
    networks are built from.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py

Details
"""""""

The topological features are loaded into a :class:`pygplates.FeatureCollection`, and the rotations
into a :class:`pygplates.RotationModel`.

A topological model can be created from file names instead. But this example creates several
topological models, and this way the files are only read once.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: load-features

The strain rates are compared at four points at 10Ma. Each point is inside a deforming network at that time.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: time-and-points

A :class:`pygplates.ResolveTopologyParameters` holds the parameters that topologies are resolved with.

Created without arguments, it has the default value of every parameter: strain rates are not
clamped, and they are smoothed using natural neighbour interpolation.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: default-parameters

Each of the other sets of parameters changes one thing, and leaves the rest at their defaults.

Clamping is turned on by *enable_strain_rate_clamping*, and *max_clamped_strain_rate* is the maximum
(in units of 1/second).

It is the :meth:`total strain rate<pygplates.StrainRate.get_total_strain_rate>` of a triangle that
is limited to the maximum. That is the magnitude of the strain rate, including both its normal and
shear components. When it is reduced, every component of the strain rate is reduced in proportion,
so the dilatation rate goes down by the same factor.

The default maximum is ``5e-15`` per second. This example uses a smaller one.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: clamped-parameters

The *strain_rate_smoothing* argument takes one of the values of :class:`pygplates.StrainRateSmoothing`.

With ``pygplates.StrainRateSmoothing.none`` the strain rate at a point is that of the triangle that
contains the point. So it is the same everywhere in a triangle, and it jumps at the edge between
two triangles.

With ``pygplates.StrainRateSmoothing.barycentric`` it is interpolated linearly from the strain rates
at the three vertices of that triangle.

With ``pygplates.StrainRateSmoothing.natural_neighbour``, which is the default, it is interpolated
from the strain rates at the vertices near the point, using natural neighbour interpolation.

(The strain rate at a vertex is an average of the strain rates of the deforming triangles around it.)

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: smoothing-parameters

The function ``print_strain_rates()`` prints the strain rates that one topological model gives.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: define-function

The topologies are resolved at 10Ma, and :meth:`pygplates.TopologicalSnapshot.get_point_strain_rates`
returns the strain rate at each point. It is these that smoothing changes.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: point-strain-rates

The function also finds the largest total strain rate of any triangle of any deforming network,
to show what clamping does. With clamping it is the maximum that was asked for (unless no
triangle reached it).

Clamping is applied to the triangles. The strain rate at a point is then interpolated from the
clamped triangles, so it is no more than the maximum either.

The triangles of a network come from its :class:`triangulation<pygplates.NetworkTriangulation>`.
A triangle that is not in the deforming region of its network has a strain rate of zero, so it
makes no difference here.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: largest-triangle-strain-rate

The largest strain rate of a triangle is printed, followed by the total strain rate at each point.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: print-values

Each set of parameters is visited with a name to print.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: iterate-parameters

A :class:`topological model<pygplates.TopologicalModel>` resolves its topologies using the
parameters it is given as its *default_resolve_topology_parameters* argument. Without that
argument it uses the default parameters.

The parameters cannot be changed afterwards, so a topological model is created for each set of
parameters.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: create-topological-model

Parameters can also be given to part of a topological model.

A topological model can be created from a list of feature collections, and each of them can be
paired with its own parameters, in a 2-tuple. Its deforming networks are then resolved using
those parameters, and the networks in the others using the *default_resolve_topology_parameters*.

The same goes for a file name, a feature or a sequence of features in place of a feature collection.

Here there is only the one feature collection. The model gives the same strain rates as the one
created with the same parameters above.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters.py
   :fragment: parameters-for-one-file

Give a rift an exponential stretching profile
+++++++++++++++++++++++++++++++++++++++++++++

This example takes the deforming networks of a rift, makes copies of them that have rift parameters,
and compares the strain rates across the rift with and without the parameters.

Data files
""""""""""

``topologies.gpml``
    The deforming network features of a rift, and the features that their boundaries are built from.
    A rift is usually modelled with two networks, one on each side of the rift axis. Each lies between
    an un-stretched margin, which moves with one of the two plates, and the rift axis. In this example
    every network in the file is taken to be part of the one rift, between South America (plate
    201) and Africa (plate 701).

``rotations.rot``
    A rotation file. It must contain rotations for the two plates of the rift, and for the plate IDs
    of the features that the deforming networks are built from.

Sample code
"""""""""""

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py

Details
"""""""

A rift is between two plates, and its rift parameters start with their two plate IDs. They have to
be the plate IDs that the un-stretched margins of the rift move with. A side of a network counts as
un-stretched where it moves exactly as one of the two plates does, and the motion of one plate
relative to the other gives the direction of stretching.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: rift-plates

The features are loaded into a :class:`pygplates.FeatureCollection`, rather than given to a
topological model as a file name, so that the network features can be replaced. The rotations are
loaded into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: load-features

A second list of the features is made, in which each network feature is replaced by a copy that
has rift parameters.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: iterate-features

Every other feature goes into the second list as it is, so both lists hold the same features for
the boundaries of the networks.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: other-features

:meth:`pygplates.Feature.create_topological_network_feature` creates a network feature. Its
*rift_parameters* argument is a tuple that begins with the two plate IDs of the rift. It doesn't
matter which is left or right: you can swap them.

They are stored in the feature as the properties ``gpml:riftLeftPlate`` and ``gpml:riftRightPlate``.
A network that has those two properties is resolved as a rift, and a network without them is not.

The copy is given the topological geometry of the network it replaces
(:meth:`pygplates.Feature.get_topological_geometry` returns it, as a
:class:`pygplates.GpmlTopologicalNetwork`), and its name, valid time and reconstruction plate ID.
Any other properties of the original would need to be copied too.

The copy still refers to the features of its boundary, since a topological geometry refers to
those features by their feature IDs.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: create-rift-network

.. note:: The tuple can have up to three more values after the two plate IDs: the exponential
   stretching constant, the strain rate resolution and the edge length threshold (in degrees) of
   that network. Any of them can be ``None``. For example ``rift_parameters=(201, 701, 2.0)`` sets
   the first. A value that is not given comes from the :class:`pygplates.ResolveTopologyParameters`
   that the network is resolved with, as below.

Three topological models are created: one from the features as they were loaded, and two from
the list with the rift networks.

The third is also given a :class:`pygplates.ResolveTopologyParameters` with a
*rift_exponential_stretching_constant* of 2, where the default is 1. The larger the constant, the
more the stretching is concentrated towards the rift axis.

The rift parameters of a :class:`pygplates.ResolveTopologyParameters` apply to every rift network
that does not have its own value in its feature. They do nothing to a network that has no rift
plate IDs.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: topological-models

The strain rates are compared along a profile across one side of the rift at 122Ma.

The points run from just inside the un-stretched margin of South America to just short of the rift
axis. They are positions at 122Ma, and belong to the rift in this example's data.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: profile-points

For each topological model, the triangles of the resolved networks are counted.

The triangulation of a rift network is refined: more vertices are added to it between the
un-stretched side and the rift axis, and they are given velocities that make the strain rate vary
exponentially along the direction of stretching. So the rift networks have more triangles.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: count-triangles

:meth:`pygplates.TopologicalSnapshot.get_point_strain_rates` returns the strain rate at each point
of the profile.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: profile-strain-rates

Finally the :meth:`dilatation rate<pygplates.StrainRate.get_dilatation_rate>` at each point of the
profile is printed.

Without rift parameters it is much the same along the whole profile. With them it increases from
the un-stretched margin to the rift axis, and with the larger stretching constant it increases more
steeply.

.. sample-code:: pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters_rift.py
   :fragment: print-profile

.. note:: How closely the strain rates follow an exponential curve depends on how far the
   triangulation is refined. That is controlled by the other two rift parameters,
   :attr:`pygplates.ResolveTopologyParameters.rift_strain_rate_resolution` and
   :attr:`pygplates.ResolveTopologyParameters.rift_edge_length_threshold_degrees`. Smaller values
   give more triangles and take longer.
   See :ref:`pygplates_primer_exponential_rift_stretching_profile` for the curve.

See also
++++++++

- Primer: :ref:`pygplates_primer_strain_rates_in_triangulation`, :ref:`pygplates_primer_strain_rate_clamping`,
  :ref:`pygplates_primer_strain_rate_smoothing`, :ref:`pygplates_primer_exponential_rift_stretching_profile`,
  :ref:`pygplates_primer_topological_model`
- Reference: :class:`pygplates.ResolveTopologyParameters`, :class:`pygplates.StrainRateSmoothing`,
  :class:`pygplates.TopologicalModel`, :meth:`pygplates.Feature.create_topological_network_feature`,
  :meth:`pygplates.TopologicalSnapshot.get_point_strain_rates`, :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_find_plates_and_strain_rates_at_static_points`,
  :ref:`pygplates_query_network_triangulation_and_rigid_blocks`,
  :ref:`pygplates_reconstruct_strain_and_strain_rate`,
  :ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`,
  :ref:`pygplates_create_topological_features`
