/**
 * \file 
 * $Revision: 5534 $
 * $Date: 2009-04-20 17:17:43 -0700 (Mon, 20 Apr 2009) $ 
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
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
 
#ifndef GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H
#define GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H

#include <QDebug>
#include <QWidget>

#include "TopologyToolsWidgetUi.h"
#include "TaskPanelWidget.h"

#include "global/types.h"
#include "model/FeatureHandle.h"

#include "gui/TopologyTools.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
	class FeatureFocus;
//	class TopologyTools;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class CreateFeatureDialog;
	class FeatureSummaryWidget;

	class TopologyToolsWidget :
			public TaskPanelWidget, 
			protected Ui_TopologyToolsWidget
	{
		Q_OBJECT
		
	public:

		explicit
		TopologyToolsWidget(
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				QAction *clear_action,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				QWidget *parent_ = NULL);

		~TopologyToolsWidget();

		virtual
		void
		handle_activation();

		virtual
		QString
		get_clear_action_text() const;

		virtual
		bool
		clear_action_enabled() const;

		virtual
		void
		handle_clear_action_triggered();

	public slots:
		
		void 
		activate( GPlatesGui::TopologyTools::CanvasToolMode mode );

		void 
		deactivate();

		void
		clear();

		void
		display_topology(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type associated_rg,
				GPlatesGlobal::TopologyTypes topology_type);

		void
		display_number_of_sections_boundary( int i ) {
			label_num_sections_boundary->setText( QString::number( i ) );
		}

		void
		display_number_of_sections_interior( int i ) {
			label_num_sections_interior->setText( QString::number( i ) );
		}

		int
		get_sections_combobox_index()
		{
			return sections_combobox->currentIndex();
		}

		void
		set_sections_combobox_index( int index )
		{
			sections_combobox->setCurrentIndex( index );
		}

		void
		handle_sections_combobox_index_changed( int index );

		void
		handle_remove_all_sections();

		void
		handle_create();
	
		void
		handle_add_feature_boundary();

		void
		handle_add_feature_interior();

		void
		handle_remove_feature();

		void
		choose_topology_tab()
		{
			tabwidget_main->setCurrentWidget( tab_topology );
		}

		void
		choose_section_tab()
		{
			tabwidget_main->setCurrentWidget( tab_section );
		}

	private:

		void
		setup_widgets();

		void
		setup_connections();

		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * The model
		 */ 
		GPlatesModel::ModelInterface *d_model_interface;

		/**
		 * The dialog the user sees when they hit the Create button.
		 * Memory managed by Qt.
		 */
		CreateFeatureDialog *d_create_feature_dialog;

		/** The tools to create and edit the topology feature */
		GPlatesGui::TopologyTools *d_topology_tools_ptr;

		/** the FeatureSummaryWidget pointer */
		FeatureSummaryWidget *d_feature_summary_widget_ptr;
		
	};
}

#endif  // GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H

