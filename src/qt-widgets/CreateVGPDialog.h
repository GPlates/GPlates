/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_CREATEVGPDIALOG_H
#define GPLATES_QTWIDGETS_CREATEVGPDIALOG_H

#include <QWidget>

#include "CreateVGPDialogUi.h"

#include "GPlatesDialog.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileState;
	class FeatureCollectionFileIO;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget;

	class CreateVGPDialog :
			public GPlatesDialog,
			protected Ui_CreateVGPDialog
	{
		Q_OBJECT

	public:
	
		enum StackedWidgetPage
		{
			PROPERTIES_PAGE,
			COLLECTION_PAGE
		};	

		explicit
		CreateVGPDialog(
			GPlatesPresentation::ViewState &view_state_,
			QWidget *parent_ = NULL);
			
		/**
		 * Reset the state of the dialog for a new creation process.                                                                     
		 */	
		void
		reset();
			
	Q_SIGNALS:

		void
		feature_created();


		// FIXME: Not sure if this signal is required any more. 
		void
		feature_collection_created(
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection,
			GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_iter);			
	
	private:
		void
		setup_connections();
		
		void
		setup_properties_page();
		
		void
		setup_collection_page();
		
	private Q_SLOTS:
		
		void
		handle_previous();
		
		void
		handle_next();
		
		void
		handle_create();
		
		void
		handle_cancel();
		
		void
		handle_site_checked(int state);
		
	private:

		/**
		 * The Model interface, used to create new features.
		 */
		GPlatesModel::ModelInterface d_model_ptr;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;

		/**
		 * Used to create an empty feature collection file.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO &d_file_io;
		
		/**
		 * The application state is used to access the reconstruction tree to
		 * perform reverse reconstruction of the temporary geometry (once we know the plate id).
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;	

		/**
		 * The widget that allows the user to select an existing feature collection
		 * to add the new feature to, or a new feature collection.
		 * Memory managed by Qt.
		 */
		ChooseFeatureCollectionWidget *d_choose_feature_collection_widget;
	};
}

#endif //GPLATES_QTWIDGETS_CREATEVGPDIALOG_H
