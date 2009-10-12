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
 
#ifndef GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H
#define GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QWidget>

#include "ManageFeatureCollectionsStateWidgetUi.h"

#include "app-logic/FeatureCollectionFileState.h"


namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsDialog;
	
	
	class ManageFeatureCollectionsStateWidget:
			public QWidget, 
			protected Ui_ManageFeatureCollectionsStateWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ManageFeatureCollectionsStateWidget(
				ManageFeatureCollectionsDialog &feature_collections_dialog,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it,
				bool reconstructable_active,
				bool reconstruction_active,
				bool enable_reconstructable,
				bool enable_reconstruction,
				QWidget *parent_ = NULL);

		/**
		 * Updates the reconstructable state of StateWidget by checking/unchecking checkboxes and
		 * disabling/enabling buttons as necessary.
		 */
		void
		update_reconstructable_state(
				bool active,
				bool enable_reconstructable);
	
		/**
		 * Updates the reconstruction state of StateWidget by checking/unchecking checkboxes and
		 * disabling/enabling buttons as necessary.
		 */
		void
		update_reconstruction_state(
				bool active,
				bool enable_reconstruction);
	
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator
		get_file_iterator() const
		{
			return d_file_iterator;
		}
	
	private slots:
		
		void
		handle_active_reconstructable_checked(
				bool checked);

		void
		handle_active_reconstructable_toggled();

		void
		handle_active_reconstruction_checked(
				bool checked);

		void
		handle_active_reconstruction_toggled();
	
	private:
	
		/**
		 * Reconfigures the button icon, tooltip etc. to indicate state.
		 */
		void
		set_reconstructable_button_properties(
				bool is_active,
				bool is_enabled);

		/**
		 * Reconfigures the button icon, tooltip etc. to indicate state.
		 */
		void
		set_reconstruction_button_properties(
				bool is_active,
				bool is_enabled);
	
		ManageFeatureCollectionsDialog &d_feature_collections_dialog;
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator d_file_iterator;
		
	};
}

#endif  // GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H
