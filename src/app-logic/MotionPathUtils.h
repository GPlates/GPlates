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

#ifndef GPLATES_APPLOGIC_MOTIONPATHUTILS_H
#define GPLATES_APPLOGIC_MOTIONPATHUTILS_H

#include "app-logic/ReconstructionGeometryCollection.h"
#include "app-logic/ReconstructionTree.h"
#include "maths/PolylineOnSphere.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometryCollection;
	class ReconstructionTree;

	namespace MotionPathUtils
	{

		/**
		* Used to obtain motion-track-relevant parameters from a motion track feature.
		*/
		class MotionPathPropertyFinder :
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			MotionPathPropertyFinder(
			    const double &reconstruction_time):
			    d_feature_is_defined_at_recon_time(true),
			    d_has_geometry(false),
			    d_reconstruction_time(
				GPlatesPropertyValues::GeoTimeInstant(reconstruction_time))
			{  }

			MotionPathPropertyFinder():
				d_feature_is_defined_at_recon_time(true),
				d_has_geometry(false)
			{  }

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_reconstruction_plate_id() const
			{
				return d_reconstruction_plate_id;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_relative_plate_id() const
			{
				return d_relative_plate_id;
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

			bool
			can_process_motion_path();

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
			visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);


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

			bool d_feature_is_defined_at_recon_time;
			bool d_has_geometry;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_reconstruction_time;
			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> d_relative_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_dissappearance;
			QString d_feature_info;
			QString d_name;

			// The GpmlArray<TimePeriod> times converted into a vector of doubles. 
			std::vector<double> d_times;

		};


		void
		calculate_motion_track(
			const GPlatesMaths::PointOnSphere &point,
			const MotionPathPropertyFinder	 &motion_track_parameters,
			std::vector<GPlatesMaths::PointOnSphere> &motion_track,
			const std::vector<GPlatesMaths::FiniteRotation> &rotations);

	}

}

#endif  // GPLATES_APPLOGIC_MOTIONPATHUTILS_H
