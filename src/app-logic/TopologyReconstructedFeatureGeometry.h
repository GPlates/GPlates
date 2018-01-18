/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCTEDFEATUREGEOMETRY_H
#define GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCTEDFEATUREGEOMETRY_H

#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrain.h"
#include "ReconstructedFeatureGeometry.h"
#include "TopologyReconstruct.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"


namespace GPlatesAppLogic
{
	/**
	 * A feature geometry that has been reconstructed using topologies (rigid plates and deforming networks).
	 *
	 * The main difference with ReconstructedFeatureGeometry is TopologyReconstructedFeatureGeometry went
	 * through the topology reconstruction pipeline and parts of its geometry may get subducted going
	 * forward in time and consumed by mid-ocean ridge going backward in time. It also stores
	 * deformation strain rates and total strains as a result of deformation via deforming networks.
	 */
	class TopologyReconstructedFeatureGeometry :
			public ReconstructedFeatureGeometry
	{
	public:
		//! A convenience typedef for a non-null shared pointer to a non-const @a TopologyReconstructedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyReconstructedFeatureGeometry> non_null_ptr_type;

		//! A convenience typedef for a non-null shared pointer to a const @a TopologyReconstructedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyReconstructedFeatureGeometry> non_null_ptr_to_const_type;


		//! Typedef for a sequence of per-geometry-point deformation instantaneous strain rates.
		typedef std::vector<DeformationStrainRate> point_deformation_strain_rate_seq_type;

		//! Typedef for a sequence of per-geometry-point deformation accumulated/total strains.
		typedef std::vector<DeformationStrain> point_deformation_total_strain_seq_type;


		/**
		 * Create a TopologyReconstructedFeatureGeometry instance.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &topology_reconstruct_geometry_time_span,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new TopologyReconstructedFeatureGeometry(
							reconstruction_tree,
							reconstruction_tree_creator,
							feature_handle,
							property_iterator,
							topology_reconstruct_geometry_time_span,
							reconstruction_plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}


		/**
		 * Returns the reconstructed geometry.
		 *
		 * This overrides the base class @a ReconstructedFeatureGeometry method.
		 */
		virtual
		geometry_ptr_type
		reconstructed_geometry() const;


		/**
		 * Returns the reconstructed geometry points in @a reconstructed_geometry.
		 */
		void
		get_reconstructed_points(
				point_seq_type &reconstructed_points) const
		{
			get_geometry_data(reconstructed_points);
		}


		/**
		 * Returns the per-geometry-point deformation strain rates.
		 *
		 * Note: Each strain rate maps to a point in @a get_reconstructed_points.
		 *
		 * Note: The number of strain rates is guaranteed to match points in @a get_reconstructed_points.
		 */
		void
		get_point_deformation_strain_rates(
				point_deformation_strain_rate_seq_type &strain_rates) const
		{
			get_geometry_data(boost::none/*points*/, strain_rates);
		}

		/**
		 * Returns the per-geometry-point deformation (total) strains.
		 *
		 * Note: Each strain maps to a point in @a get_reconstructed_points.
		 *
		 * Note: The number of strains is guaranteed to match points in @a get_reconstructed_points.
		 */
		void
		get_point_deformation_total_strains(
				point_deformation_total_strain_seq_type &total_strains) const
		{
			get_geometry_data(boost::none/*points*/, boost::none/*strain_rates*/, total_strains);
		}


		/**
		 * Combines @a get_reconstructed_points, @a get_point_deformation_strain_rates and
		 * @a get_point_deformation_total_strains (for more efficient access).
		 */
		void
		get_geometry_data(
				boost::optional<point_seq_type &> reconstructed_points = boost::none,
				boost::optional<point_deformation_strain_rate_seq_type &> strain_rates = boost::none,
				boost::optional<point_deformation_total_strain_seq_type &> strains = boost::none) const;


		/**
		 * Returns the time range over which this reconstructed feature was reconstructed using topologies.
		 */
		TimeSpanUtils::TimeRange
		get_time_range() const
		{
			return d_topology_reconstruct_geometry_time_span->get_time_range();
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	private:

		/**
		 * The source of our geometry and deformation strain rates and total strains.
		 */
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_topology_reconstruct_geometry_time_span;


		/**
		 * Instantiate a topology reconstructed feature geometry.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		TopologyReconstructedFeatureGeometry(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &topology_reconstruct_geometry_time_span,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_) :
			ReconstructedFeatureGeometry(
					reconstruction_tree_,
					reconstruction_tree_creator,
					feature_handle,
					property_iterator,
					ReconstructMethod::BY_PLATE_ID,
					reconstruction_plate_id_,
					time_of_formation_,
					reconstruct_handle_),
			d_topology_reconstruct_geometry_time_span(topology_reconstruct_geometry_time_span)
		{  }

	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYRECONSTRUCTEDFEATUREGEOMETRY_H
