/* $Id: FlowlinePropertiesWidget.h 8461 2010-05-20 14:18:01Z rwatson $ */

/**
* \file 
* $Revision: 8461 $
* $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $ 
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway
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
	class CreateFeatureDialog;
	class EditTimeSequenceWidget;

	class FlowlinePropertiesWidget:
		public AbstractCustomPropertiesWidget,
		protected Ui_FlowlinePropertiesWidget
	{
		Q_OBJECT
	public:

		explicit
		FlowlinePropertiesWidget(
				const GPlatesAppLogic::ApplicationState &application_state,
				QWidget *parent_ = NULL);
			
		~FlowlinePropertiesWidget()
		{}	

        virtual
        GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
        do_geometry_tasks(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstruction_time_geometry_,
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);
		
		
	private:
		
		/**
		 * Application state, for getting reconstruction time.     
		 */
		const GPlatesAppLogic::ApplicationState &d_application_state;
		
	};
}

#endif // GPLATES_QTWIDGETS_FLOWLINEPROPERTIESWIDGET_H

