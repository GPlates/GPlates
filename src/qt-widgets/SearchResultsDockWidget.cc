/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "SearchResultsDockWidget.h"

#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"

#include "gui/AddClickedGeometriesToFeatureTable.h"
#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/TopologySectionsTable.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::SearchResultsDockWidget::SearchResultsDockWidget(
		GPlatesGui::DockState &dock_state,
		GPlatesGui::FeatureTableModel &feature_table_model,
		ViewportWindow &main_window):
	// Use empty string for dock title so it doesn't display in the title bar...
	DockWidget("", dock_state, main_window, QString("search_results")),
	d_view_state(main_window.get_view_state()),
	d_clicked_feature_table_model(feature_table_model)
{
	setupUi(this);

	set_up_clicked_geometries_table();

	set_up_topology_sections_table(main_window);

	make_signal_slot_connections();
}


GPlatesQtWidgets::SearchResultsDockWidget::~SearchResultsDockWidget()
{
	// boost::scoped_ptr destructors need complete type.
}


void
GPlatesQtWidgets::SearchResultsDockWidget::set_up_clicked_geometries_table()
{
	// Set up the Clicked table (which is now actually a tree in disguise).
	tree_view_clicked_geometries->setRootIsDecorated(false);
	tree_view_clicked_geometries->setModel(&d_clicked_feature_table_model);
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*tree_view_clicked_geometries->header());
	tree_view_clicked_geometries->header()->setMinimumSectionSize(60);
	tree_view_clicked_geometries->header()->setSectionsMovable(true);
}


void
GPlatesQtWidgets::SearchResultsDockWidget::set_up_topology_sections_table(
		ViewportWindow &main_window)
{
	// Set up the Topology Sections Table, now that the table widget has been created.
	d_topology_sections_table.reset(
			new GPlatesGui::TopologySectionsTable(
					*table_widget_topology_sections,
					main_window.get_view_state().get_topology_boundary_sections_container(), 
					main_window.get_view_state().get_topology_interior_sections_container(),
					main_window.get_view_state()));
}


void
GPlatesQtWidgets::SearchResultsDockWidget::make_signal_slot_connections()
{
	// When the user selects a row of the table, we should focus that feature.
	// RJW: This is what triggers the highlighting of the geometry on the canvas.
	QObject::connect(tree_view_clicked_geometries->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			&d_clicked_feature_table_model,
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));
}


void
GPlatesQtWidgets::SearchResultsDockWidget::highlight_focused_feature_in_table(
		GPlatesGui::FeatureFocus &feature_focus)
{
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type rg_maybe_ptr = 
			feature_focus.associated_reconstruction_geometry();
	if ( ! rg_maybe_ptr)
	{
		return;
	}
	GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type rg_ptr(rg_maybe_ptr.get());
	
	// Check to see if this newly focused feature is in the Clicked table already.
	QModelIndex idx = d_clicked_feature_table_model.get_index_for_geometry(rg_ptr);
	if (idx.isValid())
	{
		// It is. Move the highlight to that line (if we've been good, this won't cause
		// an infinite loop of 'change' signals because FeatureFocus won't emit anything
		// if we tell it to focus something that's already focused).
		tree_view_clicked_geometries->selectionModel()->clear();
		tree_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}
	else
	{
		// It is not in there. Most likely this is from the Clone Feature action setting
		// the focus directly. 'Unshift' it onto the start of the Clicked list.
		GPlatesGui::add_geometry_to_top_of_feature_table(rg_ptr, d_clicked_feature_table_model,
				d_view_state.get_application_state().get_reconstruct_graph());
		highlight_first_clicked_feature_table_row();
	}
}


void
GPlatesQtWidgets::SearchResultsDockWidget::highlight_first_clicked_feature_table_row()
{
	QModelIndex idx = d_clicked_feature_table_model.index(0, 0);

	if (idx.isValid())
	{
		tree_view_clicked_geometries->selectionModel()->clear();
		tree_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}

	tree_view_clicked_geometries->scrollToTop();

	// The columns of the table (especially the last column) tend not to adjust
	// properly for some reason, unless we force them to:-
	for (int i = 0; i < d_clicked_feature_table_model.columnCount(); ++i)
	{
		tree_view_clicked_geometries->resizeColumnToContents(i);
	}
}


void
GPlatesQtWidgets::SearchResultsDockWidget::choose_clicked_geometry_table()
{
	tab_widget_search_results->setCurrentWidget(tab_clicked);
}


void
GPlatesQtWidgets::SearchResultsDockWidget::choose_topology_sections_table()
{
	tab_widget_search_results->setCurrentWidget(tab_topology);
}


void
GPlatesQtWidgets::SearchResultsDockWidget::set_clicked_geometry_table_tab_text(
		const QString &text)
{
	tab_widget_search_results->setTabText(
			tab_widget_search_results->indexOf(tab_clicked),
			text);
}


void
GPlatesQtWidgets::SearchResultsDockWidget::set_topology_sections_table_tab_text(
		const QString &text)
{
	tab_widget_search_results->setTabText(
			tab_widget_search_results->indexOf(tab_topology),
			text);
}
