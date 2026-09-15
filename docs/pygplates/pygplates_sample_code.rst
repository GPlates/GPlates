.. _pygplates_sample_code:

Sample code
===========

This document contains sample code that shows pyGPlates solving common plate tectonic problems.

.. note:: The :ref:`pygplates_getting_started` section has a
   :ref:`Getting Started tutorial<pygplates_getting_started_tutorial>` as well as an
   :ref:`installation guide<pygplates_getting_started_installation>` to help get started.

Each sample is a complete script that runs in a directory containing the data files it names
(such as ``rotations.rot`` or ``topologies.gpml``). The GPlates
`sample data <https://www.gplates.org/download/>`_ provides files of every kind the samples use.

.. contents::
   :local:
   :depth: 2

Load and save
-------------

This section covers how to get geometry data into a state where it can be reconstructed by pyGPlates,
and how to load and save collections of features from/to files.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_import_geometries_and_assign_plate_ids
   sample-code/pygplates_load_and_save_feature_collections

Create and query features
-------------------------

This section covers how to create features from scratch and how to query their properties.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_create_common_feature_types
   sample-code/pygplates_query_common_feature_types
   sample-code/pygplates_create_topological_features

Reconstruct
-----------

This section covers how to reconstruct regular features, motion path features and flowline features
to past geological times.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_reconstruct_regular_features
   sample-code/pygplates_reconstruct_motion_path_features
   sample-code/pygplates_reconstruct_flowline_features

Rotations
---------

This section covers querying and modifying the rotations that are used to reconstruct features to
past geological times.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_plate_rotation_hierarchy
   sample-code/pygplates_plate_circuits_to_anchored_plate
   sample-code/pygplates_modify_reconstruction_pole

Topologies and plate boundaries
-------------------------------

This section covers how to reconstruct (or resolve) dynamic geometries (such as topological plate boundaries)
at past geological times, how to query their topologies, and how to track quantities through deforming regions.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies
   sample-code/pygplates_find_total_ridge_and_subduction_zone_lengths
   sample-code/pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges
   sample-code/pygplates_detect_topology_gaps_and_overlaps
   sample-code/pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence

Velocities, strain and net rotation
-----------------------------------

This section covers how to calculate velocities of features as they move through geological time,
velocities of plates at non-moving point locations, strain and strain rate in deforming regions,
and the net rotation of the lithosphere.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_calculate_velocities_by_plate_id
   sample-code/pygplates_calculate_velocities_in_dynamic_plates
   sample-code/pygplates_reconstruct_strain_and_strain_rate
   sample-code/pygplates_sample_intra-plate_strain_rates_at_subduction_zones
   sample-code/pygplates_calculate_net_rotation

Spatial queries
---------------

This section covers queries involving distances between geometries.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_find_nearest_feature_to_a_point
   sample-code/pygplates_find_features_overlapping_a_polygon
   sample-code/pygplates_find_overriding_plate_of_closest_subducting_line

.. seealso:: :ref:`pygplates_calculate_distance_a_feature_is_reconstructed`

Isochrons
---------

This section covers some use cases specific to the isochron feature type.

.. toctree::
   :maxdepth: 3

   sample-code/pygplates_create_conjugate_isochrons_from_ridge
   sample-code/pygplates_split_isochron_into_ridges_and_transforms
