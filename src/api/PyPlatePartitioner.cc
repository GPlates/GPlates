/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "PyPlatePartitioner.h"

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/MathsUtils.h"

#include "property-values/GeoTimeInstant.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	PlatePartitionerWrapper
	plate_partitioner_create_from_features(
			FeatureCollectionSequenceFunctionArgument partitioning_features_argument,
			RotationModelFunctionArgument rotation_model_argument,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesApi::SortPartitioningPlates::Value> sort_partitioning_plates)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Get the partitioning feature collections and convert them to weak references.
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> partitioning_feature_collections;
		partitioning_features_argument.get_feature_collections(partitioning_feature_collections);

		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> partitioning_feature_collection_refs;
		BOOST_FOREACH(
				GPlatesModel::FeatureCollectionHandle::non_null_ptr_type partitioning_feature_collection,
				partitioning_feature_collections)
		{
			partitioning_feature_collection_refs.push_back(partitioning_feature_collection->reference());
		}

		// Determine grouping/sorting of partitioning plates.
		bool group_networks_then_boundaries_then_static_polygons = false;
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::SortPlates> sort_plates;
		if (sort_partitioning_plates)
		{
			switch (sort_partitioning_plates.get())
			{
			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE:
				group_networks_then_boundaries_then_static_polygons = true;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_ID:
				group_networks_then_boundaries_then_static_polygons = true;
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_AREA:
				group_networks_then_boundaries_then_static_polygons = true;
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PLATE_ID:
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PLATE_AREA:
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA;
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}

		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
		RotationModel::non_null_ptr_type rotation_model = rotation_model_argument.get_rotation_model();

		return PlatePartitionerWrapper(
				boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>(
						new GPlatesAppLogic::GeometryCookieCutter(
								reconstruction_time.value(),
								reconstruct_method_registry,
								partitioning_feature_collection_refs,
								rotation_model->get_reconstruction_tree_creator(),
								group_networks_then_boundaries_then_static_polygons,
								sort_plates)),
				rotation_model,
				partitioning_features_argument);
	}


	PlatePartitionerWrapper
	plate_partitioner_create_from_reconstruction_geometries(
			bp::object partitioning_plates, // Any python sequence (eg, list, tuple).
			bp::object rotation_model_object,
			boost::optional<GPlatesApi::SortPartitioningPlates::Value> sort_partitioning_plates)
	{
		const char *partitioning_plates_type_error_string = "Expected a sequence of ReconstructionGeometry";

		// Copy partitioning plate objects into a vector.
		// We'll store these Python objects to ensure the features they reference stay alive.
		std::vector<bp::object> partitioning_plate_objects_vector;
		PythonExtractUtils::extract_iterable(
				partitioning_plate_objects_vector,
				partitioning_plates,
				partitioning_plates_type_error_string);

		// Convert partitioning plate objects into reconstruction geometries.
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> partitioning_plates_vector;
		BOOST_FOREACH(const bp::object &partitioning_plate_object, partitioning_plate_objects_vector)
		{
			bp::extract<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> extract_partitioning_plate(
					partitioning_plate_object);
			if (!extract_partitioning_plate.check())
			{
				PyErr_SetString(PyExc_TypeError, partitioning_plates_type_error_string);
				boost::python::throw_error_already_set();
			}

			partitioning_plates_vector.push_back(extract_partitioning_plate());
		}

		// If there happen to be no partitioning plates then default the reconstruction time to zero.
		const double reconstruction_time = !partitioning_plates_vector.empty()
				? partitioning_plates_vector[0]->get_reconstruction_time()
				: 0;

		// Make sure all reconstruction times are the same.
		for (unsigned int n = 1; n < partitioning_plates_vector.size(); ++n)
		{
			GPlatesGlobal::Assert<DifferentTimesInPartitioningPlatesException>(
					GPlatesMaths::are_geo_times_approximately_equal(
							partitioning_plates_vector[n]->get_reconstruction_time(),
							reconstruction_time),
					GPLATES_ASSERTION_SOURCE);
		}

		// Determine grouping/sorting of partitioning plates.
		bool group_networks_then_boundaries_then_static_polygons = false;
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::SortPlates> sort_plates;
		if (sort_partitioning_plates)
		{
			switch (sort_partitioning_plates.get())
			{
			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE:
				group_networks_then_boundaries_then_static_polygons = true;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_ID:
				group_networks_then_boundaries_then_static_polygons = true;
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_AREA:
				group_networks_then_boundaries_then_static_polygons = true;
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PLATE_ID:
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID;
				break;

			case GPlatesApi::SortPartitioningPlates::BY_PLATE_AREA:
				sort_plates = GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA;
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}

		// Extract the rotation model (if specified).
		// It'll be used to reverse reconstruct (if reconstruction time is not present day).
		boost::optional<RotationModel::non_null_ptr_type> rotation_model;
		if (rotation_model_object != bp::object()/*Py_None*/)
		{
			bp::extract<RotationModelFunctionArgument> extract_rotation_model(rotation_model_object);
			rotation_model = extract_rotation_model().get_rotation_model();
		}
		else if (GPlatesPropertyValues::GeoTimeInstant(reconstruction_time) !=
				GPlatesPropertyValues::GeoTimeInstant(0))
		{
			PyErr_SetString(PyExc_ValueError,
					"A rotation model is required for non-zero reconstruction times (to reverse reconstruct).");
			bp::throw_error_already_set();
		}

		return PlatePartitionerWrapper(
				boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>(
						new GPlatesAppLogic::GeometryCookieCutter(
								reconstruction_time,
								partitioning_plates_vector,
								group_networks_then_boundaries_then_static_polygons,
								sort_plates)),
				rotation_model,
				boost::none,
				partitioning_plate_objects_vector);
	}

	bool
	plate_partitioner_partition_geometry(
			const GPlatesAppLogic::GeometryCookieCutter &plate_partitioner,
			bp::object geometry_object,
			bp::object partitioned_inside_geometries_object,
			bp::object partitioned_outside_geometries_object)
	{
		// Partitioned inside/outside lists may or may not get used.
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::partition_seq_type> partitioned_inside_geometries_storage;
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type> partitioned_outside_geometries_storage;
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::partition_seq_type &> partitioned_inside_geometries;
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type &> partitioned_outside_geometries;
		boost::optional<bp::list> partitioned_inside_geometries_list;
		boost::optional<bp::list> partitioned_outside_geometries_list;

		if (partitioned_inside_geometries_object != bp::object()/*Py_None*/)
		{
			bp::extract<bp::list> extract_partitioned_inside_geometries_list(partitioned_inside_geometries_object);
			if (!extract_partitioned_inside_geometries_list.check())
			{
				PyErr_SetString(PyExc_TypeError, "Expecting a list or None for 'partitioned_inside_geometries'");
				bp::throw_error_already_set();
			}
			partitioned_inside_geometries_storage = GPlatesAppLogic::GeometryCookieCutter::partition_seq_type();
			partitioned_inside_geometries = partitioned_inside_geometries_storage.get();
			partitioned_inside_geometries_list = extract_partitioned_inside_geometries_list();
		}

		if (partitioned_outside_geometries_object != bp::object()/*Py_None*/)
		{
			bp::extract<bp::list> extract_partitioned_outside_geometries_list(partitioned_outside_geometries_object);
			if (!extract_partitioned_outside_geometries_list.check())
			{
				PyErr_SetString(PyExc_TypeError, "Expecting a list or None for 'partitioned_outside_geometries'");
				bp::throw_error_already_set();
			}
			partitioned_outside_geometries_storage = GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type();
			partitioned_outside_geometries = partitioned_outside_geometries_storage.get();
			partitioned_outside_geometries_list = extract_partitioned_outside_geometries_list();
		}

		//
		// Partition the geometry.
		//
		// 'geometry_object' is either:
		//   1) a GeometryOnSphere, or
		//   2) a sequence of GeometryOnSphere's.
		//

		bool geometry_inside_any_partitions = false;

		bp::extract<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> extract_geometry(geometry_object);
		if (extract_geometry.check())
		{
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry = extract_geometry();

			geometry_inside_any_partitions = plate_partitioner.partition_geometry(
					geometry,
					partitioned_inside_geometries,
					partitioned_outside_geometries);
		}
		else
		{
			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometries;
			PythonExtractUtils::extract_iterable(
					geometries,
					geometry_object,
					"Expected a GeometryOnSphere, or a sequence of GeometryOnSphere");

			geometry_inside_any_partitions = plate_partitioner.partition_geometries(
					geometries,
					partitioned_inside_geometries,
					partitioned_outside_geometries);
		}

		//
		// Populate inside/output partitioned geometry lists if requested.
		//

		if (partitioned_inside_geometries)
		{
			// Append the inside geometry partitions to the caller's list.
			BOOST_FOREACH(
					const GPlatesAppLogic::GeometryCookieCutter::Partition &partitioned_inside_geometry,
					partitioned_inside_geometries.get())
			{
				// Each partition contains a list of geometries inside the partition's reconstruction geometry.
				bp::list partitioned_geometries_list;

				GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator partitioned_geoms_iter =
						partitioned_inside_geometry.partitioned_geometries.begin();
				GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator partitioned_geoms_end =
						partitioned_inside_geometry.partitioned_geometries.end();
				for ( ; partitioned_geoms_iter != partitioned_geoms_end; ++partitioned_geoms_iter)
				{
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type partitioned_geom = *partitioned_geoms_iter;

					partitioned_geometries_list.append(partitioned_geom);
				}

				// Append a 2-tuple containing the partitioning reconstruction geometry and partitioned list of geometries.
				partitioned_inside_geometries_list->append(
						bp::make_tuple(
								partitioned_inside_geometry.reconstruction_geometry,
								partitioned_geometries_list));
			}
		}

		if (partitioned_outside_geometries)
		{
			// Append the outside geometries to the caller's list.
			BOOST_FOREACH(
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &partitioned_outside_geometry,
					partitioned_outside_geometries.get())
			{
				partitioned_outside_geometries_list->append(partitioned_outside_geometry);
			}
		}

		return geometry_inside_any_partitions;
	}

	boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
	plate_partitioner_partition_point(
			const GPlatesAppLogic::GeometryCookieCutter &plate_partitioner,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		boost::optional<const GPlatesAppLogic::ReconstructionGeometry *> partitioning_reconstruction_geom =
				plate_partitioner.partition_point(point_on_sphere);
		if (!partitioning_reconstruction_geom)
		{
			return boost::none;
		}

		return GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type(
				partitioning_reconstruction_geom.get());
	}

	/**
	 * Returns the reconstruction time passed into constructor (used by pure Python API code).
	 */
	double
	plate_partitioner_get_reconstruction_time(
			const GPlatesAppLogic::GeometryCookieCutter &plate_partitioner)
	{
		return plate_partitioner.get_reconstruction_time();
	}

	/**
	 * Returns the rotation model (if any) passed into constructor (used by pure Python API code).
	 */
	boost::optional<RotationModel::non_null_ptr_type>
	plate_partitioner_get_rotation_model(
			const PlatePartitionerWrapper &plate_partitioner)
	{
		return plate_partitioner.get_rotation_model();
	}
}


void
export_plate_partitioner()
{
	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::PartitionProperty::Value>("PartitionProperty")
			.value("reconstruction_plate_id", GPlatesApi::PartitionProperty::RECONSTRUCTION_PLATE_ID)
			.value("valid_time_period", GPlatesApi::PartitionProperty::VALID_TIME_PERIOD)
			.value("valid_time_begin", GPlatesApi::PartitionProperty::VALID_TIME_BEGIN)
			.value("valid_time_end", GPlatesApi::PartitionProperty::VALID_TIME_END);

	// Enable boost::optional<GPlatesApi::PartitionProperty::Value> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::PartitionProperty::Value>();


	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::SortPartitioningPlates::Value>("SortPartitioningPlates")
			.value("by_partition_type", GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE)
			.value("by_partition_type_then_plate_id", GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_ID)
			.value("by_partition_type_then_plate_area", GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_AREA)
			.value("by_plate_id", GPlatesApi::SortPartitioningPlates::BY_PLATE_ID)
			.value("by_plate_area", GPlatesApi::SortPartitioningPlates::BY_PLATE_AREA);

	// Enable boost::optional<GPlatesApi::SortPartitioningPlates::Value> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::SortPartitioningPlates::Value>();


	//
	// PlatePartitioner - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::GeometryCookieCutter,
			GPlatesApi::PlatePartitionerWrapper,
			boost::noncopyable>(
					"PlatePartitioner",
					"Partition geometries using dynamic resolved topological boundaries and/or static reconstructed feature polygons.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::plate_partitioner_create_from_reconstruction_geometries,
						bp::default_call_policies(),
						(bp::arg("partitioning_plates"),
								bp::arg("rotation_model") = bp::object()/*Py_None*/,
								bp::arg("sort_partitioning_plates") =
										GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_ID)),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				// Specific overload signature...
				"__init__(...)\n"
				"A *PlatePartitioner* object can be constructed in more than one way. The following applies to both ways...\n"
				"\n"
				"  This table maps the values of the *sort_partitioning_plates* parameter to the "
				"sorting criteria used for the partitioning plates:\n"
				"\n"
				"  ======================================= ==============\n"
				"  SortPartitioningPlates Value            Description\n"
				"  ======================================= ==============\n"
				"  by_partition_type                       Group in order of resolved topological networks "
				"then resolved topological boundaries then reconstructed static polygons, but with no sorting within each group "
				"(ordering within each group is unchanged).\n"
				"  by_partition_type_then_plate_id         Same as *by_partition_type*, but also sort by "
				"plate ID (from highest to lowest) within each partition type group.\n"
				"  by_partition_type_then_plate_area       Same as *by_partition_type*, but also sort by "
				"plate area (from highest to lowest) within each partition type group.\n"
				"  by_plate_id                             Sort by plate ID (from highest to lowest), "
				"but no grouping by partition type.\n"
				"  by_plate_area                           Sort by plate area (from highest to lowest), "
				"but no grouping by partition type.\n"
				"  ======================================= ==============\n"
				"\n"
				"  .. note:: If you don't want to sort the partitioning plates (for example, if you have already sorted them) "
				"then you'll need to explicitly specify ``None`` for the *sort_partitioning_plates* parameter "
				"(eg, ``pygplates.PlatePartitioner(..., sort_partitioning_plates=None)``). "
				"This is because not specifying anything defaults to *SortPartitioningPlates.by_partition_type_then_plate_id* "
				"(since this always gives deterministic partitioning results).\n"
				"\n"
				"  If the partitioning plates overlap each other then their final ordering "
				"determines the partitioning results. Resolved topologies do not tend to overlap, "
				"but reconstructed static polygons do overlap and hence the sorting order becomes relevant.\n"
				"\n"
				"  Partitioning of points is more efficient if you sort by plate *area* because an arbitrary "
				"point is likely to be found sooner when testing against larger partitioning polygons first "
				"(and hence more remaining partitioning polygons can be skipped). Since resolved topologies don't tend "
				"to overlap you don't need to sort them by plate *ID* to get deterministic partitioning results. "
				"So we are free to sort by plate *area* (well, plate area is also deterministic but not as deterministic "
				"as sorting by plate *ID* since modifications to the plate geometries change their areas but not their plate IDs). "
				"Note that we also group by partition type in case the topological networks happen "
				"to overlay the topological plate boundaries(usually this isn't the case though):\n"
				"  ::\n"
				"\n"
				"    plate_partitioner = pygplates.PlatePartitioner(..., "
				"sort_partitioning_plates=pygplates.SortPartitioningPlates.by_partition_type_then_plate_area)\n"
				"\n"
				"  .. note:: Only those reconstructed/resolved geometries that contain a *polygon* boundary are actually used for partitioning. "
				"For :func:`resolved topologies<resolve_topologies>` this includes :class:`ResolvedTopologicalBoundary` and "
				":class:`ResolvedTopologicalNetwork`. For :func:`reconstructed geometries<reconstruct>`, a :class:`ReconstructedFeatureGeometry` "
				"is only included if its reconstructed geometry is a :class:`PolygonOnSphere`.\n"
				"\n"
				"**A PlatePartitioner object can be constructed in the following ways...**\n"
				"\n"

				// Specific overload signature...
				"__init__(partitioning_plates, [rotation_model], [sort_partitioning_plates=SortPartitioningPlates.by_partition_type_then_plate_id])\n"
				"  Create a partitioner from a sequence of reconstructed/resolved plates.\n"
				"\n"
				"  :param partitioning_plates: A sequence of reconstructed/resolved plates to partition with.\n"
				"  :type partitioning_plates: Any sequence of :class:`ReconstructionGeometry`\n"
				"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
				"filename or a sequence of rotation feature collections and/or rotation filenames\n"
				"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
				"or sequence of :class:`FeatureCollection` instances and/or strings\n"
				"  :param sort_partitioning_plates: optional sort order of partitioning plates "
				"(defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*)\n"
				"  :type sort_partitioning_plates: One of the values in the *SortPartitioningPlates* table above, or None\n"
				"  :raises: DifferentTimesInPartitioningPlatesError if all partitioning plates do not have the same "
				":meth:`reconstruction times<ReconstructionGeometry.get_reconstruction_time>`\n"
				"  :raises: ValueError if *rotation_model* is not specified and the reconstruction time "
				"(of the partitioning plates) is non-zero\n"
				"\n"
				"  The *partitioning_plates* sequence can be generated by "
				":func:`reconstructing regular geological features<reconstruct>` and/or "
				":func:`resolving topological features<resolve_topologies>`.\n"
				"  ::\n"
				"\n"
				"    resolved_topologies = []\n"
				"    pygplates.resolve_topologies('topologies.gpml', 'rotations.rot', resolved_topologies, reconstruction_time=0)\n"
				"    \n"
				"    plate_partitioner = pygplates.PlatePartitioner(resolved_topologies)\n"
				"\n"
				"  .. note:: All partitioning plates should have been generated for the same "
				"reconstruction time otherwise *DifferentTimesInPartitioningPlatesError* is raised.\n"
				"\n"
				"  .. note:: *rotation_model* should be specified (ie, not ``None``) if the partitioning plates "
				"were reconstructed/resolved to a *non-zero* reconstruction time. This enables partitioned "
				"feature geometries to be reverse-reconstructed in :meth:`partition_features`.\n")
		//
		// NOTE: We define this *after* the '__init__' associated with 'plate_partitioner_create_from_reconstruction_geometries()'
		// because boost-python matches most recently defined functions first and this function has a tighter
		// match to its parameters (eg, doesn't have general bp::object arguments that match anything).
		//
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::plate_partitioner_create_from_features,
						bp::default_call_policies(),
						(bp::arg("partitioning_features"),
								bp::arg("rotation_model"),
								bp::arg("reconstruction_time") = GPlatesPropertyValues::GeoTimeInstant(0),
								bp::arg("sort_partitioning_plates") =
										GPlatesApi::SortPartitioningPlates::BY_PARTITION_TYPE_THEN_PLATE_ID)),
				// Specific overload signature...
				"__init__(partitioning_features, rotation_model, [reconstruction_time=0], "
				"[sort_partitioning_plates=SortPartitioningPlates.by_partition_type_then_plate_id])\n"
				"  Create a partitioner by reconstructing/resolving plates from a sequence of plate features.\n"
				"\n"
				"  :param partitioning_features: A sequence of plate features to partition with.\n"
				"  :type partitioning_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
				"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
				"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
				"filename or a sequence of rotation feature collections and/or rotation filenames\n"
				"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
				"or sequence of :class:`FeatureCollection` instances and/or strings\n"
				"  :param reconstruction_time: the specific geological time to reconstruct/resolve the *partitioning_features* to\n"
				"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
				"  :param sort_partitioning_plates: optional sort order of partitioning plates "
				"(defaults to *SortPartitioningPlates.by_partition_type_then_plate_id*)\n"
				"  :type sort_partitioning_plates: One of the values in the *SortPartitioningPlates* table above, or None\n"
				"\n"
				"  The partitioning plates are generated internally by :func:`reconstructing the regular geological features<reconstruct>` "
				"and :func:`resolving the topological features<resolve_topologies>` in *partitioning_features* using the rotation model and "
				"optional reconstruction time.\n"
				"\n"
				"  To create a plate partitioner suitable for partitioning present day geometries/features (ie, *reconstruction_time* is zero):.\n"
				"  ::\n"
				"    \n"
				"    plate_partitioner = pygplates.PlatePartitioner('static_polygons.gpml', 'rotations.rot')\n")
		.def("partition_geometry",
				&GPlatesApi::plate_partitioner_partition_geometry,
				(bp::arg("geometry"),
						bp::arg("partitioned_inside_geometries") = bp::object()/*Py_None*/,
						bp::arg("partitioned_outside_geometries") = bp::object()/*Py_None*/),
				"partition_geometry(geometry, [partitioned_inside_geometries], [partitioned_outside_geometries])\n"
				"  Partitions one or more geometries into partitioning plates.\n"
				"\n"
				"  :param geometry: the geometry, or geometries, to partition\n"
				"  :type geometry: :class:`GeometryOnSphere`, or sequence (eg, ``list`` or ``tuple``) "
				"of :class:`GeometryOnSphere`\n"
				"  :param partitioned_inside_geometries: optional list of geometries partitioned *inside* "
				"the partitioning plates (note that the list is *not* cleared first)\n"
				"  :type partitioned_inside_geometries: ``list`` of 2-tuple "
				"(:class:`ReconstructionGeometry`, ``list`` of :class:`GeometryOnSphere`), or None\n"
				"  :param partitioned_outside_geometries: optional list of geometries partitioned *outside* "
				"all partitioning plates (note that the list is *not* cleared first)\n"
				"  :type partitioned_outside_geometries: ``list`` of :class:`GeometryOnSphere`, or None\n"
				"  :rtype: bool\n"
				"\n"
				"  If *geometry* is inside any partitioning plates (even partially) "
				"then ``True`` is returned and the inside parts of *geometry* are appended to "
				"*partitioned_inside_geometries* (if specified) and the outside parts appended to "
				"*partitioned_outside_geometries* (if specified). Otherwise ``False`` is returned "
				"and *geometry* is appended to *partitioned_outside_geometries* (if specified).\n"
				"\n"
				"  .. note:: Each element in *partitioned_inside_geometries* is a 2-tuple "
				"consisting of a partitioning :class:`ReconstructionGeometry` and a list of the "
				":class:`geometry<GeometryOnSphere>` pieces partitioned into it (note that these pieces "
				"can come from multiple input geometries if *geometry* is a sequence). In contrast, "
				"*partitioned_outside_geometries* is simply a list of :class:`geometries<GeometryOnSphere>` "
				"outside all partitioning plates.\n"
				"\n"
				"  .. warning:: Support for partitioning a :class:`polygon<PolygonOnSphere>` geometry "
				"is partial. See :meth:`PolygonOnSphere.partition` for more details.\n"
				"\n"
				"  To find the length of a polyline partitioned inside all reconstructed static polygons:\n"
				"  ::\n"
				"\n"
				"    polyline_to_partition = pygplates.PolylineOnSphere(...)\n"
				"    polyline_inside_length = 0\n"
				"\n"
				"    reconstructed_static_polygons = []\n"
				"    pygplates.reconstruct('static_polygons.gpml', 'rotations.rot', reconstructed_static_polygons, reconstruction_time=0)\n"
				"\n"
				"    plate_partitioner = pygplates.PlatePartitioner(reconstructed_static_polygons)\n"
				"    partitioned_inside_geometries = []\n"
				"    if plate_partitioner.partition(polyline_to_partition, partitioned_inside_geometries):\n"
				"        for partitioning_recon_geom, inside_geometries in partitioned_inside_geometries:\n"
				"            for inside_geometry in inside_geometries:\n"
				"                polyline_inside_length += inside_geometry.get_arc_length()\n"
				"\n"
				"    polyline_inside_length_in_kms = polyline_inside_length * pygplates.Earth.mean_radius_in_kms\n"
				"\n"
				"  .. seealso:: :meth:`PolygonOnSphere.partition`\n")
		.def("partition_point",
				&GPlatesApi::plate_partitioner_partition_point,
				(bp::arg("point")),
				"partition_point(point)\n"
				"  A convenient alternative to :meth:`partition_geometry`, for a point, that finds the first "
				"partitioning plate (if any) containing the point.\n"
				"\n"
				"  :param point: the point to partition\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (float,float,float) or tuple (float,float)\n"
				"  :rtype: :class:`ReconstructionGeometry` or None\n"
				"\n"
				"  .. note:: ``None`` is returned if *point* is not contained by any partitioning plates.\n"
				"\n"
				"  To find the plate ID of the reconstructed static polygon containing latitude/longitude (0,0):\n"
				"  ::\n"
				"\n"
				"    reconstructed_static_polygons = []\n"
				"    pygplates.reconstruct('static_polygons.gpml', 'rotations.rot', reconstructed_static_polygons, reconstruction_time=0)\n"
				"\n"
				"    plate_partitioner = pygplates.PlatePartitioner(reconstructed_static_polygons)\n"
				"    reconstructed_static_polygon = plate_partitioner.partition_point((0,0))\n"
				"    if reconstructed_static_polygon:\n"
				"        partitioning_plate_id = reconstructed_static_polygon.get_feature().get_reconstruction_plate_id()\n"
				"\n"
				"  .. seealso:: :meth:`PolygonOnSphere.is_point_in_polygon`\n")
		// This is a private method (has leading '_'), and we don't provide a docstring.
		// This method is accessed by pure python API code.
		.def("_get_reconstruction_time",
				&GPlatesApi::plate_partitioner_get_reconstruction_time)
		// This is a private method (has leading '_'), and we don't provide a docstring.
		// This method is accessed by pure python API code.
		.def("_get_rotation_model",
				&GPlatesApi::plate_partitioner_get_rotation_model)
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;
}

#endif // GPLATES_NO_PYTHON
