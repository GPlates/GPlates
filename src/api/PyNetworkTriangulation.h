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

#ifndef GPLATES_API_PY_NETWORK_TRIANGULATION_H
#define GPLATES_API_PY_NETWORK_TRIANGULATION_H

#include <cstdlib> // For std::size_t
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTriangulationDelaunay2.h"
#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{

	/**
	 * Information contained in Delaunay triangulation of a resolved topological network.
	 */
	class NetworkTriangulation :
			public GPlatesUtils::ReferenceCount<NetworkTriangulation>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<NetworkTriangulation> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const NetworkTriangulation> non_null_ptr_to_const_type;


		class Vertex;  // forward declaration

		class Triangle :
				public boost::equality_comparable<Triangle>
		{
		public:
			Triangle(
					GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network,
					const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle &face_handle) :
				d_resolved_topological_network(resolved_topological_network),
				d_face_handle(face_handle)
			{  }

			bool
			operator==(
					const Triangle &other) const
			{
				return d_resolved_topological_network == other.d_resolved_topological_network &&
						d_face_handle == other.d_face_handle;
			}

			std::size_t
			hash() const
			{
				// Use the hash of the face handle.
				return std::hash<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>{}(d_face_handle);
			}


			Vertex
			get_vertex(
					int index) const;

			bool
			is_in_deforming_region() const
			{
				return d_face_handle->is_in_deforming_region();
			}

			GPlatesAppLogic::DeformationStrainRate
			get_strain_rate() const
			{
				return d_face_handle->get_deformation_info().get_strain_rate();
			}

		private:
			// Keep resolved topological network alive since we're referencing an internal handle that it owns.
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type d_resolved_topological_network;
			GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle d_face_handle;
		};

		class Vertex :
				public boost::equality_comparable<Vertex>
		{
		public:
			Vertex(
					GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network,
					const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle &vertex_handle) :
				d_resolved_topological_network(resolved_topological_network),
				d_vertex_handle(vertex_handle)
			{  }

			bool
			operator==(
					const Vertex &other) const
			{
				return d_resolved_topological_network == other.d_resolved_topological_network &&
					d_vertex_handle == other.d_vertex_handle;
			}

			std::size_t
			hash() const
			{
				// Use the hash of the vertex handle.
				return std::hash<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle>{}(d_vertex_handle);
			}

			GPlatesMaths::PointOnSphere
			get_position() const
			{
				return d_vertex_handle->get_point_on_sphere();
			}

			GPlatesMaths::Vector3D
			get_velocity(
					const double &velocity_delta_time,
					GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
					GPlatesAppLogic::VelocityUnits::Value velocity_units,
					const double &earth_radius_in_kms) const;

			GPlatesAppLogic::DeformationStrainRate
			get_strain_rate() const
			{
				return d_vertex_handle->get_deformation_info().get_strain_rate();
			}

		private:
			// Keep resolved topological network alive since we're referencing an internal handle that it owns.
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type d_resolved_topological_network;
			GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle d_vertex_handle;
		};


		/**
		 * Wrapper class for functions accessing the items (triangles or vertices) of a deforming triangulation.
		 */
		template <class ItemType>
		class ItemsView
		{
		public:

			// Typedef for the item itself.
			//
			// Note: The item should not be none (unless something went wrong during initialisation of the triangulation).
			typedef boost::optional<ItemType> item_type;
			// Typedef for items sequence and iterator.
			typedef std::vector<item_type> item_seq_type;
			typedef typename item_seq_type::const_iterator const_iterator;

			ItemsView(
					NetworkTriangulation::non_null_ptr_to_const_type network_triangulation,
					const item_seq_type &items) :
				d_network_triangulation(network_triangulation),
				d_items(items)
			{  }

			const_iterator
			begin() const
			{
				return d_items.begin();
			}

			const_iterator
			end() const
			{
				return d_items.end();
			}

			unsigned int
			get_number_of_items() const
			{
				return d_items.size();
			}

			//
			// Support for "__getitem__".
			//
			// Note: The returned item should not be none
			//       (unless something went wrong during initialisation of the triangulation).
			//
			item_type
			get_item(
					long index) const;

		private:
			NetworkTriangulation::non_null_ptr_to_const_type d_network_triangulation;  // just to keep items reference valid
			const item_seq_type &d_items;
		};

		typedef ItemsView<Triangle> triangles_view_type;
		typedef ItemsView<Vertex> vertices_view_type;


		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_topological_network);


		/**
		 * Return the triangles in the Delaunay triangulation.
		 *
		 * Note: None of the returned triangles should not be none
		 *       (unless something went wrong during initialisation of the triangulation).
		 */
		const std::vector<boost::optional<Triangle>> &
		get_triangles() const
		{
			return d_triangles;
		}

		/**
		 * Return the vertices in the Delaunay triangulation.
		 *
		 * Note: None of the returned vertices should not be none
		 *       (unless something went wrong during initialisation of the triangulation).
		 */
		const std::vector<boost::optional<Vertex>> &
		get_vertices() const
		{
			return d_vertices;
		}


		/**
		 * Return wrapper class for accessing triangles in the Delaunay triangulation..
		 */
		triangles_view_type
		get_triangles_view() const
		{
			return triangles_view_type(this, get_triangles());
		}

		/**
		 * Return wrapper class for accessing vertices in the Delaunay triangulation..
		 */
		vertices_view_type
		get_vertices_view() const
		{
			return vertices_view_type(this, get_vertices());
		}

	private:

		std::vector<boost::optional<Triangle>> d_triangles;
		std::vector<boost::optional<Vertex>> d_vertices;
	};
}

#endif // GPLATES_API_PY_NETWORK_TRIANGULATION_H
