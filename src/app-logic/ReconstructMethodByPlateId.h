/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructMethodInterface.h"
#include "TopologyReconstruct.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	/**
	 * Reconstructs a feature using its present day geometry and its plate Id.
	 */
	class ReconstructMethodByPlateId :
			public ReconstructMethodInterface
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodByPlateId.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodByPlateId> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodByPlateId> non_null_ptr_to_const_type;


		/**
		 * Returns true if can reconstruct the specified feature.
		 *
		 * It only needs to have a non-topological geometry to pass this test.
		 */
		static
		bool
		can_reconstruct_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref);


		/**
		 * Creates a @a ReconstructMethodByPlateId object associated with the specified feature.
		 */
		static
		ReconstructMethodByPlateId::non_null_ptr_type
		create(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context)
		{
			return non_null_ptr_type(new ReconstructMethodByPlateId(feature_ref, context));
		}


		/**
		 * Returns the present day geometries of the feature associated with this reconstruct method.
		 */
		virtual
		void
		get_present_day_feature_geometries(
				std::vector<Geometry> &present_day_geometries) const;


		/**
		 * Reconstructs the feature associated with this reconstruct method to the specified
		 * reconstruction time and returns one or more reconstructed feature geometries.
		 *
		 * NOTE: This will still generate a reconstructed feature geometry if the
		 * feature has no plate ID (ie, even if @a can_reconstruct_feature returns false).
		 * In this case the identity rotation is used.
		 */
		virtual
		void
		reconstruct_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time);


		/**
		 * Calculates velocities at the positions of the reconstructed feature geometries, of the feature
		 * associated with this reconstruct method, at the specified reconstruction time and returns
		 * one or more reconstructed feature *velocities*.
		 */
		virtual
		void
		reconstruct_feature_velocities(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type);


		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * NOTE: The feature associated with this reconstruct method is used as a source of
		 * feature properties that determine how to perform the reconstruction (for example,
		 * a reconstruction plate ID) - the feature's geometries are not reconstructed.
		 */
		virtual
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const Context &context,
				const double &reconstruction_time,
				bool reverse_reconstruct);


		/**
		 * Returns any topology-reconstructed geometry time spans.
		 *
		 * These are only used when features are reconstructed using *topologies*.
		 * They store the results of incrementally reconstructing using resolved topological plates/networks.
		 */
		virtual
		void
		get_topology_reconstructed_geometry_time_spans(
				topology_reconstructed_geometry_time_span_sequence_type &topology_reconstructed_geometry_time_spans,
				const Context &context);

	private:

		/**
		 * Feature property information used for reconstructing.
		 */
		struct ReconstructionInfo
		{
			ReconstructionInfo() :
				// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)...
				reconstruction_plate_id(0),
				// Default to present day if there's no geometry import time property...
				geometry_import_time(0.0)
			{  }

			GPlatesModel::integer_plate_id_type reconstruction_plate_id;
			ReconstructionFeatureProperties::TimePeriod valid_time;
			double geometry_import_time;
		};

		//! Cache the present day geometries so we don't need to gather them each time they're reconstructed.
		mutable boost::optional< std::vector<Geometry> > d_present_day_geometries;

		//! Cache the reconstruction information so can re-use it for each reconstruction.
		mutable boost::optional<ReconstructionInfo> d_reconstruction_info;

		/**
		 * The topology reconstructed geometry look up tables, or boost::none if not reconstructing using topologies.
		 *
		 * There's one entry for each feature geometry property.
		 */
		mutable boost::optional<topology_reconstructed_geometry_time_span_sequence_type> d_topology_reconstructed_geometry_time_spans;


		explicit
		ReconstructMethodByPlateId(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context);

		const ReconstructionInfo &
		get_reconstruction_info(
				const Context &context) const;

		boost::optional<const topology_reconstructed_geometry_time_span_sequence_type &>
		get_topology_reconstruction_info(
				const Context &context) const;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H
