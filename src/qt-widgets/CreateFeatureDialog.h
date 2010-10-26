/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include "app-logic/ReconstructionMethod.h"

#include "maths/GeometryOnSphere.h"

#include "model/ModelInterface.h"
#include "model/FeatureHandle.h"

class QComboBox;

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
	class ChooseFeatureTypeWidget;
	class EditPlateIdWidget;
	class EditTimePeriodWidget;
	class EditStringWidget;
	class ChooseGeometryPropertyWidget;
	class InformationDialog;
	class ViewportWindow;

	class CreateFeatureDialog :
			public QDialog, 
			protected Ui_CreateFeatureDialog
	{
		Q_OBJECT
		
	public:

		/**
		* What kinds of features to create 
		*/ 
		enum FeatureType
		{
			NORMAL, TOPOLOGICAL
		};

		explicit
		CreateFeatureDialog(
				GPlatesPresentation::ViewState &view_state_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				FeatureType creation_type,
				QWidget *parent_ = NULL);
		
		/**
		 * Rather than simply exec()ing the dialog, you should call this method to
		 * ensure you are feeding the CreateFeatureDialog some valid geometry
		 * at the same time.
		 */
		bool
		set_geometry_and_display(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_);

		bool
		display();

		GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() {
			return d_feature_ref;
		}

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

		void
		handle_create_and_save();

		void
		recon_method_changed(
				int index);		
		
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
		 * The reconstruction generator is used to access the reconstruction tree to
		 * perform reverse reconstruction of the temporary geometry (once we know the plate id).
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;

		/**
		 * Used to popup dialogs in the main window.
		 */
		ViewportWindow *d_viewport_window_ptr;

		/**
		* Type of feature to create 
		*/
		FeatureType d_creation_type;

		/**
		 * The geometry that is to be included with the feature.
		 * Note that the coordinates may have to be moved to present-day, once we know
		 * what plate ID the user wishes to assign to the feature.
		 * 
		 * This may be boost::none if the create dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_opt_ptr;

		/**
		 * The custom edit widget for reconstruction. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_plate_id_widget;

		/**
		 * The custom edit widget for conjugate plate id. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_conjugate_plate_id_widget;

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

		/**
		 * The widget that allows the user to select the feature type of the new feature.
		 * Memory managed by Qt.
		 */
		ChooseFeatureTypeWidget *d_choose_feature_type_widget;

		/**
		 * The widget that allows the user to select an existing feature collection
		 * to add the new feature to, or a new feature collection.
		 * Memory managed by Qt.
		 */
		ChooseFeatureCollectionWidget *d_choose_feature_collection_widget;

		/**
		 * The newly created feature.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		/**
		* reconstruction method combox
		*/
		QComboBox *d_recon_method_combobox;
		
		/**
		* right plate id
		*/
		EditPlateIdWidget *d_right_plate_id;
		
		/**
		* left plate id
		*/
		EditPlateIdWidget *d_left_plate_id;

		/**
		 * Allows the user to pick the property that will store the geometry.
		 */
		ChooseGeometryPropertyWidget *d_listwidget_geometry_destinations;

		GPlatesAppLogic::ReconstructionMethod::Type d_recon_method;
	};
}

#endif  // GPLATES_QTWIDGETS_CREATEFEATUREDIALOG_H

