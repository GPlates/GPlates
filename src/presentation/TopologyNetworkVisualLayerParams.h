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


namespace GPlatesPresentation
{
	class TopologyNetworkVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyNetworkVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyNetworkVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new TopologyNetworkVisualLayerParams( layer_params );
		}

		bool
		show_delaunay_triangulation() const
		{
			return d_show_delaunay_triangulation;
		}

		void
		set_show_delaunay_triangulation(
				bool b)
		{
			d_show_delaunay_triangulation = b;
			emit_modified();
		}

		bool
		show_constrained_triangulation() const
		{
			return d_show_constrained_triangulation;
		}

		void
		set_show_constrained_triangulation(
				bool b)
		{
			d_show_constrained_triangulation = b;
			emit_modified();
		}

		bool
		show_mesh_triangulation() const
		{
			return d_show_mesh_triangulation;
		}

		void
		set_show_mesh_triangulation(
				bool b)
		{
			d_show_mesh_triangulation = b;
			emit_modified();
		}

		bool
		show_total_triangulation() const
		{
			return d_show_total_triangulation;
		}

		void
		set_show_total_triangulation(
				bool b)
		{
			d_show_total_triangulation = b;
			emit_modified();
		}

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
		show_fill() const
		{
			return d_show_fill;
		}

		void
		set_show_fill(
				bool b)
		{
			d_show_fill = b;
			emit_modified();
		}

		int
		color_index() const
		{
			return d_color_index;
		}

		void
		set_color_index(
				int i)
		{
			d_color_index = i;
			emit_modified();
		}

		//
		// FIXME: Should be calling 'emit_modified()' in the 'set' calls below, but cannot currently
		// because 'TopologyNetworkResolverLayerOptionsWidget::handle_update_button_clicked()' updates
		// multiple GUI controls (which results in multiple 'set' calls here which in turn modify
		// the GUI before all its controls have been updated).
		//
		// The fix is for 'TopologyNetworkResolverLayerOptionsWidget' to do an update immediately
		// after each GUI control is changed (rather than having an 'update' button that updates after
		// multiple GUI controls have been changed).
		//

		// set/get data for range1
		void set_range1_max(double max) { d_range1_max = max; }
		void set_range1_min(double min) { d_range1_min = min; }

		double get_range1_max() const { return d_range1_max; }
		double get_range1_min() const { return d_range1_min; }

		// set/get data for range2
		void set_range2_max(double max) { d_range2_max = max; }
		void set_range2_min(double min) { d_range2_min = min; }

		double get_range2_max() const { return d_range2_max; }
		double get_range2_min() const { return d_range2_min; }

		// Set Colours 
		void set_fg_colour(GPlatesGui::Colour c) { d_fg_colour = c; }
		void set_max_colour(GPlatesGui::Colour c) { d_max_colour = c; }
		void set_mid_colour(GPlatesGui::Colour c) { d_mid_colour = c; }
		void set_min_colour(GPlatesGui::Colour c) { d_min_colour = c; }
		void set_bg_colour(GPlatesGui::Colour c) { d_bg_colour = c; }

		// Get Colours 
		GPlatesGui::Colour get_fg_colour() const { return d_fg_colour; }
		GPlatesGui::Colour get_max_colour() const { return d_max_colour; }
		GPlatesGui::Colour get_mid_colour() const { return d_mid_colour; }
		GPlatesGui::Colour get_min_colour() const { return d_min_colour; }
		GPlatesGui::Colour get_bg_colour() const { return d_bg_colour; }

		// Override of virtual metions in  VisualLayerParams base.
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

		// Override of virtual metions in  VisualLayerParams base.
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);

		/**
		 * Returns the filename of the file from which the current colour
		 * palette was loaded, if it was loaded from a file.
		 * If the current colour palette is auto-generated, returns the empty string.
		 */
		const QString &
		get_colour_palette_filename() const
		{
			return d_colour_palette_filename;
		}

		/** 
		 * Set the palette 
		 */
		void
		set_colour_palette(
				const QString &filename,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette);

		/**
		 * Causes the current colour palette to be generated from the gui controls
		*/
		void
		user_generated_colour_palette();
		
		/**
		 * Return the current colour palette.
		 *
		 * Returns none if no colour palette has been set.
		 */
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type>
		get_colour_palette() const
		{
			return d_colour_palette;
		}

	protected:

		explicit
		TopologyNetworkVisualLayerParams( 
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params);

	private:

		// The various options to show or hide the triangulations 
		bool d_show_delaunay_triangulation;
		bool d_show_constrained_triangulation;
		bool d_show_mesh_triangulation;
		bool d_show_total_triangulation;
		bool d_show_fill;
		bool d_show_segment_velocity;
		int d_color_index;

		//! parms for creating colour palettes
		double d_range1_max;
		double d_range1_min;
		double d_range2_max;
		double d_range2_min;
		GPlatesGui::Colour d_fg_colour;
		GPlatesGui::Colour d_max_colour;
		GPlatesGui::Colour d_mid_colour;
		GPlatesGui::Colour d_min_colour;
		GPlatesGui::Colour d_bg_colour;

		/**
		 * The current colour palette for this layer
		 */
		QString d_colour_palette_filename;

		/**
		 * The current colour palette for this layer, whether set explicitly as
		 * loaded from a file, or auto-generated.
		 */
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> d_colour_palette;
	};
}

#endif // GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H
