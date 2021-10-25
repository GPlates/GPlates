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
 
#ifndef GPLATES_PRESENTATION_RECONSTRUCTVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_RECONSTRUCTVISUALLAYERPARAMS_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "VisualLayerParams.h"

#include "gui/Colour.h"

#include "maths/types.h"

#include "property-values/GeoTimeInstant.h"

 // Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesPresentation
{
	class ReconstructVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructVisualLayerParams> non_null_ptr_to_const_type;


		enum VGPVisibilitySetting
		{
			ALWAYS_VISIBLE, /**< all vgps are displayed at all times */
			TIME_WINDOW, /**< all vgps are displayed between a specified time interval */
			DELTA_T_AROUND_AGE /**< vgps are displayed if the reconstruction time is within a time window around the VGP's age */

							   // NOTE: Any new values should also be added to @a transcribe.
		};


		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new ReconstructVisualLayerParams(layer_params);
		}


		VGPVisibilitySetting
		get_vgp_visibility_setting() const;

		void
		set_vgp_visibility_setting(
				VGPVisibilitySetting setting);

		const GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_earliest_time() const;

		void
		set_vgp_earliest_time(
				const GPlatesPropertyValues::GeoTimeInstant &earliest_time);

		const GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_latest_time() const;

		void
		set_vgp_latest_time(
				const GPlatesPropertyValues::GeoTimeInstant &latest_time);

		double
		get_vgp_delta_t() const;

		void
		set_vgp_delta_t(
				double vgp_delta_t);

		bool
		get_vgp_draw_circular_error() const;

		void
		set_vgp_draw_circular_error(
				bool draw);

		bool
		show_vgp(
				double current_time,
				const boost::optional<double> &age) const;

		void
		set_fill_polygons(
				bool fill);

		bool
		get_fill_polygons() const;

		void
		set_fill_polylines(
				bool fill);

		bool
		get_fill_polylines() const;

		/**
		 * Sets the opacity of filled primitives.
		 */
		void
		set_fill_opacity(
				const double &opacity);

		/**
		 * Gets the opacity of filled primitives.
		 */
		double
		get_fill_opacity() const
		{
			return d_fill_opacity;
		}

		/**
		 * Sets the intensity of filled primitives.
		 */
		void
		set_fill_intensity(
				const double &intensity);

		/**
		 * Gets the intensity of filled primitives.
		 */
		double
		get_fill_intensity() const
		{
			return d_fill_intensity;
		}

		/**
		 * Returns the filled primitives modulate colour.
		 *
		 * This is a combination of the opacity and intensity as (I, I, I, O) where
		 * 'I' is intensity and 'O' is opacity.
		 */
		GPlatesGui::Colour
		get_fill_modulate_colour() const;


		/**
		 * Whether to show topology-reconstructed feature geometries.
		 */
		void
		set_show_topology_reconstructed_feature_geometries(
				bool show_topology_reconstructed_feature_geometries);

		bool 
		get_show_topology_reconstructed_feature_geometries() const;


		/**
		 * Whether to show strain accumulation at the points of deformed feature geometries.
		 */
		void
		set_show_strain_accumulation(
				bool show_strain_accumulation);

		bool 
		get_show_strain_accumulation() const;


		void
		set_strain_accumulation_scale(
				const double &strain_accumulation_scale);

		double
		get_strain_accumulation_scale() const;


		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruct_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_visual_layer_params(*this);
		}

	protected:

		explicit
		ReconstructVisualLayerParams(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params);

	private:

		static const double INITIAL_VGP_DELTA_T;

		/**
		 * Enum indicating what sort of VGP visibility we have.                                                                
		 */
		VGPVisibilitySetting d_vgp_visibility_setting;

		/**
		 * Begin time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_earliest_time;

		/**
		 * End time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_latest_time;

		/**
		 * Delta used for time window around VGP age.                                                                     
		 */
		GPlatesMaths::real_t d_vgp_delta_t;

		bool d_vgp_draw_circular_error;

		bool d_fill_polygons;
		bool d_fill_polylines;

		//! The opacity of filled primitives in the range [0,1].
		double d_fill_opacity;
		//! The intensity of filled primitives in the range [0,1].
		double d_fill_intensity;

		bool d_show_topology_reconstructed_feature_geometries;
		bool d_show_show_strain_accumulation;
		double d_strain_accumulation_scale;
	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			ReconstructVisualLayerParams::VGPVisibilitySetting &vgp_visibility_setting,
			bool transcribed_construct_data);
}

#endif // GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
