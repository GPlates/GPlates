/* $Id: FlowlineUtils.h 8209 2010-04-27 14:24:11Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-04-27 16:24:11 +0200 (ti, 27 apr 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway.
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

#ifndef GPLATES_APPLOGIC_FLOWLINEUTILS_H
#define GPLATES_APPLOGIC_FLOWLINEUTILS_H

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesAppLogic
{
	namespace FlowlineUtils
	{
		/**
		* Determines if there are any flowline features in the collection.
		*/
		class DetectFlowlineFeatures:
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetectFlowlineFeatures() :
			  d_found_flowline_features(false)
			  {  }


			  bool
				  has_flowline_features() const
			  {
				  return d_found_flowline_features;
			  }


				virtual
				bool
				initialise_pre_feature_properties(
						feature_handle_type &feature_handle)
				{
					if (d_found_flowline_features)
					{
						// We've already found a flowline feature so just return.
						// NOTE: We don't actually want to visit the feature's properties.
						return false;
					}

					static const GPlatesModel::FeatureType flowline_feature_type =
					GPlatesModel::FeatureType::create_gpml("Flowline");

					if (feature_handle.feature_type() == flowline_feature_type)
					{
						d_found_flowline_features = true;
					}

					// NOTE: We don't actually want to visit the feature's properties.
					return false;
				}

		private:
			bool d_found_flowline_features;
		};

		/**
		 * Used to obtain flowline-relevant parameters from a flowline feature.
		 */
		class FlowlinePropertyFinder :
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			FlowlinePropertyFinder(
				const double &reconstruction_time):
				d_feature_is_defined_at_recon_time(true),
				d_has_geometry(false),
				d_reconstruction_time(
				    GPlatesPropertyValues::GeoTimeInstant(reconstruction_time))
			{  }

			FlowlinePropertyFinder():
				d_feature_is_defined_at_recon_time(true),
				d_has_geometry(false)
			{  }

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_reconstruction_plate_id() const
			{
				return d_reconstruction_plate_id;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_left_plate() const
			{
				return d_left_plate;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_right_plate() const
			{
				return d_right_plate;
			}

			const std::vector<double> &
			get_times()
			{
				return d_times;
			}

			QString
			get_feature_info_string()
			{
				return d_feature_info;
			}

			QString
			get_name() 
			{
				return d_name;
			}
				
			bool
			has_geometry()
			{
				return d_has_geometry;
			}

			/**
			 * Whether or not we should calculate flowlines for the
			 * current time.
			 */
			bool
			can_process_flowline();

			/**
			 * Whether or not we should display the seed point for the
			 * current time.
			 */
			bool
			can_process_seed_point();

			/**
			 * Whether or not we have enough info in the feature to
			 * perform a seed-point correction.
			 */
			bool
			can_correct_seed_point();

			/**
			 * Returns optional time of appearance if a "gml:validTime" property is found.
			 */
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_appearance() const
			{
				return d_time_of_appearance;
			}


			/**
			 * Returns optional time of dissappearance if a "gml:validTime" property is found.
			 */
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
			get_time_of_dissappearance() const
			{
				return d_time_of_appearance;
			}

		private:
			virtual
			bool
			initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

			virtual
			void
			finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

			virtual
			void
			visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_has_geometry = true;
			}


			virtual
			void
			visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_has_geometry = true;
			}

			virtual
			void
			visit_gpml_array(
				const GPlatesPropertyValues::GpmlArray &gpml_array);

			virtual
			void
			visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

			virtual
			void
			visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

			virtual
			void
			visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

			virtual
			void
			visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);


			bool d_feature_is_defined_at_recon_time;
			bool d_has_geometry;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_reconstruction_time;
			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_left_plate;
			boost::optional<GPlatesModel::integer_plate_id_type> d_right_plate;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_dissappearance;
			QString d_feature_info;
			QString d_name;

			// The GpmlArray<TimePeriod> times converted into a vector of doubles. 
			std::vector<double> d_times;

		};

		void
		calculate_flowline(
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &seed_point,
			const FlowlinePropertyFinder &flowline_parameters,
			std::vector<GPlatesMaths::PointOnSphere> &flowline,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations);

		/**
		 * Halves the angle of the provided FiniteRotation.
		 */
		void
		get_half_angle_rotation(
			GPlatesMaths::FiniteRotation &rotation);

		void
		fill_times_vector(
			std::vector<double> &times,
			const double &reconstruction_time,
			const std::vector<double> &time_samples);		

		void
		get_times_from_time_period_array(
			std::vector<double> &times,
			const GPlatesPropertyValues::GpmlArray &array);

		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		reconstruct_seed_point(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations,
			bool reverse = false);

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_seed_points(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations,
			bool reverse = false);

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_flowline_seed_points(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
			const double &current_time,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			const GPlatesModel::FeatureHandle::weak_ref &feature_handle,
			bool reverse = false);

		/**
		 * Fills @a seed_point_rotations with half-stage rotations from earliest flowline
		 * time to current time.
		 */
		void
		fill_seed_point_rotations(
			const double &current_time,
			const std::vector<double> &flowline_times,
			const GPlatesModel::integer_plate_id_type &left_plate_id,
			const GPlatesModel::integer_plate_id_type &right_plate_id,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			std::vector<GPlatesMaths::FiniteRotation> &seed_point_rotations);

		/**
		 * Given a flowline end point(s) @a geometry_ at time @a reconstruction_time,
		 * calculates the spreading centre for that flowline.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		correct_end_point_to_centre(
		    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_,
		    const GPlatesModel::integer_plate_id_type &plate_1,
		    const GPlatesModel::integer_plate_id_type &plate_2,
		    const std::vector<double> &times,
			const ReconstructionTreeCreator &reconstruction_tree_creator,
		    const double &reconstruction_time);

	}

}


#endif // GPLATES_APPLOGIC_FLOWLINEUTILS_H

