/* $Id$ */
/**
* \file 
* $Revision$
* $Date$ 
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

#ifndef GPLATES_QTWIDGETS_SNAPNEARBYVERTICESWIDGET_H
#define GPLATES_QTWIDGETS_SNAPNEARBYVERTICESWIDGET_H

#include <QWidget>
#include "boost/optional.hpp"

#include "SnapNearbyVerticesWidgetUi.h"

#include "model/FeatureHandle.h"
#include "model/types.h"

namespace GPlatesCanvasTools
{
	class ModifyGeometryState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesQtWidgets
{
	class TaskPanel;

	class SnapNearbyVerticesWidget:
		public QWidget,
		protected Ui_SnapNearbyVerticesWidget
	{
		Q_OBJECT
	public:
		SnapNearbyVerticesWidget(
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);
			

		
	private:
		void
		setup_connections();
		
		void
		send_update_signal();
	

		void
		set_default_widget_values();
		
		
	private Q_SLOTS:
		void
		handle_vertex_checkbox_changed(int);
		
		void
		handle_plate_checkbox_changed(int);
		
		void
		handle_spinbox_threshold_changed(double);
		
		void
		handle_spinbox_plate_id_changed(int);	
		
	private:
	
		GPlatesCanvasTools::ModifyGeometryState &d_modify_geometry_state;
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		boost::optional<GPlatesModel::integer_plate_id_type> d_conjugate_plate_id;
		GPlatesModel::FeatureHandle::const_weak_ref d_focused_feature;
		
	};

}


#endif // GPLATES_QTWIDGETS_SNAPNEARBYVERTICES_H
