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
 
#ifndef GPLATES_QTWIDGETS_CREATEFEATUREDIALOG_H
#define GPLATES_QTWIDGETS_CREATEFEATUREDIALOG_H

#include <boost/optional.hpp>
#include "CreateFeatureDialogUi.h"

#include "maths/GeometryOnSphere.h"
#include "model/ModelInterface.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class InformationDialog;
	class EditPlateIdWidget;
	class EditTimePeriodWidget;
	class EditStringWidget;

	class CreateFeatureDialog:
			public QDialog, 
			protected Ui_CreateFeatureDialog
	{
		Q_OBJECT
		
	public:

		explicit
		CreateFeatureDialog(
				GPlatesModel::ModelInterface &model_interface,
				GPlatesQtWidgets::ViewportWindow &view_state_,
				QWidget *parent_ = NULL);
		
		/**
		 * Rather than simply exec()ing the dialog, you should call this method to
		 * ensure you are feeding the CreateFeatureDialog some valid geometry
		 * at the same time.
		 */
		bool
		set_geometry_and_display(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_);
	
	signals:
		
		void
		feature_created(
				GPlatesModel::FeatureHandle::weak_ref feature);
					
	private slots:
		
		void
		handle_prev();

		void
		handle_next();
		
		void
		handle_page_change(
				int page);

		void
		handle_create();
		
	private:
	
		void
		set_up_button_box();
		
		void
		set_up_feature_type_page();

		void
		set_up_feature_properties_page();

		void
		set_up_feature_collection_page();
		
		void
		set_up_geometric_property_list();
		
		/**
		 * The Model interface, used to create new features.
		 */
		GPlatesModel::ModelInterface *d_model_ptr;
	
		/**
		 * The View State is used to access the reconstruction tree to perform reverse
		 * reconstruction of the temporary geometry (once we know the plate id).
		 */
		ViewportWindow *d_view_state_ptr;

		/**
		 * The geometry that is to be included with the feature.
		 * Note that the coordinates may have to be moved to present-day, once we know
		 * what plate ID the user wishes to assign to the feature.
		 * 
		 * This may be boost::none if the create dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_opt_ptr;

		/**
		 * The custom edit widget for GpmlPlateId. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_plate_id_widget;

		/**
		 * The custom edit widget for GmlTimePeriod. Memory managed by Qt.
		 */
		EditTimePeriodWidget *d_time_period_widget;

		/**
		 * The custom edit widget for XsString which we are using for the gml:name property.
		 * Memory managed by Qt.
		 */
		EditStringWidget *d_name_widget;
		
		/**
		 * Button added to buttonbox for the final feature creation step; takes the place
		 * of an 'OK' button.
		 * Memory managed by Qt. This is a member so that we can enable and disable it as appropriate.
		 */
		QPushButton *d_button_create;
		
	};
}

#endif  // GPLATES_QTWIDGETS_CREATEFEATUREDIALOG_H

