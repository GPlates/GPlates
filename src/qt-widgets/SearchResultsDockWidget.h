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

#ifndef GPLATES_QT_WIDGETS_SEARCHRESULTSDOCKWIDGET_H
#define GPLATES_QT_WIDGETS_SEARCHRESULTSDOCKWIDGET_H

#include <boost/scoped_ptr.hpp>
#include <QPointer>
#include <QWidget>
#include <QTableWidget>

#include "ui_SearchResultsDockWidgetUi.h"

#include "DockWidget.h"


namespace GPlatesGui
{
	class FeatureFocus;
	class FeatureTableModel;
	class TopologySectionsTable;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * A tabbed widget for displaying search results such as clicked features or topology sections.
	 */
	class SearchResultsDockWidget :
			public DockWidget, 
			protected Ui_SearchResultsDockWidget
	{
		Q_OBJECT

	public:

		explicit
		SearchResultsDockWidget(
				GPlatesGui::DockState &dock_state,
				GPlatesGui::FeatureTableModel &feature_table_model,
				ViewportWindow &main_window);

		~SearchResultsDockWidget();

	public Q_SLOTS:

		/**
		 * Highlights the row of the 'clicked geometry' feature table that corresponds to the focused feature.
		 */
		void
		highlight_focused_feature_in_table(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Highlights the first row in the "clicked geometry" feature table.
		 */
		void
		highlight_first_clicked_feature_table_row();

		void
		choose_clicked_geometry_table();

		void
		choose_topology_sections_table();

		void
		set_clicked_geometry_table_tab_text(
				const QString &text);

		void
		set_topology_sections_table_tab_text(
				const QString &text);

	private:

		void
		set_up_clicked_geometries_table();

		void
		set_up_topology_sections_table(
				ViewportWindow &main_window);

		void
		make_signal_slot_connections();


		GPlatesPresentation::ViewState &d_view_state;

		GPlatesGui::FeatureTableModel &d_clicked_feature_table_model;

		/**
		 * Manages the 'Topology Sections' table widget.
		 */
		boost::scoped_ptr<GPlatesGui::TopologySectionsTable> d_topology_sections_table;
	};
}

#endif // GPLATES_QT_WIDGETS_SEARCHRESULTSDOCKWIDGET_H
