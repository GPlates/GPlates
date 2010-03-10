/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_PALEOMAGUTILS_H
#define GPLATES_APP_LOGIC_PALEOMAGUTILS_H

#include "gui/ColourProxy.h"
#include "maths/Rotation.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureHandle.h"

#include "property-values/GmlPoint.h"
#include "property-values/GeoTimeInstant.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesModel
{
	class Reconstruction;
}


namespace GPlatesAppLogic
{
	namespace PaleomagUtils
	{
		
		
		/**
		* Determines if there are any paleomag features in the collection.
		*/
		class DetectPaleomagFeatures:
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetectPaleomagFeatures() :
			  d_found_paleomag_features(false)
			{  }


			bool
			has_paleomag_features() const
			{
				return d_found_paleomag_features;
			}


			virtual
			void
			visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle)
			{
				if (d_found_paleomag_features)
				{
					// We've already found a paleomag feature so just return.
					return;
				}

				static const GPlatesModel::FeatureType paleomag_feature_type = 
					GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

				if (feature_handle.feature_type() == paleomag_feature_type)
				{
					d_found_paleomag_features = true;
				}

				// NOTE: We don't actually want to visit the feature's properties
				// so we're not calling 'visit_feature_properties()'.
			}

		private:
			bool d_found_paleomag_features;
		};
		
	
		bool
		detect_paleomag_features(
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection);
			
	
		class VgpRenderer:
			public GPlatesModel::FeatureVisitor
		{
		public:
			
			VgpRenderer(
				GPlatesModel::Reconstruction &reconstruction,
				boost::optional<const double> &reconstruction_time,
				boost::optional<GPlatesMaths::Rotation> &additional_rotation,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type target_layer,
				const GPlatesGui::ColourProxy &colour,
				bool draw_error_as_ellipse = true
				):
				d_reconstruction(reconstruction),
				d_reconstruction_time(reconstruction_time),
				d_additional_rotation(additional_rotation),
				d_target_layer(target_layer),
				d_colour(colour),
				d_draw_error_as_ellipse(draw_error_as_ellipse)
			{ };
			
			virtual
			void
			finalise_post_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);
			
				
			virtual
			void
			visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);	
				
				
			virtual
			void
			visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);
				
			virtual
			void
			visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);
				
			virtual
			void
			visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);
				
			virtual
			void
			visit_xs_double(
				GPlatesPropertyValues::XsDouble &xs_double);
				
		private:
		
			GPlatesModel::Reconstruction &d_reconstruction;
			boost::optional<const double> d_reconstruction_time;
			boost::optional<GPlatesMaths::Rotation> d_additional_rotation;
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_target_layer;
			const GPlatesGui::ColourProxy &d_colour;
			const bool d_draw_error_as_ellipse;
			
			boost::optional<GPlatesMaths::PointOnSphere> d_site_point;
			boost::optional<GPlatesMaths::PointOnSphere> d_vgp_point;
			boost::optional<double> d_a95;
			boost::optional<double> d_dm;
			boost::optional<double> d_dp;
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_begin_time;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_end_time;
				
		};	
	
	}
}

#endif //GPLATES_APP_LOGIC_PALEOMAGUTILS_H
