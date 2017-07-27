/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VIEWFEATUREGEOMETRIESWIDGET_H
#define GPLATES_QTWIDGETS_VIEWFEATUREGEOMETRIESWIDGET_H

#include <QWidget>

#include "ViewFeatureGeometriesWidgetUi.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewFeatureGeometriesWidget: 
			public QWidget,
			protected Ui_ViewFeatureGeometriesWidget 
	{
		Q_OBJECT
		
	public:
		explicit
		ViewFeatureGeometriesWidget(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~ViewFeatureGeometriesWidget()
		{  }
		

	public Q_SLOTS:
	
		/**
		 * Clears the geometry display in preparation for a new set of geometries.
		 */
		void
		reset();
		
		/**
		 * Updates the dialog to redisplay the geometry of the current Feature.
		 *
		 * Called when the current reconstruction time changes.
		 */
		void
		refresh_display();

		/**
		 * Updates the dialog to display the geometry of a new Feature.
		 * Any changes that might be uncommited from the previous Feature will be discarded.
		 *
		 * Called by FeaturePropertiesDialog after the weak_ref is checked for validity.
		 */
		void
		edit_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg);

	protected:

		virtual
		void
		showEvent(
				QShowEvent *event_);

	private:
		/**
		 * This is the reconstruction generator which is used to obtain the
		 * reconstruction in order to iterate over RFGs.
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;

		/**
		 * This is the feature focus which tracks changes to the currently focused feature.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		
		/**
		 * This is the feature we are displaying. Make sure to check this ref is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		/**
		 * The @a ReconstructionGeometry associated with the feature that is in focus.
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type d_focused_rg;

		/**
		 * The geometry tree is only populated when this widget is visible.
		 *
		 * This is an optimisation that delays populating until this widget is visible in order to
		 * avoid continually populating the widget when the reconstruction time changes or the focused
		 * feature changes, even though the widget is not visible.
		 */
		bool d_populate_geometry_tree_when_visible;
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWFEATUREGEOMETRIESWIDGET_H
