/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEW_OPERATIONS_RENDERRECONSTRUCTIONGEOMETRIES_H
#define GPLATES_VIEW_OPERATIONS_RENDERRECONSTRUCTIONGEOMETRIES_H

#include "app-logic/ReconstructHook.h"


namespace GPlatesGui
{
	class ColourTable;
}

namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;

	/**
	 * Creates rendered geometries for all @a ReconstructionGeometry objects
	 * in @a reconstruction and adds them to the RECONSTRUCTION_LAYER of
	 * @a rendered_geom_collection.
	 *
	 * The RECONSTRUCTION_LAYER is first cleared before any rendered geoms are added.
	 * @a colour_table is used to colour RFGs by plate id.
	 */
	void
	render_reconstruction_geometries(
			GPlatesModel::Reconstruction &reconstruction,
			RenderedGeometryCollection &rendered_geom_collection,
			GPlatesGui::ColourTable &colour_table);


	/**
	 * Hook to render reconstruction geometries after a reconstruction.
	 */
	class RenderReconstructionGeometriesHook :
			public GPlatesAppLogic::ReconstructHook
	{
	public:
		RenderReconstructionGeometriesHook(
				RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::ColourTable &colour_table) :
			d_rendered_geometry_collection(rendered_geom_collection),
			d_colour_table(colour_table)
		{  }


		/**
		 * Callback hook after a reconstruction is created.
		 */
		virtual
		void
		post_reconstruction_hook(
				GPlatesModel::ModelInterface &/*model*/,
				GPlatesModel::Reconstruction &reconstruction,
				const double &/*reconstruction_time*/,
				GPlatesModel::integer_plate_id_type /*reconstruction_anchored_plate_id*/,
				GPlatesFeatureVisitors::TopologyResolver &/*topology_resolver*/)
		{
			render_reconstruction_geometries(
					reconstruction,
					d_rendered_geometry_collection,
					d_colour_table);
		}

	private:
		RenderedGeometryCollection &d_rendered_geometry_collection;
		GPlatesGui::ColourTable &d_colour_table;
	};
}

#endif // GPLATES_VIEW_OPERATIONS_RENDERRECONSTRUCTIONGEOMETRIES_H
