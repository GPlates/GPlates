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
 
#ifndef GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H

#include <boost/optional.hpp>
#include <QString>

#include "VisualLayerParams.h"

#include "gui/Colour.h"
#include "gui/ColourPalette.h"
#include "gui/DrawStyleManager.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesPresentation
{
	class TopologyNetworkVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyNetworkVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyNetworkVisualLayerParams> non_null_ptr_to_const_type;


		enum TriangulationColourMode
		{
			TRIANGULATION_COLOUR_DRAW_STYLE,
			TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE,
			TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE,
			TRIANGULATION_COLOUR_STRAIN_RATE_STYLE
		};

		enum TriangulationDrawMode
		{
			TRIANGULATION_DRAW_NONE,
			TRIANGULATION_DRAW_MESH,
			TRIANGULATION_DRAW_FILL
		};


		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new TopologyNetworkVisualLayerParams( layer_params );
		}


		TriangulationColourMode
		get_triangulation_colour_mode() const
		{
			return d_triangulation_colour_mode;
		}

		void
		set_triangulation_colour_mode(
				TriangulationColourMode triangulation_colour_mode)
		{
			d_triangulation_colour_mode = triangulation_colour_mode;
			emit_modified();
		}


		TriangulationDrawMode
		get_triangulation_draw_mode() const
		{
			return d_triangulation_draw_mode;
		}

		void
		set_triangulation_draw_mode(
				TriangulationDrawMode triangulation_draw_mode)
		{
			d_triangulation_draw_mode = triangulation_draw_mode;
			emit_modified();
		}


		// Set min/max absolute dilatation strain rate (for colour blending).
		void
		set_min_abs_dilatation(
				const double &min_abs_dilatation);
		void
		set_max_abs_dilatation(
				const double &max_abs_dilatation);

		// Get min/max absolute dilatation strain rate (for colour blending).
		const double &
		get_min_abs_dilatation() const
		{
			return d_min_abs_dilatation;
		}
		const double &
		get_max_abs_dilatation() const
		{
			return d_max_abs_dilatation;
		}

		/**
		 * Returns the dilatation colour palette filename, if loaded from a file.
		 *
		 * Returns the empty string if auto-generated.
		 */
		const QString &
		get_dilatation_colour_palette_filename() const
		{
			return d_dilatation_colour_palette_filename;
		}

		/**
		 * Return the dilatation colour palette.
		 *
		 * Returns none if no colour palette has been set.
		 */
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type>
		get_dilatation_colour_palette() const
		{
			return d_dilatation_colour_palette;
		}

		/** 
		 * Set the dilatation palette.
		 */
		void
		set_dilatation_colour_palette(
				const QString &filename,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette);

		/**
		 * Use the default dilatation colour palette.
		*/
		void
		use_default_dilatation_colour_palette();


		// Set min/max absolute second invariant strain rate (for colour blending).
		void
		set_min_abs_second_invariant(
				const double &min_abs_second_invariant);
		void
		set_max_abs_second_invariant(
				const double &max_abs_second_invariant);

		// Get min/max absolute second invariant strain rate (for colour blending).
		const double &
		get_min_abs_second_invariant() const
		{
			return d_min_abs_second_invariant;
		}
		const double &
		get_max_abs_second_invariant() const
		{
			return d_max_abs_second_invariant;
		}

		/**
		 * Returns the second invariant colour palette filename, if loaded from a file.
		 *
		 * Returns the empty string if auto-generated.
		 */
		const QString &
		get_second_invariant_colour_palette_filename() const
		{
			return d_second_invariant_colour_palette_filename;
		}

		/**
		 * Return the second invariant colour palette.
		 *
		 * Returns none if no colour palette has been set.
		 */
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type>
		get_second_invariant_colour_palette() const
		{
			return d_second_invariant_colour_palette;
		}

		/** 
		 * Set the second invariant palette.
		 */
		void
		set_second_invariant_colour_palette(
				const QString &filename,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette);

		/**
		 * Use the default second invariant colour palette.
		*/
		void
		use_default_second_invariant_colour_palette();


		// Set min/max strain rate style (for colour blending).
		void
		set_min_strain_rate_style(
				const double &min_strain_rate_style);
		void
		set_max_strain_rate_style(
				const double &max_strain_rate_style);

		// Get min/max strain rate style (for colour blending).
		const double &
		get_min_strain_rate_style() const
		{
			return d_min_strain_rate_style;
		}
		const double &
		get_max_strain_rate_style() const
		{
			return d_max_strain_rate_style;
		}

		/**
		 * Returns the strain rate style colour palette filename, if loaded from a file.
		 *
		 * Returns the empty string if auto-generated.
		 */
		const QString &
		get_strain_rate_style_colour_palette_filename() const
		{
			return d_strain_rate_style_colour_palette_filename;
		}

		/**
		 * Return the strain rate style colour palette.
		 *
		 * Returns none if no colour palette has been set.
		 */
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type>
		get_strain_rate_style_colour_palette() const
		{
			return d_strain_rate_style_colour_palette;
		}

		/** 
		 * Set the strain rate style palette.
		 */
		void
		set_strain_rate_style_colour_palette(
				const QString &filename,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette);

		/**
		 * Use the default strain rate style colour palette.
		*/
		void
		use_default_strain_rate_style_colour_palette();


		bool
		show_segment_velocity() const
		{
			return d_show_segment_velocity;
		}

		void
		set_show_segment_velocity(
				bool b)
		{
			d_show_segment_velocity = b;
			emit_modified();
		}


		bool
		get_fill_triangulation() const
		{
			return d_triangulation_draw_mode == TRIANGULATION_DRAW_FILL;
		}

		void
		set_fill_triangulation(
				bool b)
		{
			d_triangulation_draw_mode = b ? TRIANGULATION_DRAW_FILL : TRIANGULATION_DRAW_NONE;
			emit_modified();
		}


		bool
		get_fill_rigid_blocks() const
		{
			return d_fill_rigid_blocks;
		}

		void
		set_fill_rigid_blocks(
				bool b)
		{
			d_fill_rigid_blocks = b;
			emit_modified();
		}


		/**
		 * Sets the opacity of filled triangulation and rigid blocks.
		 */
		void
		set_fill_opacity(
				const double &opacity)
		{
			d_fill_opacity = opacity;
			emit_modified();
		}

		/**
		 * Gets the opacity of filled triangulation and rigid blocks.
		 */
		double
		get_fill_opacity() const
		{
			return d_fill_opacity;
		}


		/**
		 * Sets the intensity of filled triangulation and rigid blocks.
		 */
		void
		set_fill_intensity(
				const double &intensity)
		{
			d_fill_intensity = intensity;
			emit_modified();
		}

		/**
		 * Gets the intensity of filled triangulation and rigid blocks.
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
		get_fill_modulate_colour() const
		{
			return GPlatesGui::Colour(d_fill_intensity, d_fill_intensity, d_fill_intensity, d_fill_opacity);
		}


		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);

		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

	protected:

		explicit
		TopologyNetworkVisualLayerParams( 
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params);

	private:

		TriangulationColourMode d_triangulation_colour_mode;
		TriangulationDrawMode d_triangulation_draw_mode;

		//! Dilatation strain rate parameters.
		double d_min_abs_dilatation;
		double d_max_abs_dilatation;
		//! The dilatation colour palette filename (or empty if using default palette).
		QString d_dilatation_colour_palette_filename;
		//! The dilatation colour palette, whether set explicitly as loaded from a file, or auto-generated.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> d_dilatation_colour_palette;

		//! Second invariant strain rate parameters.
		double d_min_abs_second_invariant;
		double d_max_abs_second_invariant;
		//! The second invariant colour palette filename (or empty if using default palette).
		QString d_second_invariant_colour_palette_filename;
		//! The second invariant colour palette, whether set explicitly as loaded from a file, or auto-generated.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> d_second_invariant_colour_palette;

		//! Strain rate style parameters.
		double d_min_strain_rate_style;
		double d_max_strain_rate_style;
		//! The strain rate style colour palette filename (or empty if using default palette).
		QString d_strain_rate_style_colour_palette_filename;
		//! The strain rate style colour palette, whether set explicitly as loaded from a file, or auto-generated.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> d_strain_rate_style_colour_palette;

		// The various options to show or hide.
		bool d_show_segment_velocity;
		bool d_fill_rigid_blocks;

		//! The opacity of the filled triangulation and rigid blocks in the range [0,1].
		double d_fill_opacity;
		//! The intensity of the filled triangulation and rigid blocks in the range [0,1].
		double d_fill_intensity;


		void
		create_default_dilatation_colour_palette();

		void
		create_default_second_invariant_colour_palette();

		void
		create_default_strain_rate_style_colour_palette();
	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TopologyNetworkVisualLayerParams::TriangulationColourMode &triangulation_colour_mode,
			bool transcribed_construct_data);

	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			TopologyNetworkVisualLayerParams::TriangulationDrawMode &triangulation_draw_mode,
			bool transcribed_construct_data);
}

#endif // GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H
