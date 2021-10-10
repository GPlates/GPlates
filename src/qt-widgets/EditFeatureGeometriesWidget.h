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
 
#ifndef GPLATES_QTWIDGETS_EDITFEATUREGEOMETRIESWIDGET_H
#define GPLATES_QTWIDGETS_EDITFEATUREGEOMETRIESWIDGET_H

#include <QWidget>
#include "EditFeatureGeometriesWidgetUi.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class EditFeatureGeometriesWidget: 
			public QWidget,
			protected Ui_EditFeatureGeometriesWidget 
	{
		Q_OBJECT
		
	public:
		explicit
		EditFeatureGeometriesWidget(
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		virtual
		~EditFeatureGeometriesWidget()
		{  }
		

	public slots:
	
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
		 * Updates the dialog to display and edit the geometry of a new Feature.
		 * Any changes that might be uncommited from the previous Feature will be discarded.
		 *
		 * Called by FeaturePropertiesDialog after the weak_ref is checked for validity.
		 */
		void
		edit_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);

	private slots:
		
	private:
		/**
		 * This is the view state which is used to obtain the reconstruction in order to
		 * iterate over RFGs.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the feature focus which tracks changes to the currently focused feature.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		
		/**
		 * This is the feature we are displaying. Make sure to check this ref is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITFEATUREGEOMETRIESWIDGET_H
