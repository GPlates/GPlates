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

#include <vector>
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
	 * Returns a sequence of clicked geometries given a click position.
	 *
	 * Tests if any rendered geometries (referencing reconstruction geometries)
	 * contained in @a rendered_geometry_collection are selected by the
	 * clicked point @a click_point_on_sphere and adds any reconstruction geometries
	 * found to @a clicked_geom_seq.
	 *
	 * NOTE: The reconstruction geometries must also give a result of 'true' when
	 * passed to @a filter_recon_geom_predicate.
	 */
	void
	get_clicked_geometries(
			std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &clicked_geom_seq,
			const GPlatesMaths::PointOnSphere &click_point_on_sphere,
			double proximity_inclusion_threshold,
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
			filter_reconstruction_geometry_predicate_type filter_recon_geom_predicate =
					&default_filter_reconstruction_geometry_predicate);


	/**
	 * Adds the clicked geometries in @a clicked_geom_seq to the clicked feature table.
	 *
	 * Also updates status bar of @a view_state with the number of clicked rendered geometries.
	 * Also unsets the feature focus if no reconstruction geometries were clicked.
	 *
	 * If @a highlight_first_clicked_feature_in_table is true then the first clicked feature in the
	 * table will be highlighted (will be the focused feature), otherwise the currently focused
	 * feature will be highlighted.
	 * Setting @a highlight_first_clicked_feature_in_table to true is useful when the user just
	 * clicked on the globe (so you want to ignore the previously focused feature).
	 * Setting @a highlight_first_clicked_feature_in_table to false is useful when restoring the
	 * clicked table (and focused feature) to a previous state.
	 */
	void
	add_clicked_geometries_to_feature_table(
			const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &clicked_geom_seq,
			GPlatesQtWidgets::ViewportWindow &view_state,
			GPlatesGui::FeatureTableModel &clicked_table_model,
			GPlatesGui::FeatureFocus &feature_focus,
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
			bool highlight_first_clicked_feature_in_table = true);


	/**
	 * Combines the above two functions (@a get_clicked_geometries and @a add_clicked_geometries_to_feature_table).
	 */
	inline
	void
	get_and_add_clicked_geometries_to_feature_table(
			const GPlatesMaths::PointOnSphere &click_point_on_sphere,
			double proximity_inclusion_threshold,
			GPlatesQtWidgets::ViewportWindow &view_state,
			GPlatesGui::FeatureTableModel &clicked_table_model,
			GPlatesGui::FeatureFocus &feature_focus,
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
			filter_reconstruction_geometry_predicate_type filter_recon_geom_predicate =
					&default_filter_reconstruction_geometry_predicate,
			bool highlight_first_clicked_feature_in_table = true)
	{
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> clicked_geom_seq;

		get_clicked_geometries(
				clicked_geom_seq,
				click_point_on_sphere,
				proximity_inclusion_threshold,
				rendered_geometry_collection,
				filter_recon_geom_predicate);

		add_clicked_geometries_to_feature_table(
				clicked_geom_seq,
				view_state,
				clicked_table_model,
				feature_focus,
				reconstruct_graph,
				highlight_first_clicked_feature_in_table);
	}


	/**
	 * Inserts a new feature/geometry entry @a reconstruction_geometry_ptr into the
	 * @a clicked_table_model at the top (row 0) of the table, moving all other
	 * entries down one row.
	 */
	void
	add_geometry_to_top_of_feature_table(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry_ptr,
			GPlatesGui::FeatureTableModel &clicked_table_model,
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph);
}

#endif // GPLATES_GUI_ADDCLICKEDGEOMETRIESTOFEATURETABLE_H
