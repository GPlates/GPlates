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

#ifndef GPLATES_GUI_ADDCLICKEDGEOMETRIESTOFEATURETABLE_H
#define GPLATES_GUI_ADDCLICKEDGEOMETRIESTOFEATURETABLE_H

#include <boost/function.hpp>

#include "app-logic/ReconstructionGeometry.h"


namespace GPlatesAppLogic
{
	class ReconstructGraph;
}

namespace GPlatesGui
{
	class FeatureFocus;
	class FeatureTableModel;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	/**
	 * Typedef for a boost function (predicate) used to filter reconstruction geometries.
	 *
	 * It takes a reconstruction geometry as its argument and returns a bool.
	 */
	typedef boost::function<bool (const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &)>
			filter_reconstruction_geometry_predicate_type;


	/**
	 * The default reconstruction geoemetry filter always returns true.
	 */
	inline
	bool
	default_filter_reconstruction_geometry_predicate(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &)
	{
		return true;
	}


	/**
	 * Tests if any rendered geometries (referencing reconstruction geometries)
	 * contained in @a rendered_geometry_collection are selected by the
	 * clicked point @a click_point_on_sphere and adds any reconstruction geometries
	 * found to @a clicked_table_model.
	 *
	 * NOTE: The reconstruction geometries must also give a result of 'true' when
	 * passed to @a filter_recon_geom_predicate.
	 *
	 * Also updates status bar of @a view_state with the number of clicked
	 * rendered geometries (that reference reconstruction geometries and pass
	 * the @a filter_recon_geom_predicate test).
	 * Also unsets the feature focus if no reconstruction geometries were clicked.
	 */
	void
	add_clicked_geometries_to_feature_table(
			const GPlatesMaths::PointOnSphere &click_point_on_sphere,
			double proximity_inclusion_threshold,
			const GPlatesQtWidgets::ViewportWindow &view_state,
			GPlatesGui::FeatureTableModel &clicked_table_model,
			GPlatesGui::FeatureFocus &feature_focus,
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
			filter_reconstruction_geometry_predicate_type filter_recon_geom_predicate =
					&default_filter_reconstruction_geometry_predicate);
}

#endif // GPLATES_GUI_ADDCLICKEDGEOMETRIESTOFEATURETABLE_H
