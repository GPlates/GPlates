/* $Id$ */

/**
 * @file 
 * Most recent change:
 *   $Date$
 * 
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

#ifndef GPLATES_GUI_RASTERCOLOURSCHEMEMAP_H
#define GPLATES_GUI_RASTERCOLOURSCHEMEMAP_H

#include <map>
#include <QObject>
#include <QString>

#include "RasterColourScheme.h"

#include "app-logic/Layer.h"


namespace GPlatesAppLogic
{
	class ReconstructGraph;
}


namespace GPlatesGui
{
	/**
	 * RasterColourSchemeMap stores a mapping of Layers to RasterColourSchemes,
	 * which in turn store which band the user is interested in and the colour
	 * palette to be applied to that band.
	 */
	class RasterColourSchemeMap :
			public QObject
	{
		Q_OBJECT

	private:

		struct LayerInfo
		{
			LayerInfo(
					const RasterColourScheme::non_null_ptr_type &colour_scheme_,
					const QString &palette_file_name_) :
				colour_scheme(colour_scheme_),
				palette_file_name(palette_file_name_)
			{  }

			RasterColourScheme::non_null_ptr_type colour_scheme;
			QString palette_file_name;
		};

	public:

		typedef LayerInfo layer_info_type;

		RasterColourSchemeMap(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph);

		/**
		 * Sets the colour scheme for a particular @a layer.
		 */
		void
		set_colour_scheme(
				const GPlatesAppLogic::Layer &layer,
				const RasterColourScheme::non_null_ptr_type &colour_scheme,
				const QString &palette_file_name);

		/**
		 * Returns the colour scheme for the particular @a layer.
		 *
		 * Returns boost::none if the layer is not in the map.
		 */
		boost::optional<RasterColourScheme::non_null_ptr_type>
		get_colour_scheme(
				const GPlatesAppLogic::Layer &layer);

		/**
		 * Returns the colour scheme and palette file name for the particular @a layer.
		 *
		 * Returns boost::none if the layer is not in the map.
		 */
		boost::optional<layer_info_type>
		get_layer_info(
				const GPlatesAppLogic::Layer &layer);

	private slots:

		void
		handle_layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

	private:

		typedef std::map<GPlatesAppLogic::Layer, LayerInfo> map_type;

		map_type d_map;
	};
}

#endif  /* GPLATES_GUI_RASTERCOLOURSCHEMEMAP_H */
