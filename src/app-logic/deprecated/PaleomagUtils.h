/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_PALEOMAGUTILS_H
#define GPLATES_APP_LOGIC_PALEOMAGUTILS_H

#include "gui/ColourProxy.h"
#include "maths/Rotation.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "view-operations/RenderedGeometryCollection.h"

const double SITE_POINT_SIZE = 4.;
const double POLE_POINT_SIZE = 4.;


namespace GPlatesPresentation
{
	class ViewState;
}


namespace GPlatesAppLogic
{
	class Reconstruction;

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
			bool
			initialise_pre_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle);

		private:
			bool d_found_paleomag_features;
		};
		
	
		/**
		 * Returns true if there are any paleomag features in @a feature_collection.
		 */
		bool
		detect_paleomag_features(
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection);
			
	
		class VgpRenderer:
			public GPlatesModel::FeatureVisitor
		{
		public:
			
			VgpRenderer(
				Reconstruction &reconstruction,
				boost::optional<GPlatesMaths::Rotation> &additional_rotation,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type target_layer,
				const GPlatesGui::ColourProxy &colour,
				GPlatesPresentation::ViewState *view_state_,
				bool should_add_to_reconstruction = false
				):
				d_reconstruction(reconstruction),
				d_additional_rotation(additional_rotation),
				d_target_layer(target_layer),
				d_colour(colour),
				d_view_state_ptr(view_state_),
				d_should_add_to_reconstruction(should_add_to_reconstruction)
				
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
		
			Reconstruction &d_reconstruction;
			
			/**
			 * A rotation applied to the Vgp geometries before rendering.
			 * 
			 * If the VgpRenderer is used from the PoleManipulation tool, we need to 
			 * perform this additional rotation for rendering the "dragged" geometries.
			 */
			boost::optional<GPlatesMaths::Rotation> d_additional_rotation;
			
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_target_layer;
			const GPlatesGui::ColourProxy &d_colour;
			GPlatesPresentation::ViewState *d_view_state_ptr;
			
			/**
			 * Whether or not the reconstructed Vgp geometries should be added to the
			 * set of reconstruction geometries.
			 *
			 * If the VgpRenderer is used in the normal Pmag workflow, we want to add the 
			 * geometries; if it's used from the manipulate pole tool, for example, we don't
			 * necessarily want to add them.
			 */
			bool d_should_add_to_reconstruction;

			
			boost::optional<GPlatesMaths::PointOnSphere> d_site_point;
			boost::optional<GPlatesModel::FeatureHandle::iterator> d_site_iterator;
			boost::optional<GPlatesMaths::PointOnSphere> d_vgp_point;
			boost::optional<GPlatesModel::FeatureHandle::iterator> d_vgp_iterator;
			boost::optional<double> d_a95;
			boost::optional<double> d_dm;
			boost::optional<double> d_dp;
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_begin_time;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_end_time;
			boost::optional<double> d_age;
		};	
	
	}
}

#endif //GPLATES_APP_LOGIC_PALEOMAGUTILS_H
