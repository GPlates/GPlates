/* $Id: CreateVGPDialog.h 11436 2011-05-04 11:04:00Z rwatson $ */

/**
 * \file 
 * $Revision: 11436 $
 * $Date: 2011-05-04 13:04:00 +0200 (on, 04 mai 2011) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_CREATESMALLCIRCLEFEATUREDIALOG_H
#define GPLATES_QTWIDGETS_CREATESMALLCIRCLEFEATUREDIALOG_H

#include <QWidget>

#include "CreateSmallCircleFeatureDialogUi.h"

#include "app-logic/FeatureCollectionFileState.h"
#include "maths/SmallCircle.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileState;
	class FeatureCollectionFileIO;
}


namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget;
	class EditTimePeriodWidget;

	class CreateSmallCircleFeatureDialog :
			public QDialog,
			protected Ui_CreateSmallCircleFeatureDialog
	{
		Q_OBJECT

		// Convenience typedef for small circle collection.
		typedef std::vector<GPlatesMaths::SmallCircle> small_circle_collection_type;

	public:
	
		enum StackedWidgetPage
		{
			PROPERTIES_PAGE,
			COLLECTION_PAGE
		};	

		CreateSmallCircleFeatureDialog(
			GPlatesAppLogic::ApplicationState *app_state_ptr,
			const small_circle_collection_type &small_circles,
			QWidget *parent_ = NULL);
			
		/**
		 * Reset the state of the dialog for a new creation process.                                                                     
		 */	
		void
		reset();
			
	Q_SIGNALS:

		void
		feature_created();			
	
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

		/**
		 * Widget for defining a time period.                                                                    
		 */
		EditTimePeriodWidget *d_edit_time_period_widget;

		/**
		 * A reference to the small circle collection which we will make features out of.                                                                    
		 */
		const small_circle_collection_type &d_small_circles;

	};
}

#endif //GPLATES_QTWIDGETS_CREATESMALLCIRCLEFEATUREDIALOG_H