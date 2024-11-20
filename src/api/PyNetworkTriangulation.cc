/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#include <boost/cast.hpp>

#include "PyNetworkTriangulation.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "utils/Earth.h"


namespace bp = boost::python;


unsigned int
GPlatesApi::NetworkTriangulation::Triangle::get_vertex_index(
		int index) const
{
	if (index < 0 || index >= 3)
	{
		PyErr_SetString(PyExc_ValueError, "*index* should be in the range [0, 2]");
		bp::throw_error_already_set();
	}

	return d_face_handle->vertex(index)->get_vertex_index();
}


GPlatesMaths::Vector3D
GPlatesApi::NetworkTriangulation::Vertex::get_velocity(
		const double &velocity_delta_time,
		GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
		GPlatesAppLogic::VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	// Velocity delta time must be positive.
	if (velocity_delta_time <= 0)
	{
		PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
		bp::throw_error_already_set();
	}

	return d_vertex_handle->calc_velocity_vector(
			velocity_delta_time,
			velocity_delta_time_type,
			velocity_units,
			earth_radius_in_kms);
}


template <class ItemType>
typename GPlatesApi::NetworkTriangulation::ItemsView<ItemType>::item_type
GPlatesApi::NetworkTriangulation::ItemsView<ItemType>::get_item(
		long index) const
{
	if (index < 0)
	{
		index += d_items.size();
	}

	if (index >= boost::numeric_cast<long>(d_items.size()) ||
		index < 0)
	{
		PyErr_SetString(PyExc_IndexError, "Index out of range");
		bp::throw_error_already_set();
	}

	return d_items[index];
}

// Explicitly instantiate our two uses of NetworkTriangulation::ItemsView<ItemType>::get_item().
template GPlatesApi::NetworkTriangulation::ItemsView<GPlatesApi::NetworkTriangulation::Triangle>::item_type
		GPlatesApi::NetworkTriangulation::ItemsView<GPlatesApi::NetworkTriangulation::Triangle>::get_item(long) const;
template GPlatesApi::NetworkTriangulation::ItemsView<GPlatesApi::NetworkTriangulation::Vertex>::item_type
		GPlatesApi::NetworkTriangulation::ItemsView<GPlatesApi::NetworkTriangulation::Vertex>::get_item(long) const;


GPlatesApi::NetworkTriangulation::non_null_ptr_type
GPlatesApi::NetworkTriangulation::create(
		GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type resolved_topological_network)
{
	non_null_ptr_type network_triangulation(new NetworkTriangulation(resolved_topological_network));

	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_topological_network->get_triangulation_network().get_delaunay_2();

	// Get the number of finite faces and vertices in the Delaunay triangulation.
	const unsigned int num_faces = delaunay_triangulation_2.number_of_faces();
	const unsigned int num_vertices = delaunay_triangulation_2.number_of_vertices();

	// Resize triangle and vertex arrays to fit.
	network_triangulation->d_triangles.resize(num_faces);
	network_triangulation->d_vertices.resize(num_vertices);

	// Iterate over the individual faces of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_iter = delaunay_triangulation_2.finite_faces_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_end = delaunay_triangulation_2.finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
		// Create the triangle.
		const Triangle triangle(network_triangulation, finite_faces_2_iter);

		// Index of triangle in the triangulation.
		const unsigned int face_index = finite_faces_2_iter->get_face_index();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				face_index < network_triangulation->d_triangles.size(),
				GPLATES_ASSERTION_SOURCE);

		// Add the triangle.
		network_triangulation->d_triangles[face_index] = triangle;
	}

	// Iterate over the vertices of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_vertices_iterator
			finite_vertices_2_iter = delaunay_triangulation_2.finite_vertices_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_vertices_iterator
			finite_vertices_2_end = delaunay_triangulation_2.finite_vertices_end();
	for ( ; finite_vertices_2_iter != finite_vertices_2_end; ++finite_vertices_2_iter)
	{
		// Create the vertex.
		const Vertex vertex(network_triangulation, finite_vertices_2_iter);

		// Index of vertex in the triangulation.
		const unsigned int vertex_index = finite_vertices_2_iter->get_vertex_index();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertex_index < network_triangulation->d_vertices.size(),
				GPLATES_ASSERTION_SOURCE);

		// Add the vertex.
		network_triangulation->d_vertices[vertex_index] = vertex;
	}

	return network_triangulation;
}


void
export_network_triangulation()
{
	{
		//
		// NetworkTriangulation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		bp::scope network_triangulation_wrapper_class = bp::class_<
				GPlatesApi::NetworkTriangulation,
				GPlatesApi::NetworkTriangulation::non_null_ptr_type,
				boost::noncopyable>(
						"NetworkTriangulation",
						"The Delaunay triangulation of a :class:`resolved topological network <ResolvedTopologicalNetwork>`.\n"
						"\n"
						"This is the triangulation of the convex hull of the vertices obtained from the network's resolved boundary (polygon), "
						"and any interior rigid blocks (polygons) and any interior geometries (points or lines).\n"
						"\n"
						".. seealso:: :ref:`pygplates_primer_network_triangulation` in the *Primer* documentation."
						"\n"
						".. versionadded:: 0.50\n",
						bp::no_init)
			.def("get_triangles",
					&GPlatesApi::NetworkTriangulation::get_triangles_view,
					"get_triangles\n"
					"  Returns a read-only sequence of the triangles in this triangulation.\n"
					"\n"
					"  :rtype: a read-only sequence of :class:`NetworkTriangulation.Triangle`\n"
					"\n"
					"  The following operations for accessing the triangles in the returned read-only sequence are supported:\n"
					"\n"
					"  =========================== =======================================================================\n"
					"  Operation                   Result\n"
					"  =========================== =======================================================================\n"
					"  ``len(seq)``                number of triangles in the triangulation\n"
					"  ``for t in seq``            iterates over the triangles *t* in the triangulation\n"
					"  ``seq[i]``                  the triangle in the triangulation at index *i*\n"
					"  =========================== =======================================================================\n"
					"\n"
					"  The following example demonstrates some uses of the above operations:\n"
					"  ::\n"
					"\n"
					"    network_triangulation = resolved_topological_network.get_network_triangulation()\n"
					"    triangles = network_triangulation.get_triangles()\n"
					"    vertices = network_triangulation.get_vertices()\n"
					"    for triangle in triangles:\n"
					"        triangle_vertex_0 = vertices[triangle.get_vertex_index(0)]\n"
					"        triangle_vertex_1 = vertices[triangle.get_vertex_index(1)]\n"
					"        triangle_vertex_2 = vertices[triangle.get_vertex_index(2)]\n"
					"        triangle_strain_rate = triangle.strain_rate\n"
					"\n"
					"  .. note:: The returned sequence is *read-only* and cannot be modified.\n")
			.def("get_vertices",
					&GPlatesApi::NetworkTriangulation::get_vertices_view,
					"get_vertices\n"
					"  Returns a read-only sequence of the vertices in this triangulation.\n"
					"\n"
					"  :rtype: a read-only sequence of :class:`NetworkTriangulation.Vertex`\n"
					"\n"
					"  The following operations for accessing the vertices in the returned read-only sequence are supported:\n"
					"\n"
					"  =========================== =======================================================================\n"
					"  Operation                   Result\n"
					"  =========================== =======================================================================\n"
					"  ``len(seq)``                number of vertices in the triangulation\n"
					"  ``for v in seq``            iterates over the vertices *v* in the triangulation\n"
					"  ``seq[i]``                  the vertex in the triangulation at index *i*\n"
					"  =========================== =======================================================================\n"
					"\n"
					"  The following example demonstrates some uses of the above operations:\n"
					"  ::\n"
					"\n"
					"    network_triangulation = resolved_topological_network.get_network_triangulation()\n"
					"    vertices = network_triangulation.get_vertices()\n"
					"    for vertex in vertices:\n"
					"        vertex_position = vertex.position\n"
					"        vertex_strain_rate = vertex.strain_rate\n"
					"        vertex_velocity = vertex.get_velocity()\n"
					"\n"
					"  .. note:: The returned sequence is *read-only* and cannot be modified.\n")
		;

		//
		// NetworkTriangulation.Triangle - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class NetworkTriangulation (due to above 'bp::scope').
		bp::class_<GPlatesApi::NetworkTriangulation::Triangle>(
						"Triangle",
						"A triangle in a :class:`network triangulation <NetworkTriangulation>`.\n"
						"\n"
						"Triangles are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
						"\n"
						".. seealso:: :ref:`pygplates_primer_network_triangulation` in the *Primer* documentation."
						"\n"
						".. versionadded:: 0.50\n",
						bp::no_init)
			.def("get_vertex_index",
					&GPlatesApi::NetworkTriangulation::Triangle::get_vertex_index,
					(bp::arg("index")),
					"get_vertex_index(index)\n"
					"  Returns the vertex index into :meth:`NetworkTriangulation.get_vertices` of one of this triangle's three vertices.\n"
					"\n"
					"  :param index: the index of this triangle's vertex (in the range [0, 2])\n"
					"  :type index: int\n"
					"  :rtype: int\n"
					"  :raises: ValueError if *index* is not in the range [0, 2]\n"
					"\n"
					"  The following example demonstrates how to access the :meth:`triangulation vertices <NetworkTriangulation.get_vertices>` "
					"from a triangle's vertex indices:\n"
					"  ::\n"
					"\n"
					"    network_triangulation = resolved_topological_network.get_network_triangulation()\n"
					"    triangles = network_triangulation.get_triangles()\n"
					"    vertices = network_triangulation.get_vertices()\n"
					"    for triangle in triangles:\n"
					"        triangle_vertex_0 = vertices[triangle.get_vertex_index(0)]\n"
					"        triangle_vertex_1 = vertices[triangle.get_vertex_index(1)]\n"
					"        triangle_vertex_2 = vertices[triangle.get_vertex_index(2)]\n")
			.add_property("is_in_deforming_region",
					&GPlatesApi::NetworkTriangulation::Triangle::is_in_deforming_region,
					"Whether this triangle is *in* the deforming region of the network.\n"
					"\n"
					"  :type: bool\n"
					"\n"
					"  .. note:: A triangle is *in* the deforming region if its centroid is in the deforming region (where the deforming region "
					"is defined to be *inside* the network's boundary polygon but *outside* any interior rigid block polygons).\n")
			.add_property("strain_rate",
					&GPlatesApi::NetworkTriangulation::Triangle::get_strain_rate,
					"Return the constant strain rate across this triangle.\n"
					"\n"
					"  :type: :class:`StrainRate`\n"
					"\n"
					"  .. note:: This will be ``pygplates.StrainRate.zero`` if this triangle is *not* :attr:`deforming <is_in_deforming_region>`.\n")
			// Due to the numerical tolerance in comparisons we cannot make hashable.
			// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
			.def(GPlatesApi::NoHashDefVisitor(false, true))
			.def(bp::self == bp::self)
			.def(bp::self != bp::self)
		;

		//
		// A wrapper around view access to the *triangles* of a network triangulation.
		//
		// We don't document this wrapper (using docstrings) since it's documented in "NetworkTriangulation".
		bp::class_<GPlatesApi::NetworkTriangulation::triangles_view_type>(
				// Prefix with '_' so users know it's an implementation detail (they should not be accessing it directly).
				"_TrianglesView",
				bp::no_init)
			.def("__iter__",
					bp::iterator<const GPlatesApi::NetworkTriangulation::triangles_view_type>())
			.def("__len__",
					&GPlatesApi::NetworkTriangulation::triangles_view_type::get_number_of_items)
			.def("__getitem__",
					&GPlatesApi::NetworkTriangulation::triangles_view_type::get_item)
		;

		//
		// NetworkTriangulation.Vertex - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class NetworkTriangulation (due to above 'bp::scope').
		bp::class_<GPlatesApi::NetworkTriangulation::Vertex>(
						"Vertex",
						"A vertex in a :class:`network triangulation <NetworkTriangulation>`.\n"
						"\n"
						"Vertices are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
						"\n"
						".. seealso:: :ref:`pygplates_primer_network_triangulation` in the *Primer* documentation."
						"\n"
						".. versionadded:: 0.50\n",
						bp::no_init)
			.add_property("position",
					&GPlatesApi::NetworkTriangulation::Vertex::get_position,
					"Return the position of this vertex.\n"
					"\n"
					"  :type: :class:`PointOnSphere`\n")
			.def("get_velocity",
					&GPlatesApi::NetworkTriangulation::Vertex::get_velocity,
					(bp::arg("velocity_delta_time") = 1.0,
							bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
							bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
							bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS),
					"get_velocity([velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], "
					"[velocity_units=pygplates.VelocityUnits.kms_per_my], [earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms])\n"
					"Returns the velocity of this vertex.\n"
					"\n"
					"  :param velocity_delta_time: The time delta used to calculate velocity (defaults to 1 Myr).\n"
					"  :type velocity_delta_time: float\n"
					"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
					"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
					"  :type velocity_delta_time_type: *VelocityDeltaTimeType.t_plus_delta_t_to_t*, "
					"*VelocityDeltaTimeType.t_to_t_minus_delta_t* or *VelocityDeltaTimeType.t_plus_minus_half_delta_t*\n"
					"  :param velocity_units: whether to return velocity as *kilometres per million years* or "
					"*centimetres per year* (defaults to *kilometres per million years*)\n"
					"  :type velocity_units: *VelocityUnits.kms_per_my* or *VelocityUnits.cms_per_yr*\n"
					"  :param earth_radius_in_kms: the radius of the Earth in *kilometres* (defaults to ``pygplates.Earth.mean_radius_in_kms``)\n"
					"  :type earth_radius_in_kms: float\n"
					"  :rtype: :class:`Vector3D`\n"
					"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n")
			.add_property("strain_rate",
					&GPlatesApi::NetworkTriangulation::Vertex::get_strain_rate,
					"Return the strain rate at this vertex.\n"
					"\n"
					"  :type: :class:`StrainRate`\n"
					"\n"
					"  .. note:: This is the area-averaged strain rate of :attr:`deforming <NetworkTriangulation.Triangle.is_in_deforming_region>` "
					"triangles incident to this vertex.\n")
			// Due to the numerical tolerance in comparisons we cannot make hashable.
			// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
			.def(GPlatesApi::NoHashDefVisitor(false, true))
			.def(bp::self == bp::self)
			.def(bp::self != bp::self)
		;

		//
		// A wrapper around view access to the *vertices* of a network triangulation.
		//
		// We don't document this wrapper (using docstrings) since it's documented in "NetworkTriangulation".
		bp::class_<GPlatesApi::NetworkTriangulation::vertices_view_type>(
				// Prefix with '_' so users know it's an implementation detail (they should not be accessing it directly).
				"_VerticesView",
				bp::no_init)
			.def("__iter__",
					bp::iterator<const GPlatesApi::NetworkTriangulation::vertices_view_type>())
			.def("__len__",
					&GPlatesApi::NetworkTriangulation::vertices_view_type::get_number_of_items)
			.def("__getitem__",
					&GPlatesApi::NetworkTriangulation::vertices_view_type::get_item)
		;
	}

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::NetworkTriangulation>();

	// Enable boost::optional<NetworkTriangulation::Triangle> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::NetworkTriangulation::Triangle>();

	// Enable boost::optional<NetworkTriangulation::Vertex> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::NetworkTriangulation::Vertex>();
}
