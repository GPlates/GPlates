/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDVERTEXSOURCEINFO_H
#define GPLATES_APP_LOGIC_RESOLVEDVERTEXSOURCEINFO_H

#include <utility> // std::make_pair
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include "boost/tuple/tuple_comparison.hpp"
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>

#include "ReconstructionFeatureProperties.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "VelocityDeltaTime.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"

#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Information, shared by vertices of a resolved geometry, that references the original
	 * reconstructed feature geometry.
	 *
	 * For example, a resolved topological plate boundary might reference a resolved topological line
	 * as one of its topological sections which in turn references topological sections that are
	 * reconstructed feature geometries. Only the source reconstructed feature geometries contain
	 * information that can be used to calculate velocities for example.
	 *
	 * These can be shared by multiple vertices (if came from same source reconstructed feature geometry)
	 * since this saves memory by avoiding duplication across all vertices.
	 */
	class ResolvedVertexSourceInfo :
			public boost::equality_comparable<ResolvedVertexSourceInfo>,
			public GPlatesUtils::ReferenceCount<ResolvedVertexSourceInfo>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedVertexSourceInfo> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedVertexSourceInfo> non_null_ptr_to_const_type;


		/**
		 * Create a source info from a reconstructed geometry/feature.
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructedFeatureGeometry::non_null_ptr_to_const_type &reconstruction_properties)
		{
			return non_null_ptr_type(new ResolvedVertexSourceInfo(reconstruction_properties));
		}

		/**
		 * Adapt a source info to calculate velocity at a fixed point.
		 *
		 * This is useful when rubber banding topological sections such that the velocity is always
		 * calculated at an end point of the section. Then two source infos (for two adjacent sections)
		 * are interpolated to a point midway between the ends of the two sections.
		 */
		static
		const non_null_ptr_type
		create(
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info,
				const GPlatesMaths::PointOnSphere &fixed_point)
		{
			return non_null_ptr_type(new ResolvedVertexSourceInfo(source_info, fixed_point));
		}

		/**
		 * Create an interpolation between two source infos.
		 *
		 * @a interpolate_ratio is in range [0, 1] where 0 represents @a source_info1 and 1 represents @a source_info2.
		 */
		static
		const non_null_ptr_type
		create(
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info1,
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info2,
				const double &interpolate_ratio)
		{
			return non_null_ptr_type(
					new ResolvedVertexSourceInfo(source_info1, source_info2, interpolate_ratio));
		}


		/**
		 * Get the stage rotation for the specified reconstruction time and velocity delta time.
		 *
		 * The result is cached in case the next vertex calls this method with the same parameters.
		 * It's likely that multiple vertices sharing us will all request the same stage rotation
		 * at the same time.
		 */
		GPlatesMaths::FiniteRotation
		get_stage_rotation(
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type) const;

		/**
		 * Calculates the velocity vector at the specified point location.
		 *
		 * The stage rotation(s) used to calculate velocity are cached in case the next vertex calls
		 * this method with the same parameters (except @a point which can differ).
		 * It's likely that multiple vertices sharing us will all request velocities using the same
		 * parameters at the same time.
		 */
		GPlatesMaths::Vector3D
		get_velocity_vector(
				const GPlatesMaths::PointOnSphere &point,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type) const;

		/**
		 * Equality operator - operator != provided by boost::equality_comparable.
		 */
		bool
		operator==(
				const ResolvedVertexSourceInfo &rhs) const;

	private:

		/**
		 * Geometry was reconstructed by plate ID.
		 */
		struct PlateIdProperties
		{
			PlateIdProperties(
					const ReconstructionTreeCreator &reconstruction_tree_creator_,
					GPlatesModel::integer_plate_id_type plate_id_) :
				reconstruction_tree_creator(reconstruction_tree_creator_),
				plate_id(plate_id_)
			{  }

			//! Rotation tree generator used to create/reconstruct the ReconstructedFeatureGeometry.
			ReconstructionTreeCreator reconstruction_tree_creator;
			GPlatesModel::integer_plate_id_type plate_id;
		};

		/**
		 * Geometry was reconstructed by half stage rotation.
		 */
		struct HalfStageRotationProperties
		{
			HalfStageRotationProperties(
					const ReconstructionTreeCreator &reconstruction_tree_creator_,
					ReconstructedFeatureGeometry::non_null_ptr_to_const_type reconstruction_properties_) :
				reconstruction_tree_creator(reconstruction_tree_creator_),
				reconstruction_properties(reconstruction_properties_)
			{  }

			const ReconstructionFeatureProperties &
			get_reconstruction_params() const
			{
				if (!reconstruction_params)
				{
					// Get the left/right plate IDs, etc.
					ReconstructionFeatureProperties reconstruction_feature_properties;
					reconstruction_feature_properties.visit_feature(reconstruction_properties->get_feature_ref());
					reconstruction_params = boost::in_place(reconstruction_feature_properties);
				}

				return reconstruction_params.get();
			}

			//! Rotation tree generator used to create/reconstruct the ReconstructedFeatureGeometry.
			ReconstructionTreeCreator reconstruction_tree_creator;
			//! The properties used to reconstruct are obtained from this reconstruction geometry.
			ReconstructedFeatureGeometry::non_null_ptr_to_const_type reconstruction_properties;
			//! Cached reconstruction parameters are calculated if/when needed.
			mutable boost::optional<ReconstructionFeatureProperties> reconstruction_params;
		};

		/**
		 * Adapter that fixes velocity calculations to a specific point.
		 */
		struct FixedPointVelocityAdapter
		{
			FixedPointVelocityAdapter(
					ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info_,
					const GPlatesMaths::PointOnSphere &fixed_point_) :
				source_info(source_info_),
				fixed_point(fixed_point_)
			{  }

			ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info;
			GPlatesMaths::PointOnSphere fixed_point;
		};

		/**
		 * Interpolating between two vertex source infos.
		 */
		struct InterpolateVertexSourceInfos
		{
			InterpolateVertexSourceInfos(
					ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info1_,
					ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info2_,
					const double &interpolate_ratio_) :
				source_info1(source_info1_),
				source_info2(source_info2_),
				interpolate_ratio(interpolate_ratio_)
			{  }

			ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info1;
			ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info2;
			double interpolate_ratio;
		};


		/**
		 * Vertex source is one of the above types.
		 */
		typedef boost::variant<
				PlateIdProperties,
				HalfStageRotationProperties,
				FixedPointVelocityAdapter,
				InterpolateVertexSourceInfos>
						source_type;


		/**
		 * Variant visitor to calculate stage rotation.
		 */
		struct CalcStageRotationVisitor :
				public boost::static_visitor<GPlatesMaths::FiniteRotation>
		{
			CalcStageRotationVisitor(
					const double &reconstruction_time_,
					const double &velocity_delta_time_,
					VelocityDeltaTime::Type velocity_delta_time_type_) :
				reconstruction_time(reconstruction_time_),
				velocity_delta_time(velocity_delta_time_),
				velocity_delta_time_type(velocity_delta_time_type_)
			{  }

			GPlatesMaths::FiniteRotation
			operator()(
					const PlateIdProperties &source) const;

			GPlatesMaths::FiniteRotation
			operator()(
					const HalfStageRotationProperties &source) const;

			GPlatesMaths::FiniteRotation
			operator()(
					const FixedPointVelocityAdapter &source) const;

			GPlatesMaths::FiniteRotation
			operator()(
					const InterpolateVertexSourceInfos &source) const;

			double reconstruction_time;
			double velocity_delta_time;
			VelocityDeltaTime::Type velocity_delta_time_type;
		};

		/**
		 * Variant visitor to calculate velocity vector.
		 */
		struct CalcVelocityVectorVisitor :
				public boost::static_visitor<GPlatesMaths::Vector3D>
		{
			CalcVelocityVectorVisitor(
					const ResolvedVertexSourceInfo &source_info_,
					const GPlatesMaths::PointOnSphere &point_,
					const double &reconstruction_time_,
					const double &velocity_delta_time_,
					VelocityDeltaTime::Type velocity_delta_time_type_) :
				source_info(source_info_),
				point(point_),
				reconstruction_time(reconstruction_time_),
				velocity_delta_time(velocity_delta_time_),
				velocity_delta_time_type(velocity_delta_time_type_)
			{  }

			/**
			 * When *not* interpolating, just calculate from the stage rotation.
			 */
			template <typename SourceType>
			GPlatesMaths::Vector3D
			operator()(
					const SourceType &source) const
			{
				// Calculate the velocity from the stage rotation.
				return GPlatesMaths::calculate_velocity_vector(
						point,
						source_info.get_stage_rotation(
								reconstruction_time,
								velocity_delta_time,
								velocity_delta_time_type),
						velocity_delta_time);
			}

			GPlatesMaths::Vector3D
			operator()(
					const FixedPointVelocityAdapter &source) const
			{
				return source.source_info->get_velocity_vector(
						// Use the fixed point instead of the caller's point...
						source.fixed_point,
						reconstruction_time,
						velocity_delta_time,
						velocity_delta_time_type);
			}

			/**
			 * When interpolating, avoid interpolating the stage rotations, instead interpolate the velocity vectors.
			 *
			 * It's appears to give the same results as interpolating the stage rotation and calculating velocity
			 * from that, but we'll interpolate velocities just to be sure.
			 */
			GPlatesMaths::Vector3D
			operator()(
					const InterpolateVertexSourceInfos &source) const
			{
				const GPlatesMaths::Vector3D velocity1 =
						source.source_info1->get_velocity_vector(
								point,
								reconstruction_time,
								velocity_delta_time,
								velocity_delta_time_type);
				const GPlatesMaths::Vector3D velocity2 =
						source.source_info2->get_velocity_vector(
								point,
								reconstruction_time,
								velocity_delta_time,
								velocity_delta_time_type);

				// Interpolate the velocity vectors from both sources.
				return (1.0 - source.interpolate_ratio) * velocity1 + source.interpolate_ratio * velocity2;
			}

			const ResolvedVertexSourceInfo &source_info;
			const GPlatesMaths::PointOnSphere &point;
			double reconstruction_time;
			double velocity_delta_time;
			VelocityDeltaTime::Type velocity_delta_time_type;
		};

		/**
		 * Variant visitor to compare equality.
		 */
		struct EqualityVisitor :
				public boost::static_visitor<bool>
		{
			template <typename LHSType, typename RHSType>
			bool
			operator()(
					const LHSType &,
					const RHSType &) const
			{
				return false; // Different types compare unequal.
			}

			bool
			operator()(
					const PlateIdProperties &lhs,
					const PlateIdProperties &rhs) const;

			bool
			operator()(
					const HalfStageRotationProperties &lhs,
					const HalfStageRotationProperties &rhs) const;

			bool
			operator()(
					const FixedPointVelocityAdapter &lhs,
					const FixedPointVelocityAdapter &rhs) const;

			bool
			operator()(
					const InterpolateVertexSourceInfos &lhs,
					const InterpolateVertexSourceInfos &rhs) const;
		};


		//
		// Cache the stage rotation for a specific reconstruction time and velocity delta time.
		// It's likely that multiple vertices sharing us will all request the same stage rotation
		// at the same time.
		//
		typedef boost::tuple<
				GPlatesMaths::Real/*reconstruction_time*/,
				GPlatesMaths::Real/*velocity_delta_time*/,
				VelocityDeltaTime::Type>
						stage_rotation_key_type;


		source_type d_source;

		/**
		 * Stage rotation key (input parameters) and value (stage rotation).
		 *
		 * Initially is none.
		 */
		mutable boost::optional<
				std::pair<
						stage_rotation_key_type,
						GPlatesMaths::FiniteRotation> > d_cached_stage_rotation;


		/**
		 * Create source info using reconstruction by plate ID or by half-stage rotation.
		 */
		explicit
		ResolvedVertexSourceInfo(
				const ReconstructedFeatureGeometry::non_null_ptr_to_const_type &reconstruction_properties) :
			d_source(create_source_from_reconstruction_properties(reconstruction_properties))
		{  }

		/**
		 * Adapt a source info to calculate velocity at a fixed point.
		 */
		ResolvedVertexSourceInfo(
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info,
				const GPlatesMaths::PointOnSphere &fixed_point) :
			d_source(FixedPointVelocityAdapter(source_info, fixed_point))
		{  }

		/**
		 * Create an interpolated source info (between two other source infos).
		 */
		ResolvedVertexSourceInfo(
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info1,
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type source_info2,
				const double &interpolate_ratio) :
			d_source(InterpolateVertexSourceInfos(source_info1, source_info2, interpolate_ratio))
		{  }


		/**
		 * Create source info using reconstruction by plate ID or by half-stage rotation.
		 */
		source_type
		create_source_from_reconstruction_properties(
				const ReconstructedFeatureGeometry::non_null_ptr_to_const_type &reconstruction_properties);

		/**
		 * Calculate the stage rotation for the specified reconstruction time and velocity delta time.
		 */
		GPlatesMaths::FiniteRotation
		calc_stage_rotation(
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type) const;
	};


	//! Typedef for a sequence of @a ResolvedVertexSourceInfo objects.
	typedef std::vector<ResolvedVertexSourceInfo::non_null_ptr_to_const_type> resolved_vertex_source_info_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDVERTEXSOURCEINFO_H
