/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include <QCheckBox>

#include "CreateFeatureDialogUi.h"

#include "CreateFeaturePropertiesPage.h"

#include "app-logic/ReconstructMethodType.h"

#include "gui/CanvasToolWorkflows.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/ModelInterface.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/TopLevelProperty.h"

#include "property-values/StructuralType.h"


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
	class AbstractCustomPropertiesWidget;
	class ChooseFeatureCollectionWidget;
	class ChooseFeatureTypeWidget;
	class ChoosePropertyWidget;
	class CreateFeaturePropertiesPage;
	class EditPlateIdWidget;
	class EditTimePeriodWidget;
	class EditStringWidget;
	class InformationDialog;
	class ResizeToContentsTextEdit;
	class ViewportWindow;

	class CreateFeatureDialog :
			public QDialog, 
			protected Ui_CreateFeatureDialog
	{
		Q_OBJECT
		
	public:

		enum StackedWidgetPage
		{
			FEATURE_TYPE_PAGE,
			COMMON_PROPERTIES_PAGE,
			ALL_PROPERTIES_PAGE,
			CONJUGATE_PROPERTIES_PAGE,
			FEATURE_COLLECTION_PAGE
		};


		CreateFeatureDialog(
				GPlatesPresentation::ViewState &view_state_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				QWidget *parent_ = NULL);


		/**
		 * Rather than simply exec()ing the dialog, you should call this method to ensure
		 * you are feeding the CreateFeatureDialog some valid geometry at the same time.
		 *
		 * NOTE: The property value does not need to be wrapped in a time-dependent wrapper because
		 * the GPGIM will be used to determine if that should be done based on the property name.
		 *
		 * NOTE: An exception is thrown if the specified geometric property value is not one
		 * of the following structural types:
		 *   GPlatesPropertyValues::GmlLineString::non_null_ptr_type,
		 *   GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type,
		 *   GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type,
		 *   GPlatesPropertyValues::GmlPoint::non_null_ptr_type,
		 *   GPlatesPropertyValues::GmlPolygon::non_null_ptr_type,
		 *   GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type,
		 *   GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type, or
		 *   GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type.
		 *
		 * Returns true on success.
		 */
		bool
		set_geometry_and_display(
				const GPlatesModel::PropertyValue::non_null_ptr_type &geometry_property_value);


	Q_SIGNALS:
		
		void
		feature_created(
				GPlatesModel::FeatureHandle::weak_ref feature);
					
	private Q_SLOTS:
		
		void
		handle_prev();

		void
		handle_next();
		
		/**
		 * @brief Called when we are switching between pages.
		 * Delegates to handle_enter_page and handle_leave_page appropriately.
		 * @param page the new page we are transitioning to
		 */
		void
		handle_page_change(
				int page);
		
		void
		handle_leave_page(
		        int page);

		void
		handle_enter_page(
		        int page,
		        int last_page);
		
		/**
		 * The 'create' button on the last page has been pushed; assemble our lists of properties
		 * into actual features (using create_feature()), add geometry properties and reverse-reconstruct
		 * them so the geometry is properly expressed in present-day coordinates, then add them to
		 * the selected feature collection.
		 */		
		void
		handle_create();

		void
		handle_create_and_save();

		void
		recon_method_changed(
				int index);		

		void
		handle_conjugate_plate_id_changed();

		void
		handle_feature_type_changed();
		
		void
		handle_canvas_tool_triggered(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

	private:
	
		void
		set_up_button_box();

		void
		set_up_custom_properties_page();
		
		void
		set_up_feature_type_page();

		void
		set_up_common_properties_page();

		void
		set_up_feature_properties_page();

		void
		set_up_conjugate_properties_page();

		void
		set_up_feature_collection_page();

		void
		set_up_feature_list();

		void
		select_default_feature_type();

		void
		set_up_geometric_property_list();

		void
		select_default_geometry_property_name();

		void
		set_up_common_properties();

		void
		set_up_all_properties_gui();

		void
		set_up_conjugate_properties_gui();
		
		void
		clear_properties_not_allowed_for_current_feature_type();

		void
		copy_common_properties_into_all_properties();
		
		void
		copy_common_property_into_all_properties(
				const GPlatesModel::PropertyName &property_name,
				const GPlatesModel::PropertyValue::non_null_ptr_type &property_value);

		void
		remove_common_property_from_all_properties(
				const GPlatesModel::PropertyName &property_name);

		/**
		 * Takes the "all properties" list d_feature_properties and populates the "conjugate properties"
		 * list d_conjugate_properties based on that.
		 */
		void
		generate_conjugate_properties_from_all_properties();

		/**
		 * Helper for generate_conjugate_properties_from_all_properties(), in cases where we need
		 * to tweak a property's value.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		generate_conjugate_property(
				const GPlatesModel::PropertyName &property_name,
				const GPlatesModel::PropertyValue::non_null_ptr_type &property_value);

		bool
		display();

		/**
		 * Handles the main effort of constructing a new feature based on the list of properties
		 * we have assembled during this dialog's invocation. Can be used for both primary and
		 * conjugate feature creation. Does not add the returned Feature to a FeatureCollection.
		 *
		 * Called by handle_create(); can throw InvalidPropertyValueException.
		 */
		GPlatesModel::FeatureHandle::non_null_ptr_type
		create_feature(
		        const GPlatesModel::FeatureType feature_type,
		        const CreateFeaturePropertiesPage::property_seq_type feature_properties,
		        const GPlatesModel::PropertyName geometry_property_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		
		boost::optional<GPlatesModel::FeatureHandle::iterator>
		add_geometry_property(
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const GPlatesModel::PropertyName &geometry_property_name);

		bool
		reverse_reconstruct_geometry_property(
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Given two features and a property name (e.g. gpml:conjugate), create a FeatureReference
		 * in @a feature linking to @a target_feature.
		 */
		void
		create_feature_link(
		        GPlatesModel::FeatureHandle::non_null_ptr_type feature,
		        const GPlatesModel::PropertyName &property_name,
		        GPlatesModel::FeatureHandle::non_null_ptr_to_const_type target_feature);

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
		 * The geometry that is to be included with the feature.
		 * Note that the coordinates may have to be moved to present-day (for non-topological geometries),
		 * once we know what plate ID the user wishes to assign to the feature.
		 * 
		 * This may be boost::none if the create dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_geometry_property_value;

		/**
		 * The geometry type of the geometry that is to be included with the feature.
		 * 
		 * This may be boost::none if the create dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesPropertyValues::StructuralType> d_geometry_property_type;

		/**
		 * The feature type (if any selected).
		 *
		 * A feature type will always be selected unless, for some reason, there are no feature
		 * types populated in the list (should only be possible if GPGIM is incorrect).
		 */
		boost::optional<GPlatesModel::FeatureType> d_feature_type;

		/**
		 * The feature type that was previously selected when we last left the Choose Feature Type page.
		 *
		 * Since we want to enable a workflow where repeatedly digitising the same feature type keeps
		 * properties around from the last feature that was created, we want to be able to test whether
		 * the user has selected a different feature type recently.
		 */
		boost::optional<GPlatesModel::FeatureType> d_previously_selected_feature_type;
		
		/**
		 * The custom edit widget for reconstruction. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_plate_id_widget;

		/**
		 * The custom edit widget for conjugate plate id. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_conjugate_plate_id_widget;

		/**
		 * The custom edit widget for 'relative plate' id (for MotionPath feature type). Memory managed by Qt.
		 */
		EditPlateIdWidget *d_relative_plate_id_widget;

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
		 * A feature type description QTextEdit that resizes to its contents.
		 */
		ResizeToContentsTextEdit *d_feature_type_description_widget;

		/**
		 * The widget that allows the user to select an existing feature collection
		 * to add the new feature to, or a new feature collection.
		 * Memory managed by Qt.
		 */
		ChooseFeatureCollectionWidget *d_choose_feature_collection_widget;

		/**
		 * The reconstruction method widget (containing label and combobox).
		 */
		QWidget *d_recon_method_widget;

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
		 * Abstract base widget for custom properties widgets.                                                                    
		 * 
		 * This widget handles special-case properties for specific feature types.
		 * This is currently only needed for a very small number of feature types.
		 */
		boost::optional<AbstractCustomPropertiesWidget *> d_custom_properties_widget;

		/**
		 * The stacked widget page where properties (allowed by the GPGIM for the feature type)
		 * can be added to the feature by the user.
		 */
		CreateFeaturePropertiesPage *d_create_feature_properties_page;

		/**
		 * The stacked widget page where properties (allowed by the GPGIM for the feature type)
		 * can be modified and added for the conjugate feature, if one is to be created.
		 */
		CreateFeaturePropertiesPage *d_create_conjugate_properties_page;
		
		/**
		 * Allows the user to pick the property that will store the geometry.
		 */
		ChoosePropertyWidget *d_listwidget_geometry_destinations;

		GPlatesAppLogic::ReconstructMethod::Type d_recon_method;

		/**
		 *  Checkbox for creating a conjugate feature.                                                                    
		 */
		QCheckBox *d_create_conjugate_feature_checkbox;

		/**
		 * The index of the current stacked widget page.
		 *
		 * This is used, along with "currentIndex()", to determine page transitions.
		 */
		StackedWidgetPage d_current_page;

		/**
		 * The properties (excluding the selected geometry property) to create the feature with.
		 *
		 * These are also kept around from the previous dialog invocation if the geometry
		 * and feature types are the same (then user has option to re-use same properties).
		 * The list is partially cleared via clear_properties_not_allowed_for_current_feature_type()
		 * during handle_page_change() from the feature type selection to the common properties page
		 * if we are dealing with a new feature type.
		 */
		CreateFeaturePropertiesPage::property_seq_type d_feature_properties;

		/**
		 * The properties (excluding the selected geometry property) to create the conjugate feature with.
		 *
		 * This is only used if the "Create Conjugate" checkbox was shown and is checked. Properties
		 * in this list are first populated based on d_feature_properties (while swapping things like
		 * gpml:conjugatePlateId etc.), and then loaded into the CreateFeaturePropertiesPage responsible
		 * for the "Add / Tweak Conjugate Properties" page of the dialog when transitioning to it
		 * from the "All Properties (for the primary feature)" page. When transitioning away from the
		 * conjugate properties page, any potential edits are sucked out of the widget and back into
		 * this list.
		 *
		 * When we get to the final feature creation stage, the conjugate feature is constructed using
		 * this list in the same manner.
		 */
		CreateFeaturePropertiesPage::property_seq_type d_conjugate_properties;
		
		/**
		 * The last canvas tool explicitly chosen by the user (i.e. not the
		 * result of an automatic switch of canvas tool by GPlates code).
		 *
		 * In particular, this is used to switch back to the Click Geometry tool
		 * after the user clicks "Clone Geometry" and completes the "Create
		 * Feature" dialog; this is because "Clone Geometry" automatically takes
		 * the user to a digitisation tool.
		 */
		boost::optional<
				std::pair<
						GPlatesGui::CanvasToolWorkflows::WorkflowType,
						GPlatesGui::CanvasToolWorkflows::ToolType> >
								d_canvas_tool_last_chosen_by_user;
	};
}

#endif  // GPLATES_QTWIDGETS_CREATEFEATUREDIALOG_H

