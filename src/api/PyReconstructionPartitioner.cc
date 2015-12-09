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
#include <boost/shared_ptr.hpp>

#include "PyReconstructionPartitioner.h"

#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "maths/MathsUtils.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>
	reconstruction_partitioner_create(
			bp::object reconstruction_geometries) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python reconstruction geometries sequence.
		bp::stl_input_iterator<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
				reconstruction_geometries_begin(reconstruction_geometries),
				reconstruction_geometries_end;

		// Copy into a vector.
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> recon_geometries_vector;
		std::copy(
				reconstruction_geometries_begin,
				reconstruction_geometries_end,
				std::back_inserter(recon_geometries_vector));

		// If there happen to be no reconstruction geometries then default the reconstruction time to zero.
		const double reconstruction_time = !recon_geometries_vector.empty()
				? recon_geometries_vector[0]->get_reconstruction_time()
				: 0;

		// Make sure all reconstruction times are the same.
		for (unsigned int n = 1; n < recon_geometries_vector.size(); ++n)
		{
			GPlatesGlobal::Assert<DifferentTimesInPartitioningReconstructionGeometriesException>(
					GPlatesMaths::are_geo_times_approximately_equal(
							recon_geometries_vector[n]->get_reconstruction_time(),
							reconstruction_time),
					GPLATES_ASSERTION_SOURCE);
		}

		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_static_polygons;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				recon_geometries_vector.begin(),
				recon_geometries_vector.end(),
				reconstructed_static_polygons);

		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				recon_geometries_vector.begin(),
				recon_geometries_vector.end(),
				resolved_topological_boundaries);

		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				recon_geometries_vector.begin(),
				recon_geometries_vector.end(),
				resolved_topological_networks);

		return boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>(
				new GPlatesAppLogic::GeometryCookieCutter(
						reconstruction_time,
						reconstructed_static_polygons,
						resolved_topological_boundaries,
						resolved_topological_networks));
	}

	bool
	reconstruction_partitioner_partition(
			const GPlatesAppLogic::GeometryCookieCutter &reconstruction_partitioner,
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
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

		const bool geometry_inside_any_partitions = reconstruction_partitioner.partition_geometry(
				geometry,
				partitioned_inside_geometries,
				partitioned_outside_geometries);

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
	reconstruction_partitioner_partition_point(
			const GPlatesAppLogic::GeometryCookieCutter &reconstruction_partitioner,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		boost::optional<const GPlatesAppLogic::ReconstructionGeometry *> partitioning_reconstruction_geom =
				reconstruction_partitioner.partition_point(point_on_sphere);
		if (!partitioning_reconstruction_geom)
		{
			return boost::none;
		}

		return GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type(
				partitioning_reconstruction_geom.get());
	}
}


void
export_reconstruction_partitioner()
{
	//
	// ReconstructionPartitioner - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::GeometryCookieCutter,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>,
			boost::noncopyable>(
					"ReconstructionPartitioner",
					"Partition geometries using dynamic resolved topological boundaries and/or static reconstructed feature polygons.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::reconstruction_partitioner_create,
						bp::default_call_policies(),
						(bp::arg("partitioning_reconstruction_geometries"))),
				"__init__(partitioning_reconstruction_geometries)\n"
				"  Create a geometry partitioner from a sequence of reconstruction geometries.\n"
				"\n"
				"  :param partitioning_reconstruction_geometries: A sequence of reconstruction geometries to partition with.\n"
				"  :type partitioning_reconstruction_geometries: Any sequence of :class:`ReconstructionGeometry`\n"
				"  :raises: DifferentTimesInPartitioningReconstructionGeometriesError if all "
				"partitioning reconstruction geometries do not have the same "
				":meth:`reconstruction times<ReconstructionGeometry.get_reconstruction_time>`\n"
				"\n"
				"  The *partitioning_reconstruction_geometries* sequence can be generated by "
				":func:`reconstructing regular geological features<reconstruct>` and/or "
				":func:`resolving topological features<resolve_topologies>`.\n"
				"  ::\n"
				"\n"
				"    resolved_topologies = []\n"
				"    pygplates.resolve_topologies('topologies.gpml', 'rotations.rot', resolved_topologies, reconstruction_time)\n"
				"    \n"
				"    reconstruction_partitioner = pygplates.ReconstructionPartitioner(resolved_topologies)\n"
				"\n"
				"  Only those types of :class:`reconstruction geometries<ReconstructionGeometry>` "
				"that contain a polygon boundary are actually used for partitioning. For resolved topologies "
				"this includes :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`. "
				"For reconstructed geometries, a :class:`ReconstructedFeatureGeometry` is only included if its "
				"reconstructed geometry is a :class:`PolygonOnSphere`.\n"
				"\n"
				"  .. note:: If the partitioning polygons overlap each other then their final ordering "
				"determines the partitioning results (see :meth:`partition` and :meth:`partition_point`). "
				"Resolved topologies do not tend to overlap, but reconstructed static polygons do overlap "
				"and hence the sorting order becomes relevant.\n"
				"\n"
				"  .. note:: All partitioning reconstruction geometries should have been generated for the same "
				"reconstruction time otherwise *DifferentTimesInPartitioningReconstructionGeometriesError* is raised.\n")
		.def("partition",
				&GPlatesApi::reconstruction_partitioner_partition,
				(bp::arg("geometry"),
						bp::arg("partitioned_inside_geometries") = bp::object()/*Py_None*/,
						bp::arg("partitioned_outside_geometries") = bp::object()/*Py_None*/),
				"partition(geometry, [partitioned_inside_geometries], [partitioned_outside_geometries])\n"
				"  Finds the first partitioning reconstruction geometry (if any) that contains a point.\n"
				"\n"
				"  :param geometry: the geometry to partition\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param partitioned_inside_geometries: optional list of geometries partitioned *inside* "
				"the partitioning reconstruction geometries (note that the list is *not* cleared first)\n"
				"  :type partitioned_inside_geometries: ``list`` of 2-tuple "
				"(:class:`ReconstructionGeometry`, ``list`` of :class:`GeometryOnSphere`), or None\n"
				"  :param partitioned_outside_geometries: optional list of geometries partitioned *outside* "
				"all partitioning reconstruction geometries (note that the list is *not* cleared first)\n"
				"  :type partitioned_outside_geometries: ``list`` of :class:`GeometryOnSphere`, or None\n"
				"  :rtype: bool\n"
				"\n"
				"  If *geometry* is inside any partitioning reconstruction geometries (even partially) "
				"then ``True`` is returned and the inside parts of *geometry* are appended to "
				"*partitioned_inside_geometries* (if specified) and the outside parts appended to "
				"*partitioned_outside_geometries* (if specified). Otherwise ``False`` is returned "
				"and *geometry* is appended to *partitioned_outside_geometries* (if specified).\n"
				"\n"
				"  .. note:: Each element in *partitioned_inside_geometries* is a 2-tuple "
				"consisting of a partitioning :class:`ReconstructionGeometry` and a list of the "
				":class:`geometry<GeometryOnSphere>` pieces partitioned into it. In contrast, "
				"*partitioned_outside_geometries* is simply a list of :class:`geometries<GeometryOnSphere>` "
				"outside all partitioning reconstruction geometries.\n"
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
				"    pygplates.reconstruct('static_polygons.gpml', 'rotations.rot', reconstructed_static_polygons, reconstruction_time)\n"
				"\n"
				"    reconstruction_partitioner = pygplates.ReconstructionPartitioner(reconstructed_static_polygons)\n"
				"    partitioned_inside_geometries = []\n"
				"    if reconstruction_partitioner.partition(polyline_to_partition, partitioned_inside_geometries):\n"
				"        for partitioning_recon_geom, inside_geometries in partitioned_inside_geometries:\n"
				"            for inside_geometry in inside_geometries:\n"
				"                polyline_inside_length += inside_geometry.get_arc_length()\n"
				"\n"
				"    polyline_inside_length_in_kms = polyline_inside_length * pygplates.Earth.mean_radius_in_kms\n"
				"\n"
				"  .. seealso:: :meth:`PolygonOnSphere.partition`\n")
		.def("partition_point",
				&GPlatesApi::reconstruction_partitioner_partition_point,
				(bp::arg("point")),
				"partition_point(point)\n"
				"  A convenient alternative to :meth:`partition`, for a point, that finds the first "
				"partitioning reconstruction geometry (if any) containing the point.\n"
				"\n"
				"  :param point: the point to partition\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (float,float,float) or tuple (float,float)\n"
				"  :rtype: :class:`ReconstructionGeometry` or None\n"
				"\n"
				"  .. note:: ``None`` is returned if *point* is not contained by any partitioning reconstruction geometries.\n"
				"\n"
				"  To find the plate ID of the reconstructed static polygon containing latitude/longitude (0,0):\n"
				"  ::\n"
				"\n"
				"    reconstructed_static_polygons = []\n"
				"    pygplates.reconstruct('static_polygons.gpml', 'rotations.rot', reconstructed_static_polygons, reconstruction_time)\n"
				"\n"
				"    reconstruction_partitioner = pygplates.ReconstructionPartitioner(reconstructed_static_polygons)\n"
				"    reconstructed_static_polygon = reconstruction_partitioner.partition_point((0,0))\n"
				"    if reconstructed_static_polygon:\n"
				"        partitioning_plate_id = reconstructed_static_polygon.get_feature().get_reconstruction_plate_id()\n"
				"\n"
				"  .. seealso:: :meth:`PolygonOnSphere.is_point_in_polygon`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;
}

#endif // GPLATES_NO_PYTHON
