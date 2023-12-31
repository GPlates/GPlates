/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H
#define GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometry.h"
#include "ReconstructionGeometryVisitor.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"

#include "maths/PolygonOnSphere.h"
#include "maths/PolygonPartitioner.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionTreeCreator;
	class ReconstructMethodRegistry;


	/**
	 * Partitions geometry using dynamic resolved topological boundaries and/or
	 * static reconstructed feature polygons.
	 */
	class GeometryCookieCutter :
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for a sequence of geometries resulting from partitioning a single geometry.
		 */
		typedef GPlatesMaths::PolygonPartitioner::partitioned_geometry_seq_type partitioned_geometry_seq_type;

		/**
		 * Typedef for a partitioning polygon and the geometries partitioned inside it.
		 */
		class Partition
		{
		public:
			Partition(
					const ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry_) :
				reconstruction_geometry(reconstruction_geometry_)
			{ }

			ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry;
			partitioned_geometry_seq_type partitioned_geometries;
		};

		/**
		 * Typedef for a sequence of inside partitions.
		 */
		typedef std::list<Partition> partition_seq_type;


		/**
		 * Enumerated ways to sort plates.
		 */
		enum SortPlates
		{
			SORT_BY_PLATE_ID,
			SORT_BY_PLATE_AREA
		};


		/**
		 * Finds reconstructed polygon geometries to partition other geometry with.
		 *
		 * Topological networks (and their static interior polygons, if any) are used first when
		 * partitioning, then topological boundaries and then regular static polygons.
		 * The static interior polygons of any topological networks are higher priority than the
		 * networks themselves (it's required since the network boundaries contain the interior polygons).
		 *
		 * The partitioning polygons, in each group (ie, static, topological boundary, topological network),
		 * are optionally sorted either sorted by plate ID (from highest plate id to lowest) or
		 * plate area (from largest area to lowest), if @a sort_plates is not none.
		 * This ensures that if there are any overlapping polygons in the (combined) set then
		 * those with the highest plate id will partition before those with lower plate ids, or
		 * those with largest area will partition before those with lower area.
		 *
		 * The reason for preferring plates further from the anchor is, I think,
		 * because it provides more detailed rotations since plates further down the plate
		 * circuit (or tree) are relative to plates higher up the tree and hence provide
		 * extra detail.
		 * And they are presumably smaller so they will partition their area leaving geometry
		 * partitioned outside their area a bigger overlapping plate polygon.
		 *
		 * Ideally the plate boundaries shouldn't overlap at all but it is possible to construct
		 * a set that do overlap.
		 *
		 * NOTE: We also include topological network here even though they are deforming
		 * and not rigid regions. This is because the current topological closed plate polygons
		 * do *not* cover the entire globe and leave holes where there are topological networks.
		 * So we assign plate ids using the topological networks with the understanding that
		 * these are to be treated as rigid regions as a first order approximation (although the
		 * plate ids don't exist in the rotation file so they'll need to be added - for example
		 * the Andes deforming region has plate id 29201 which should be mapped to 201 in
		 * the rotation file).
		 *
		 * Note that if the topological network contains interior static polygons (microblocks)
		 * then they do *not* need to be included in @a reconstructed_static_polygons - they are
		 * found directly through the networks in @a resolved_topological_networks.
		 *
		 * @a partition_point_speed_and_memory determines the speed versus memory trade-off of the
		 * point-in-polygon tests for partitioned *point* geometries.
		 */
		GeometryCookieCutter(
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &> reconstructed_static_polygons,
				boost::optional<const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &> resolved_topological_boundaries,
				boost::optional<const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &> resolved_topological_networks,
				boost::optional<SortPlates> sort_plates = SORT_BY_PLATE_ID,
				GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory = GPlatesMaths::PolygonOnSphere::ADAPTIVE);


		/**
		 * Finds reconstructed polygon geometries to partition other geometry with.
		 *
		 * This is the same as the above constructor except the resolved topological networks,
		 * resolved topological boundaries and reconstructed static polygons are combined
		 * (in @a reconstruction_geometries) and @a group_networks_then_boundaries_then_static_polygons
		 * determines whether they are grouped/sorted in that order or not. If @a sort_plates
		 * is specified then it applies within those sub-groups.
		 */
		GeometryCookieCutter(
				const double &reconstruction_time,
				const std::vector<ReconstructionGeometry::non_null_ptr_type> &reconstruction_geometries,
				bool group_networks_then_boundaries_then_static_polygons = true,
				boost::optional<SortPlates> sort_plates = SORT_BY_PLATE_ID,
				GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory = GPlatesMaths::PolygonOnSphere::ADAPTIVE);


		/**
		 * Finds reconstructed polygon geometries to partition other geometry with.
		 *
		 * This is the same as the above constructor except that the resolved topological networks,
		 * resolved topological boundaries and reconstructed static polygons are reconstructed/resolved
		 * from @a feature_collections.
		 */
		GeometryCookieCutter(
				const double &reconstruction_time,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				bool group_networks_then_boundaries_then_static_polygons = true,
				boost::optional<SortPlates> sort_plates = SORT_BY_PLATE_ID,
				GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory = GPlatesMaths::PolygonOnSphere::ADAPTIVE);


		/**
		 * Returns true if we have partitioning polygons.
		 */
		bool
		has_partitioning_polygons() const;


		/**
		 * Partition @a geometry using the partitioning polygons found in the constructor.
		 *
		 * If the partitioning polygons (passed in the constructor) overlap each other then
		 * @a geometry is partitioned consecutively according to the final ordering of
		 * the partitioning polygons. See the constructor comment for more details.
		 *
		 * On returning @a partitioned_outside_geometries contains any partitioned
		 * geometries that are not inside any partitioning polygons.
		 * If there happen to be no partitioning polygons then @a geometry is added
		 * to @a partitioned_outside_geometries.
		 *
		 * Returns true if @a geometry is inside any partitioning polygons (even partially)
		 * in which case elements were appended to @a partitioned_inside_geometries.
		 */
		bool
		partition_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				boost::optional<partition_seq_type &> partitioned_inside_geometries = boost::none,
				boost::optional<partitioned_geometry_seq_type &> partitioned_outside_geometries = boost::none) const;


		/**
		 * Same as @a partition_geometry except partitions multiple input geometries (instead of one).
		 *
		 * Returns true if any geometry in @a geometries is inside any partitioning polygons (even partially)
		 * in which case elements were appended to @a partitioned_inside_geometries.
		 */
		bool
		partition_geometries(
				const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &geometries,
				boost::optional<partition_seq_type &> partitioned_inside_geometries = boost::none,
				boost::optional<partitioned_geometry_seq_type &> partitioned_outside_geometries = boost::none) const;


		/**
		 * Finds which partitioning polygon boundary contains @a point.
		 *
		 * Returns false if no containing boundaries are found.
		 *
		 * NOTE: If there are overlapping partitioning polygons and the specified point
		 * is inside more that one polygon then the first partitioning polygon containing
		 * the point is chosen (according to the final ordering of the partitioning polygons).
		 * See the constructor comment for more details.
		 */
		boost::optional<const ReconstructionGeometry *>
		partition_point(
				const GPlatesMaths::PointOnSphere &point) const;


		/**
		 * Returns the reconstruction time of the reconstructed partitioning polygons
		 * used to partition geometry with.
		 */
		double
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}

	private:
		/**
		 * Associates a @a ReconstructionGeometry with its polygon structure
		 * for geometry partitioning.
		 */
		class PartitioningGeometry
		{
		public:
			PartitioningGeometry(
					const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
					GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory);


			ReconstructionGeometry::non_null_ptr_to_const_type d_reconstruction_geometry;
			GPlatesMaths::PolygonPartitioner::non_null_ptr_type d_polygon_partitioner;

			//! Used to sort by plate id.
			struct SortPlateIdHighestToLowest
			{
				bool
				operator()(
						const PartitioningGeometry &lhs,
						const PartitioningGeometry &rhs) const;
			};

			//! Used to sort by plate area.
			struct SortPlateAreaHighestToLowest
			{
				bool
				operator()(
						const PartitioningGeometry &lhs,
						const PartitioningGeometry &rhs) const;
			};
		};

		//! Typedef for a sequence of partitioning geometries.
		typedef std::vector<PartitioningGeometry> partitioning_geometry_seq_type;


		/**
		 * Visits reconstruction geometries to add as partitioning geometries.
		 */
		class AddPartitioningReconstructionGeometry :
				public ReconstructionGeometryVisitor
		{
		public:
			// Bring base class visit methods into scope of current class.
			using ReconstructionGeometryVisitor::visit;


			explicit
			AddPartitioningReconstructionGeometry(
					GeometryCookieCutter &geometry_cookie_cutter) :
				d_geometry_cookie_cutter(geometry_cookie_cutter)
			{  }

			// Derivations of ReconstructedFeatureGeometry default to its implementation...
			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg);

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb);

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn);

		private:
			GeometryCookieCutter &d_geometry_cookie_cutter;
		};


		//! The partitioning geometries.
		partitioning_geometry_seq_type d_partitioning_geometries;

		double d_reconstruction_time;

		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory d_partition_point_speed_and_memory;


		/**
		 * Adds @a ReconstructionGeometry objects, unsorted by type, as partitioning geometries.
		 */
		void
		add_partitioning_reconstruction_geometries(
				const std::vector<ReconstructionGeometry::non_null_ptr_type> &reconstruction_geometries,
				boost::optional<SortPlates> sort_plates);


		/**
		 * Adds @a ResolvedTopologicalNetwork objects as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_networks(
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				boost::optional<SortPlates> sort_plates);

		/**
		 * Add a @a ResolvedTopologicalNetwork as a partitioning geometry.
		 */
		void
		add_partitioning_resolved_topological_network(
				const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_topological_network);


		/**
		 * Adds @a ResolvedTopologicalBoundary objects as partitioning geometries.
		 */
		void
		add_partitioning_resolved_topological_boundaries(
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				boost::optional<SortPlates> sort_plates);

		/**
		 * Add a @a ResolvedTopologicalBoundary as a partitioning geometry.
		 */
		void
		add_partitioning_resolved_topological_boundary(
				const ResolvedTopologicalBoundary::non_null_ptr_type &resolved_topological_boundary);


		/**
		 * Adds @a ReconstructedFeatureGeometry objects as partitioning geometries.
		 */
		void
		add_partitioning_reconstructed_feature_polygons(
				const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				boost::optional<SortPlates> sort_plates);

		/**
		 * Add a @a ReconstructedFeatureGeometry as a partitioning geometry, if it has a *polygon* geometry.
		 */
		void
		add_partitioning_reconstructed_feature_polygon(
				const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry);


		/**
		 * Sort plates within a partitioning group.
		 */
		void
		sort_plates_in_partitioning_group(
				const partitioning_geometry_seq_type::iterator &partitioning_group_begin,
				const partitioning_geometry_seq_type::iterator &partitioning_group_end,
				SortPlates sort_plates);
	};
}

#endif // GPLATES_APP_LOGIC_GEOMETRYCOOKIECUTTER_H
