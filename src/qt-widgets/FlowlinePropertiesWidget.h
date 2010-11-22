/* $Id: FlowlinePropertiesWidget.h 8461 2010-05-20 14:18:01Z rwatson $ */

/**
* \file 
* $Revision: 8461 $
* $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $ 
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_FLOWLINEPROPERTIESWIDGET_H
#define GPLATES_QTWIDGETS_FLOWLINEPROPERTIESWIDGET_H

#include <QDebug>
#include <QWidget>

#include "AbstractCustomPropertiesWidget.h"
#include "FlowlinePropertiesWidgetUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class EditTimeSequenceWidget;

	class FlowlinePropertiesWidget:
		public AbstractCustomPropertiesWidget,
		protected Ui_FlowlinePropertiesWidget
	{
		Q_OBJECT
	public:
		explicit
		FlowlinePropertiesWidget(
			GPlatesAppLogic::ApplicationState *application_state_ptr,
			QWidget *parent_ = NULL);
			
		~FlowlinePropertiesWidget()
		{
			qDebug() << "Deleting FlowlinePropertiesWidget";
		}	
		
		virtual
		void
		add_properties_to_feature(
			GPlatesModel::FeatureHandle::weak_ref feature_handle);
		
		virtual
		void
		add_geometry_properties_to_feature(
			GPlatesModel::PropertyValue::non_null_ptr_type geometry_property,
			GPlatesModel::FeatureHandle::weak_ref feature_handle);	
					
		void
		update();
		
	private	slots:

		void
		handle_checkbox_status_changed();
		
	private:
		
		/**
		 * Application state, for getting reconstruction time.     
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;
		
		/**
		 * Custom edit widget used for time sequence. 
		 */
		EditTimeSequenceWidget *d_time_sequence_widget;
		
		
	};
}

#endif // GPLATES_QTWIDGETS_FLOWLINEPROPERTIESWIDGET_H

