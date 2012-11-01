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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <map>
#include <QSet>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>

#include "CreateFeatureDialog.h"

#include "CanvasToolBarDockWidget.h"
#include "ChooseFeatureCollectionWidget.h"
#include "ChooseFeatureTypeWidget.h"
#include "ChoosePropertyWidget.h"
#include "CreateFeaturePropertiesPage.h"
#include "EditPlateIdWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditStringWidget.h"
#include "FlowlinePropertiesWidget.h"
#include "InformationDialog.h"
#include "InvalidPropertyValueException.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/TopologyGeometryType.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/GeometrySetter.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Dialogs.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureType.h"
#include "model/Gpgim.h"
#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/PropertyName.h"
#include "model/types.h"

#include "presentation/ViewState.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	bool
	is_topological_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_LINE =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_NETWORK =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

		const GPlatesPropertyValues::StructuralType property_type =
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value);

		return property_type == GPML_TOPOLOGICAL_LINE ||
			property_type == GPML_TOPOLOGICAL_POLYGON ||
			property_type == GPML_TOPOLOGICAL_NETWORK;
	}


	bool
	is_non_topological_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		static const GPlatesPropertyValues::StructuralType GML_LINE_STRING =
				GPlatesPropertyValues::StructuralType::create_gml("LineString");
		static const GPlatesPropertyValues::StructuralType GML_ORIENTABLE_CURVE =
				GPlatesPropertyValues::StructuralType::create_gml("OrientableCurve");
		static const GPlatesPropertyValues::StructuralType GML_MULTI_POINT =
				GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");
		static const GPlatesPropertyValues::StructuralType GML_POINT =
				GPlatesPropertyValues::StructuralType::create_gml("Point");
		static const GPlatesPropertyValues::StructuralType GML_POLYGON =
				GPlatesPropertyValues::StructuralType::create_gml("Polygon");

		const GPlatesPropertyValues::StructuralType property_type =
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value);

		return property_type == GML_LINE_STRING ||
			property_type == GML_ORIENTABLE_CURVE ||
			property_type == GML_MULTI_POINT ||
			property_type == GML_POINT ||
			property_type == GML_POLYGON;
	}

	bool
	is_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_non_topological_geometry(property_value) ||
			is_topological_geometry(property_value);
	}


	/**
	 * Query the GPGIM to determine whether we should present the user
	 * with a conjugatePlateId property value edit widget.
	 * This is based on FeatureType.
	 */
	bool
	should_offer_conjugate_plate_id_prop(
			const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget,
			const GPlatesModel::Gpgim &gpgim)
	{
		// Get currently selected feature type.
		boost::optional<GPlatesModel::FeatureType> feature_type =
				choose_feature_type_widget->get_feature_type();
		if (!feature_type)
		{
			return false;
		}

		static const GPlatesModel::PropertyName CONJUGATE_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

		// See if the feature type supports a conjugate plate id property.
		return gpgim.get_feature_property(feature_type.get(), CONJUGATE_PLATE_ID_PROPERTY_NAME);
	}

	/**
	 *  Returns whether or not we offer an additional checkbox for creating a conjugate isochron. 
	 *
	 *  Returns true if the selected feature type is "Isochron".  
	 */
	bool
	should_offer_create_conjugate_isochron_checkbox(
		const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget)
	{
		static const GPlatesModel::FeatureType isochron_type = GPlatesModel::FeatureType::create_gpml("Isochron");

		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			choose_feature_type_widget->get_feature_type();
		if (feature_type_opt) {
			return (*feature_type_opt == isochron_type);
		} else {
			return false;
		}
	}	

	/**
	 * Set some default states and/or restrictions on the reconstruction method, depending on the selected feature type.
	 */
	void
	set_recon_method_state(
			QWidget *recon_method_widget,
			QComboBox *recon_method_combo_box,
			const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget,
			const GPlatesModel::Gpgim &gpgim)
	{
		// Get currently selected feature type.
		boost::optional<GPlatesModel::FeatureType> feature_type =
				choose_feature_type_widget->get_feature_type();
		if (!feature_type)
		{
			recon_method_widget->setVisible(false); // Invisible reconstruction method widget.
			recon_method_combo_box->setEnabled(false); // Disable combo box.
			recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			return;
		}

		static const GPlatesModel::PropertyName RECONSTRUCTION_METHOD_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionMethod");

		// See if the feature type supports a reconstruction method property.
		if (!gpgim.get_feature_property(feature_type.get(), RECONSTRUCTION_METHOD_PROPERTY_NAME))
		{
			recon_method_widget->setVisible(false); // Invisible reconstruction method widget.
			recon_method_combo_box->setEnabled(false); // Disable combo box.
			recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			return;
		}

		// Reconstruction method widget is visible (ie, label and combobox are visible).
		recon_method_widget->setVisible(true);

		static const GPlatesModel::FeatureType FLOWLINE_TYPE = GPlatesModel::FeatureType::create_gpml("Flowline");
		static const GPlatesModel::FeatureType MOTION_PATH_TYPE = GPlatesModel::FeatureType::create_gpml("MotionPath");

		// Flowline feature types will be reconstructed as HALF_STAGE_ROTATION.
		if (feature_type.get() == FLOWLINE_TYPE)
		{
			recon_method_combo_box->setEnabled(false); // Prevent user from changing option.
			recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
			return;
		}

		// Motion track types will be set to BY_PLATE_ID.
		// (Later we should allow changing to HALF_STAGE_ROTATION; the MotionPathGeometryPopulator
		// won't currently handle this correctly, so disable this option until we do handle it).
		if (feature_type.get() == MOTION_PATH_TYPE)
		{
			recon_method_combo_box->setEnabled(false); // Prevent user from changing option.
			recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			return;
		}

		static const GPlatesModel::FeatureType MID_OCEAN_RIDGE_TYPE =
				GPlatesModel::FeatureType::create_gpml("MidOceanRidge");

		// Mid-ocean ridge feature types will be reconstructed as HALF_STAGE_ROTATION.
		if (feature_type.get() == MID_OCEAN_RIDGE_TYPE)
		{
			recon_method_combo_box->setEnabled(true); // Allow user to change option.
			recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
			return;
		}

		// Else default to reconstruction by-plate-id.
		recon_method_combo_box->setEnabled(true); // Allow user to change option.
		recon_method_combo_box->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	}


	/**
	 * Handles special case properties appropriate to the feature type selected in 
	 * the @a choose_feature_type_widget.
	 *
	 * Returns boost::none if the feature has no corresponding custom properties widget.
	 *
	 * Currently we only have FlowlinePropertiesWidget - all other feature types will return boost::none.
	 */
	boost::optional<GPlatesQtWidgets::AbstractCustomPropertiesWidget *>
	get_custom_properties_widget(
			const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget,
			const GPlatesAppLogic::ApplicationState &application_state,
			GPlatesQtWidgets::CreateFeatureDialog *create_feature_dialog_ptr)
	{
		static const GPlatesModel::FeatureType flowline_type = GPlatesModel::FeatureType::create_gpml("Flowline");

		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type =
			choose_feature_type_widget->get_feature_type();

		if (feature_type)
		{
			if (feature_type.get() == flowline_type)
			{
				return new GPlatesQtWidgets::FlowlinePropertiesWidget(
						application_state,
						create_feature_dialog_ptr);
			}
		}

		return boost::none;

	}	
}



GPlatesQtWidgets::CreateFeatureDialog::CreateFeatureDialog(
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_gpgim(view_state_.get_application_state().get_gpgim()),
	d_model_ptr(view_state_.get_application_state().get_model_interface()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io()),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_viewport_window_ptr(&viewport_window_),
	d_plate_id_widget(new EditPlateIdWidget(this)),
	d_conjugate_plate_id_widget(new EditPlateIdWidget(this)),
	d_time_period_widget(new EditTimePeriodWidget(this)),
	d_name_widget(new EditStringWidget(this)),
	d_choose_feature_type_widget(
			new ChooseFeatureTypeWidget(
				view_state_.get_application_state().get_gpgim(),
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
				view_state_.get_application_state().get_reconstruct_method_registry(),
				d_file_state,
				d_file_io,
				this)),
	d_recon_method_widget(new QWidget(this)),
	d_recon_method_combobox(new QComboBox(this)),
	d_right_plate_id(new EditPlateIdWidget(this)),
	d_left_plate_id(new EditPlateIdWidget(this)),
	d_create_feature_properties_page(
			new CreateFeaturePropertiesPage(
				view_state_.get_application_state().get_gpgim(),
				view_state_,
				this)),
	d_listwidget_geometry_destinations(
			new ChoosePropertyWidget(
				view_state_.get_application_state().get_gpgim(),
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_recon_method(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID),
	d_create_conjugate_isochron_checkbox(new QCheckBox(this)),
	d_current_page(FEATURE_TYPE_PAGE)
{
	setupUi(this);

	// Add sub-widgets to placeholders.
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_type_widget,
			widget_choose_feature_type_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_listwidget_geometry_destinations,
			listwidget_geometry_destinations_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_create_feature_properties_page,
			widget_feature_properties_page_placeholder);
	
	set_up_button_box();
	
	set_up_feature_type_page();
	set_up_common_properties_page();
	set_up_feature_properties_page();
	set_up_feature_collection_page();
		
	// When the current page is changed, we need to enable and disable some buttons.
	QObject::connect(stack, SIGNAL(currentChanged(int)),
			this, SLOT(handle_page_change(int)));
	// Send a fake page change event to ensure buttons are set up properly at start.
	handle_page_change(0);

	// Handle explicit *user* triggering of a canvas tool action so we can restore the last
	// *user* selected tool once the user completes the "Create Feature" dialog.
	// Note that this excludes automatic canvas tool selection by GPlates.
	QObject::connect(
			&viewport_window_.canvas_tool_bar_dock_widget(),
			SIGNAL(canvas_tool_triggered_by_user(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)),
			this,
			SLOT(handle_canvas_tool_triggered(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_button_box()
{
	// QDialogButtonBox for navigation and feature creation:
	// Default 'OK' button should read 'Create'.
	d_button_create = buttonbox->addButton(tr("Create"), QDialogButtonBox::AcceptRole);
	d_button_create->setDefault(true);
	
	QObject::connect(buttonbox, SIGNAL(accepted()),
			this, SLOT(handle_create()));
	QObject::connect(buttonbox, SIGNAL(rejected()),
			this, SLOT(reject()));
	
	// A few extra buttons for switching between the pages; I set them up outside
	// the QDialogButtonBox to guarantee that "Previous" comes before "Next".
	QObject::connect(button_prev, SIGNAL(clicked()),
			this, SLOT(handle_prev()));
	QObject::connect(button_next, SIGNAL(clicked()),
			this, SLOT(handle_next()));

	QObject::connect(button_create_and_save, SIGNAL(clicked()),
			this, SLOT(handle_create_and_save()));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_type_page()
{
	// Pushing Enter or double-clicking should cause the page to advance.
	QObject::connect(
			d_choose_feature_type_widget,
			SIGNAL(item_activated()),
			this,
			SLOT(handle_next()));

	// If the feature type has changed we may need to reset the custom properties widget.
	QObject::connect(
			d_choose_feature_type_widget, 
			SIGNAL(current_index_changed(boost::optional<GPlatesModel::FeatureType>)),
			this, 
			SLOT(handle_feature_type_changed()));	
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_common_properties_page()
{
	// Pushing Enter or double-clicking a geometric property should cause focus to advance.
	QObject::connect(d_listwidget_geometry_destinations, SIGNAL(item_activated()),
			d_plate_id_widget, SLOT(setFocus()));
	// The various Edit widgets need pass focus along the chain if Enter is pressed.
	QObject::connect(d_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::connect(d_time_period_widget, SIGNAL(enter_pressed()),
			d_name_widget, SLOT(setFocus()));
	QObject::connect(d_name_widget, SIGNAL(enter_pressed()),
			button_next, SLOT(setFocus()));
	QObject::connect(d_recon_method_combobox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(recon_method_changed(int)));
	QObject::connect(d_conjugate_plate_id_widget, SIGNAL(value_changed()),
			this, SLOT(handle_conjugate_value_changed()));
	
	// Reconfigure some accelerator keys that conflict.
	d_plate_id_widget->label()->setText(tr("Plate &ID:"));
	d_conjugate_plate_id_widget->label()->setText(tr("C&onjugate ID:"));
	// Conjugate Plate IDs are optional.
	d_conjugate_plate_id_widget->set_null_value_permitted(true);
	d_conjugate_plate_id_widget->reset_widget_to_default_values();
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);
	
	// Set up checkbox for creating conjugate isochron. 
	d_create_conjugate_isochron_checkbox->setChecked(false);
	d_create_conjugate_isochron_checkbox->setText("Create con&jugate isochron");
	QString tool_tip_string("Create an additional isochron feature using the same geometry, \
							and with plate id and conjugate plate id reversed.");
	d_create_conjugate_isochron_checkbox->setToolTip(tool_tip_string);
	d_create_conjugate_isochron_checkbox->setEnabled(false);

	//Add reconstruction method combobox
	QHBoxLayout *recon_method_layout;
	QLabel * recon_method_label;
	recon_method_label = new QLabel(this);
	d_recon_method_combobox->insertItem(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID, tr("By Plate ID"));
	d_recon_method_combobox->insertItem(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION, tr("Half Stage Rotation"));
	recon_method_layout = new QHBoxLayout(d_recon_method_widget);
	recon_method_layout->setContentsMargins(0, 0, 0, 0);
	recon_method_layout->setSpacing(6);
	QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d_recon_method_combobox->setSizePolicy(sizePolicy1);
	recon_method_label->setText(tr("Reconstruction Method:"));
	recon_method_layout->addWidget(recon_method_label);
	recon_method_layout->addWidget(d_recon_method_combobox);

	// Create the edit widgets we'll need, and add them to the Designer-created widgets.
	QHBoxLayout *plate_id_layout;
	plate_id_layout = new QHBoxLayout;
	plate_id_layout->setSpacing(2);
	plate_id_layout->setMargin(0);
	plate_id_layout->addWidget(d_plate_id_widget);
	plate_id_layout->addWidget(d_conjugate_plate_id_widget);

	//Add right and left plate id widgets
	//these widgets are invisible by default
	QHBoxLayout *right_and_left_plate_id_layout;
	right_and_left_plate_id_layout = new QHBoxLayout;
	right_and_left_plate_id_layout->setSpacing(2);
	right_and_left_plate_id_layout->setMargin(0);
	d_left_plate_id->label()->setText(tr("&Left Plate ID:"));
	d_right_plate_id->label()->setText(tr("&Right Plate ID:"));
	right_and_left_plate_id_layout->addWidget(d_left_plate_id);
	right_and_left_plate_id_layout->addWidget(d_right_plate_id);
	d_left_plate_id->setVisible(false);
	d_right_plate_id->setVisible(false);

	QVBoxLayout *edit_layout;
	edit_layout = new QVBoxLayout;
	edit_layout->addWidget(d_recon_method_widget);
	edit_layout->addItem(plate_id_layout);
	edit_layout->addItem(right_and_left_plate_id_layout);
	edit_layout->addWidget(d_time_period_widget);
	edit_layout->addWidget(d_name_widget);
	edit_layout->addWidget(d_create_conjugate_isochron_checkbox);
	edit_layout->insertStretch(-1);
	groupbox_common_properties->setLayout(edit_layout);
	
	// Note that the geometric properties list must be populated dynamically
	// on page change, see handle_page_change() below.
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_properties_page()
{
	// Adding a new property changes the focus to the "Next" button.
	QObject::connect(d_create_feature_properties_page, SIGNAL(finished()),
			button_next, SLOT(setFocus()));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_collection_page()
{
	// Pushing Enter or double-clicking should be the same as clicking create.
	QObject::connect(d_choose_feature_collection_widget, SIGNAL(item_activated()),
			this, SLOT(handle_create()));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_list()
{
	boost::optional<GPlatesPropertyValues::StructuralType> geometric_property_type;
	// Get the structural type of the geometric property.
	if (d_geometry_property_value)
	{
		geometric_property_type =
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(
						*d_geometry_property_value.get());
	}

	// Populate list of feature types that support the geometric property type.
	// If no geometric property type (not selected by user yet) then select all feature types.
	d_choose_feature_type_widget->populate(geometric_property_type);

	// Default to 'gpml:UnclassifiedFeature' (if it supports the geometric property type).
	static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	d_choose_feature_type_widget->set_feature_type(UNCLASSIFIED_FEATURE_TYPE);
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_geometric_property_list()
{
	// Get the FeatureType the user has selected.
	boost::optional<GPlatesModel::FeatureType> feature_type_opt =
		d_choose_feature_type_widget->get_feature_type();
	if ( ! feature_type_opt) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Get the structural type of the geometric property.
	const GPlatesPropertyValues::StructuralType geometric_property_type =
			GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(
					*d_geometry_property_value.get());

	// Populate the listwidget_geometry_destinations based on what is legal right now.
	d_listwidget_geometry_destinations->populate(feature_type_opt.get(), geometric_property_type);
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_properties()
{
	// Get the FeatureType the user has selected.
	boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			d_choose_feature_type_widget->get_feature_type();
	if (!feature_type_opt)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType feature_type = feature_type_opt.get();

	//
	// Create a list of common feature properties from the data the user entered into the common properties page.
	//

	CreateFeaturePropertiesPage::property_seq_type common_feature_properties;

	try
	{
		// Add a gml:name Property.
		add_common_feature_property_to_list(
				common_feature_properties,
				GPlatesModel::PropertyName::create_gml("name"),
				d_name_widget->create_property_value_from_widget(),
				feature_type);

		// Add a gml:validTime Property.
		add_common_feature_property_to_list(
				common_feature_properties,
				GPlatesModel::PropertyName::create_gml("validTime"),
				d_time_period_widget->create_property_value_from_widget(),
				feature_type);

		// If we are using half stage rotation, add right and left plate id.
		if (GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION == d_recon_method)
		{
			add_common_feature_property_to_list(
					common_feature_properties,
					GPlatesModel::PropertyName::create_gpml("reconstructionMethod"),
					GPlatesPropertyValues::Enumeration::create(
							GPlatesPropertyValues::EnumerationType::create_gpml("ReconstructionMethodEnumeration"),
							"HalfStageRotation"),
					feature_type);

			add_common_feature_property_to_list(
					common_feature_properties,
					GPlatesModel::PropertyName::create_gpml("leftPlate"),
					d_left_plate_id->create_property_value_from_widget(),
					feature_type);
			add_common_feature_property_to_list(
					common_feature_properties,
					GPlatesModel::PropertyName::create_gpml("rightPlate"),
					d_right_plate_id->create_property_value_from_widget(),
					feature_type);
		}
		else // Not using a half-stage rotation...
		{
			// Add a gpml:reconstructionPlateId Property.
			add_common_feature_property_to_list(
					common_feature_properties,
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					d_plate_id_widget->create_property_value_from_widget(),
					feature_type);
		}

		// Add a gpml:conjugatePlateId Property.
		if (!d_conjugate_plate_id_widget->is_null() &&
				should_offer_conjugate_plate_id_prop(d_choose_feature_type_widget, d_gpgim))
		{
			add_common_feature_property_to_list(
					common_feature_properties,
					GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
					d_conjugate_plate_id_widget->create_property_value_from_widget(),
					feature_type);
		}
	}
	catch (const InvalidPropertyValueException &exc)
	{
		QMessageBox::critical(this, tr("Property Value Invalid"),
				tr("A feature property could not be added: %1.").arg(exc.reason()),
				QMessageBox::Ok);
		return;
	}

	// Get the PropertyName the user has selected for geometry to go into.
	boost::optional<GPlatesModel::PropertyName> geometry_property_name_opt =
			d_listwidget_geometry_destinations->get_property_name();
	if (!geometry_property_name_opt)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geometry_property_name = geometry_property_name_opt.get();

	// Set the feature type and initial feature properties.
	// The user can then add more properties supported by the feature type.
	d_create_feature_properties_page->initialise(
			feature_type,
			geometry_property_name,
			common_feature_properties);
}


void
GPlatesQtWidgets::CreateFeatureDialog::add_common_feature_property_to_list(
		CreateFeaturePropertiesPage::property_seq_type &common_feature_properties,
		const GPlatesModel::PropertyName &property_name,
		const GPlatesModel::PropertyValue::non_null_ptr_type &property_value,
		const GPlatesModel::FeatureType &feature_type)
{
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;

	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> top_level_property =
			GPlatesModel::ModelUtils::create_top_level_property(
					property_name,
					property_value,
					d_gpgim,
					feature_type,
					&error_code);
	if (!top_level_property)
	{
		// Not successful in adding property; show error message.
		QMessageBox::warning(this,
				QObject::tr("Failed to create a common feature property."),
				QObject::tr(GPlatesModel::ModelUtils::get_error_message(error_code)) +
						" " + QObject::tr("Property '") +
						convert_qualified_xml_name_to_qstring(property_name) +
						QObject::tr("' will not be added to the feature."),
				QMessageBox::Ok);
		return;
	}

	common_feature_properties.push_back(top_level_property.get());
}


bool
GPlatesQtWidgets::CreateFeatureDialog::set_geometry_and_display(
		const GPlatesModel::PropertyValue::non_null_ptr_type &geometry_property_value)
{
	// Make sure the property value is a geometric property type.
	if (!is_geometry(*geometry_property_value))
	{
		QMessageBox::critical(this, tr("No geometry"),
			tr("No geometry was supplied to this dialog. Please try digitising again."));
		// FIXME: Exception.
		return false;
    }

	d_geometry_property_value = geometry_property_value;

	return display();
}


bool
GPlatesQtWidgets::CreateFeatureDialog::display()
{
	// Populate the d_choose_feature_type_widget based on what features support
	// the geometric property type.
	//
	// NOTE: If the dialog was last left on the first page (by the user) then just selecting
	// the first page will not result in a page change and hence no event.
	// So we need to explicitly set up the feature type list (normally done in the first page
	// (the feature type page).
	set_up_feature_list();

	// Set the stack back to the first page.
	stack->setCurrentIndex(FEATURE_TYPE_PAGE);

	// The Feature Collections list needs to be repopulated each time.
	d_choose_feature_collection_widget->initialise();
	
	// Show the dialog modally.
	return exec() == QDialog::Accepted;
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_prev()
{
	int prev_index = stack->currentIndex() - 1;
	if (prev_index < stack->count()) {
		stack->setCurrentIndex(prev_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_next()
{
	if (stack->currentIndex() == COMMON_PROPERTIES_PAGE)
	{
		// If the start-end time are not valid, do not change page.
		if (!d_time_period_widget->valid())
		{
			QMessageBox::warning(this, tr("Time Period Invalid"),
				tr("The begin-end time is not valid."),
				QMessageBox::Ok);
			return;
		}
	}
	else if (stack->currentIndex() == ALL_PROPERTIES_PAGE)
	{
		// If there are required feature properties the user has not yet added, do not change page.
		if (!d_create_feature_properties_page->is_finished())
		{
			QMessageBox::warning(this,
				tr("Feature is missing required properties"),
				tr("Please add properties that are required for the feature type.\n"
					"These are properties that have a minimum multiplicity of one."),
				QMessageBox::Ok);
			return;
		}
	}

	int next_index = stack->currentIndex() + 1;
	if (next_index < stack->count()) {
		stack->setCurrentIndex(next_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_page_change(
		int page)
{
	// Enable all buttons and then disable buttons appropriately.
	button_prev->setEnabled(true);
	button_next->setEnabled(true);
	button_create_and_save->setEnabled(true);
	d_button_create->setEnabled(true);
	
	// Disable buttons which are not valid for the page,
	// and focus the first widget.
	switch (page)
	{
	case FEATURE_TYPE_PAGE:
		// Populate the d_choose_feature_type_widget based on what features support
		// the geometric property type.
		set_up_feature_list();
		d_choose_feature_type_widget->setFocus();
		button_prev->setEnabled(false);
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		break;

	case COMMON_PROPERTIES_PAGE:
		// Populate the listwidget_geometry_destinations based on what is legal right now.
		set_up_geometric_property_list();
		d_listwidget_geometry_destinations->setFocus();
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		// Make sure it's null (or "None") because it's accessed even when it's not visible.
		d_conjugate_plate_id_widget->set_null(true);
		d_conjugate_plate_id_widget->setVisible(
			d_recon_method_combobox->currentIndex() == 0 /* plate id */ &&
				should_offer_conjugate_plate_id_prop(d_choose_feature_type_widget, d_gpgim));
		// Make sure it's unchecked because it's accessed programmatically even when it's not visible.
		d_create_conjugate_isochron_checkbox->setChecked(false);
		d_create_conjugate_isochron_checkbox->setVisible(
			// Currently only allow user to select if there's a non-topological geometry because creating
			// conjugate requires reverse reconstructing using non-topological reconstruction...
			is_non_topological_geometry(*d_geometry_property_value.get()) &&
				should_offer_create_conjugate_isochron_checkbox(d_choose_feature_type_widget));
		set_recon_method_state(
			d_recon_method_widget,
			d_recon_method_combobox,
			d_choose_feature_type_widget,
			d_gpgim);
		break;

	case ALL_PROPERTIES_PAGE:
		// Create the common properties and list them in the feature properties page.
		// We only need to do this if the user is advancing from a prior page.
		// If the user pressed the "previous" button (from the feature collection page) then we just
		// keep the existing feature properties they have already set up so they don't lose them.
		if (d_current_page <= ALL_PROPERTIES_PAGE)
		{
			set_up_feature_properties();
		}
		d_create_feature_properties_page->setFocus();
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		break;

	case FEATURE_COLLECTION_PAGE:
		d_choose_feature_collection_widget->setFocus();
		button_next->setEnabled(false);
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Update the current page (helps determine page transitions).
	d_current_page = static_cast<StackedWidgetPage>(page);
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
{
	// Get the PropertyName the user has selected for geometry to go into.
	boost::optional<GPlatesModel::PropertyName> geometry_property_name_item =
		d_listwidget_geometry_destinations->get_property_name();
	if (!geometry_property_name_item)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geometry_property_name = *geometry_property_name_item;
	
	// Get the FeatureType the user has selected.
	boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			d_choose_feature_type_widget->get_feature_type();
	if (!feature_type_opt)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType feature_type = *feature_type_opt;

	try
	{
		// Create a feature with no properties (yet).
		GPlatesModel::FeatureHandle::non_null_ptr_type feature =
				GPlatesModel::FeatureHandle::create(feature_type);

		// Add all feature properties (including the common properties).
		CreateFeaturePropertiesPage::property_seq_type feature_properties;
		d_create_feature_properties_page->get_feature_properties(feature_properties);
		BOOST_FOREACH(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property,
				feature_properties)
		{
			feature->add(feature_property);
		}

		// Add the (reconstruction-time) geometry property to the feature.
		const boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property_iterator =
				add_geometry_property(feature->reference(), geometry_property_name);
		if (!geometry_property_iterator)
		{
			reject();
			return;
		}

		// Reverse-reconstruct the geometry (just added) back to present day.
		// This does nothing for topological geometries since they reference another feature's geometry.
		if (!reverse_reconstruct_geometry_property(feature->reference(), geometry_property_iterator.get()))
		{
			reject();
			return;
		}

		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool>
				feature_collection_file_iter = d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				(feature_collection_file_iter.first).get_file().get_feature_collection();

		// We want to merge model events across this scope so that only one model event
		// is generated instead of multiple.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state_ptr->get_model_interface().access_model());

		// Add the feature to the feature collection.
		feature_collection->add(feature);

		// If the feature is an isochron and the user wants to create the conjugate...
		if (d_create_conjugate_isochron_checkbox->isChecked())
		{
			// Should only get here for non-topological geometries (which can be reverse-reconstructed).
			create_conjugate_isochron(
					feature_collection,
					feature->reference(),
					geometry_property_iterator.get());
		}
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before we emit signals because we don't
		// know whose listening on those signals and they may be expecting model observers to
		// be up-to-date with the modified model.
		model_notification_guard.release_guard();

		emit feature_created(feature->reference());

		// If the user got into digitisation mode because they clicked the
		// "Clone Geometry" button whilst in the Click Geometry tool, for
		// example, they get taken back to the Click Geometry tool instead of
		// remaining in a digitisation tool.
		if (d_canvas_tool_last_chosen_by_user)
		{
			d_viewport_window_ptr->canvas_tool_workflows().choose_canvas_tool(
					d_canvas_tool_last_chosen_by_user->first,
					d_canvas_tool_last_chosen_by_user->second);
		}

		accept();
	}
	catch (const ChooseFeatureCollectionWidget::NoFeatureCollectionSelectedException &)
	{
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
	catch (const InvalidPropertyValueException &exc)
	{
		QMessageBox::critical(this, tr("Property Value Invalid"),
				tr("A feature property could not be added: %1.").arg(exc.reason()),
				QMessageBox::Ok);
		return;
	}
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_create_and_save()
{
	// do the regular creation process
	handle_create();

	// and now open the manage feature collections dialog
	d_viewport_window_ptr->dialogs().pop_up_manage_feature_collections_dialog();
}

void 
GPlatesQtWidgets::CreateFeatureDialog::recon_method_changed(int index)
{
	switch (index)
	{
		case GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION:
			d_plate_id_widget->setVisible(false);
			d_conjugate_plate_id_widget->setVisible(false);
			d_right_plate_id->setVisible(true);
			d_left_plate_id->setVisible(true);
			d_recon_method = GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION;
			break;
		
		case GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID:
			d_right_plate_id->setVisible(false);
			d_left_plate_id->setVisible(false);
			d_plate_id_widget->setVisible(true);
			d_conjugate_plate_id_widget->setVisible(
					should_offer_conjugate_plate_id_prop(d_choose_feature_type_widget, d_gpgim));
			d_recon_method = GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID;
			break;
	}
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_conjugate_value_changed()
{
	d_create_conjugate_isochron_checkbox->setEnabled(!d_conjugate_plate_id_widget->is_null());
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_feature_type_changed()
{
	if (d_custom_properties_widget)
	{
		delete d_custom_properties_widget.get();
		d_custom_properties_widget = boost::none;
	}

	// See if the current feature type needs a custom properties widget.
	boost::optional<GPlatesQtWidgets::AbstractCustomPropertiesWidget *> custom_properties_widget =
			get_custom_properties_widget(
					d_choose_feature_type_widget,
					*d_application_state_ptr,
					this);
	if (custom_properties_widget)
	{
		d_custom_properties_widget = custom_properties_widget.get();
		set_up_custom_properties_page();
	}
}

void
GPlatesQtWidgets::CreateFeatureDialog::set_up_custom_properties_page()
{
	if (!d_custom_properties_widget)
	{
		return;
	}

	// If a layout already exists, get rid of it. 
	QLayout *layout_ = widget_custom_geometry_placeholder->layout();
	if (layout_)
	{
		delete (layout_);
	}

	// Create the edit widgets we'll need, and add them to the Designer-created widgets.
	QGridLayout *custom_layout;
	custom_layout = new QGridLayout;
	custom_layout->setSpacing(0);
	custom_layout->setContentsMargins(0, 0, 0, 0);
	custom_layout->addWidget(d_custom_properties_widget.get());
	widget_custom_geometry_placeholder->setLayout(custom_layout);
}


boost::optional<GPlatesModel::FeatureHandle::iterator>
GPlatesQtWidgets::CreateFeatureDialog::add_geometry_property(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::PropertyName &geometry_property_name)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_geometry_property_value,
			GPLATES_ASSERTION_SOURCE);

	GPlatesModel::PropertyValue::non_null_ptr_type geometry_property_value =
			d_geometry_property_value.get();

	// Handle any custom geometry processing.
    if (d_custom_properties_widget)
    {
		// Get the geometry from the property value.
		// This only works for non-topological geometry properties.
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> reconstruction_time_geometry =
				GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(
						*geometry_property_value);
		if (reconstruction_time_geometry)
		{
			reconstruction_time_geometry.reset(
				d_custom_properties_widget.get()->do_geometry_tasks(
						reconstruction_time_geometry.get(),
						feature));

			// Wrap the modified geometry back up in a property value.
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_property_value_opt =
					GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
							reconstruction_time_geometry.get());
			if (!geometry_property_value_opt)
			{
				QMessageBox::critical(this, tr("Cannot convert geometry to property value"),
					tr("There was an error converting the digitised geometry to a usable property value."));
				reject();
				return boost::none;
			}
			geometry_property_value = geometry_property_value_opt.get();
		}
    }

	// Add the topological geometry property to the feature.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> geometry_property =
			GPlatesModel::ModelUtils::create_top_level_property(
					geometry_property_name,
					geometry_property_value,
					d_gpgim,
					feature->feature_type(),
					&add_property_error_code);
	if (!geometry_property)
	{
		// Not successful in adding geometry; show error message.
		static const char *const TOPOLOGY_ERROR_MESSAGE_APPEND = QT_TR_NOOP("Please try building topology again.");
		static const char *const NON_TOPOLOGY_ERROR_MESSAGE_APPEND = QT_TR_NOOP("Please try digitising geometry again.");

		QMessageBox::warning(this,
				tr("Failed to add geometry property to new feature."),
				tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)) + " " +
						tr(is_topological_geometry(*geometry_property_value)
								? TOPOLOGY_ERROR_MESSAGE_APPEND : NON_TOPOLOGY_ERROR_MESSAGE_APPEND),
				QMessageBox::Ok);
		return boost::none;
	}

	return feature->add(geometry_property.get());
}


bool
GPlatesQtWidgets::CreateFeatureDialog::reverse_reconstruct_geometry_property(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator)
{
	if (!geometry_property_iterator.is_still_valid())
	{
		return false;
	}

	// Get the geometry property value from the geometry property iterator.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type get_property_value_error_code;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> geometry_property_value =
			GPlatesModel::ModelUtils::get_property_value(
					**geometry_property_iterator,
					&get_property_value_error_code);
	if (!geometry_property_value)
	{
		QMessageBox::critical(this,
				tr("Failed to access geometry property."),
				tr(GPlatesModel::ModelUtils::get_error_message(get_property_value_error_code)),
				QMessageBox::Ok);
		return false;
	}

	// Topological geometries cannot be reverse reconstructed so return early.
	if (is_topological_geometry(*geometry_property_value.get()))
	{
		// This is not an error condition because a topological geometry
		// does not need a present day geometry.
		return true;
	}

	// Get the geometry from the property value.
    boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> reconstructed_geometry =
			GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(
					*geometry_property_value.get());
	if (!reconstructed_geometry)
	{
		// Should never happen.
		QMessageBox::critical(this, tr("Cannot convert property value to geometry"),
			tr("There was an error retrieving the digitised geometry from its property value."));
		// FIXME: Exception.
		return false;
	}

    // Un-Reconstruct the temporary geometry so that it's coordinates are
    // expressed in terms of present day location, given the plate ID that is associated
    // with it and the current reconstruction time.
    //
    // FIXME: Currently we can have multiple reconstruction tree visual layers but we
    // only allow one active at a time - when this changes we'll need to somehow figure out
    // which reconstruction tree to use here.
    // We could search the layers for the one that reconstructs the feature collection that
    // will contain the new feature and see which reconstruction tree that layer uses in turn.
    // This could fail if the containing feature collection is reconstructed by multiple layers
    // (for example if the user wants to reconstruct the same features using two different
    // reconstruction trees). We could detect this case and ask the user which reconstruction tree
    // to use (for reverse reconstructing).
    // This can also fail if the user is adding the created feature to a new feature collection
    // in which case we cannot know which reconstruction tree they will choose when they wrap
    // the new feature collection in a new layer. Although when the new feature collection is
    // created it will automatically create a new layer and set the "default" reconstruction tree
    // layer as its input (where 'default' will probably be the most recently created
    // reconstruction tree layer that is currently active). In this case we could figure out
    // which reconstruction tree layer this is going to be. But this is not ideal because the
    // user may then immediately switch to a different reconstruction tree input layer and our
    // reverse reconstruction will not be the one they wanted.
    // Perhaps the safest solution here is to again ask the user which reconstruction tree layer
    // to use and then use that instead of the 'default' when creating a new layer for the new
    // feature collection.
    // So in summary:
    // * if adding feature to an existing feature collection:
    //   * if feature collection is being processed by only one layer then reverse reconstruct
    //     using the reconstruction tree used by that layer,
    //   * if feature collection is being processed by more than one layer then gather the
    //     reconstruction trees used by those layers and ask user which one to
    //     reverse reconstruct with,
    // * if adding feature to a new feature collection gather all reconstruction tree layers
    //   including inactive ones and ask user which one to use for the new layer that will
    //   wrap the new feature collection.
    //

	// The default reconstruction tree.
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type default_reconstruction_tree =
			d_application_state_ptr->get_current_reconstruction()
					.get_default_reconstruction_layer_output()->get_reconstruction_tree();

	// Use the feature properties added so far to the new feature to determine how to
	// reconstruct the geometry back to present-day.
	// This takes advantage of the reconstruct-method framework and avoids a bunch of if-else
	// statements here.
	//
	// NOTE: The feature must have a geometry property present (even if it's not the correct present
	// day geometry) because some reconstruct methods will only be chosen if a geometry is present.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
			GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
					reconstructed_geometry.get(),
					feature,
					*default_reconstruction_tree,
					true/*reverse_reconstruct*/);

	// Store the geometry property value back into the geometry property (in the feature).
	GPlatesFeatureVisitors::GeometrySetter geometry_setter(present_day_geometry);
	GPlatesModel::TopLevelProperty::non_null_ptr_type geometry_property_clone =
			(*geometry_property_iterator)->deep_clone();
	geometry_setter.set_geometry(geometry_property_clone.get());
	*geometry_property_iterator = geometry_property_clone;

	return true;
}

void
GPlatesQtWidgets::CreateFeatureDialog::create_conjugate_isochron(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const GPlatesModel::FeatureHandle::weak_ref &isochron_feature,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator)
{
	if (!geometry_property_iterator.is_still_valid())
	{
		return;
	}

	static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	static const GPlatesModel::PropertyName CONJUGATE_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("conjugatePlateId");
	static const GPlatesModel::PropertyName NAME =
			GPlatesModel::PropertyName::create_gml("name");

	// Get the reconstruction plate id from the isochron feature.
	const GPlatesPropertyValues::GpmlPlateId *reconstruction_plate_id = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		isochron_feature,
		RECONSTRUCTION_PLATE_ID,
		reconstruction_plate_id))
	{
		QMessageBox::critical(this,
				tr("Failed to access isochron property."),
				tr("Unable to access 'gpml:reconstructionPlateId' property in isochron feature."),
				QMessageBox::Ok);
		return;
	}

	// Get the conjugate plate id from the isochron feature.
	const GPlatesPropertyValues::GpmlPlateId *conjugate_plate_id = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		isochron_feature,
		CONJUGATE_PLATE_ID,
		conjugate_plate_id))
	{
		QMessageBox::critical(this,
				tr("Failed to access isochron property."),
				tr("Unable to access 'gpml:conjugatePlateId' property in isochron feature."),
				QMessageBox::Ok);
		return;
	}

	// Get the name from the isochron feature.
	const GPlatesPropertyValues::XsString *name = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		isochron_feature,
		NAME,
		name))
	{
		QMessageBox::critical(this,
				tr("Failed to access isochron property."),
				tr("Unable to access 'gml:name' property in isochron feature."),
				QMessageBox::Ok);
		return;
	}

	// Create the conjugate isochron feature.
	GPlatesModel::FeatureHandle::non_null_ptr_type conjugate_isochron_feature =
			GPlatesModel::FeatureHandle::create(
					GPlatesModel::FeatureType::create_gpml("Isochron"));

	// Iterate over the isochron properties and create the conjugate isochron properties.
	GPlatesModel::FeatureHandle::iterator isochron_properties_iter = isochron_feature->begin();
	GPlatesModel::FeatureHandle::iterator isochron_properties_end = isochron_feature->end();
	for ( ; isochron_properties_iter != isochron_properties_end; ++isochron_properties_iter)
	{
		// Ignore the geometry property - we're going to add the conjugate geometry later.
		if (isochron_properties_iter == geometry_property_iterator)
		{
			continue;
		}

		const GPlatesModel::PropertyName &property_name = (*isochron_properties_iter)->property_name();

		if (property_name == RECONSTRUCTION_PLATE_ID)
		{
			// Swap the reconstruction and conjugate plate ids.
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
			if (!GPlatesModel::ModelUtils::add_property(
					conjugate_isochron_feature->reference(),
					property_name,
					conjugate_plate_id->deep_clone_as_prop_val(),
					d_gpgim,
					true/*check_property_name_allowed_for_feature_type*/,
					&add_property_error_code))
			{
				// Not successful in adding geometry; show error message.
				QMessageBox::warning(this,
						QObject::tr("Failed to add property to the conjugate isochron."),
						QObject::tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)),
						QMessageBox::Ok);
				return;
			}

			continue;
		}

		if (property_name == CONJUGATE_PLATE_ID)
		{
			// Swap the reconstruction and conjugate plate ids.
			GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
			if (!GPlatesModel::ModelUtils::add_property(
					conjugate_isochron_feature->reference(),
					property_name,
					reconstruction_plate_id->deep_clone_as_prop_val(),
					d_gpgim,
					true/*check_property_name_allowed_for_feature_type*/,
					&add_property_error_code))
			{
				// Not successful in adding geometry; show error message.
				QMessageBox::warning(this,
						QObject::tr("Failed to add property to the conjugate isochron."),
						QObject::tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)),
						QMessageBox::Ok);
				return;
			}

			continue;
		}

		if (property_name == NAME)
		{
			// Change the "gml:name" property.
			// FIXME: we should give the user the chance to enter a new name.
			QString conjugate_name_string =
					GPlatesUtils::make_qstring_from_icu_string(name->value().get());
			conjugate_name_string.prepend("Conjugate of ");
			GPlatesModel::PropertyValue::non_null_ptr_type conjugate_name_property_value =
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::make_icu_string_from_qstring(conjugate_name_string));

			GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
			if (!GPlatesModel::ModelUtils::add_property(
					conjugate_isochron_feature->reference(),
					property_name,
					conjugate_name_property_value,
					d_gpgim,
					true/*check_property_name_allowed_for_feature_type*/,
					&add_property_error_code))
			{
				// Not successful in adding geometry; show error message.
				QMessageBox::warning(this,
						QObject::tr("Failed to add property to the conjugate isochron."),
						QObject::tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)),
						QMessageBox::Ok);
				return;
			}

			continue;
		}

		// Clone and add the current property to the conjugate isochron feature.
		GPlatesModel::TopLevelProperty::non_null_ptr_type property_clone =
				(*isochron_properties_iter)->deep_clone();
		conjugate_isochron_feature->add(property_clone);
	}

	//
	// Create the conjugate isochron's geometry property.
	//

	// Get the geometry property value from the geometry property iterator.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type get_property_value_error_code;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> geometry_property_value =
			GPlatesModel::ModelUtils::get_property_value(
					**geometry_property_iterator,
					&get_property_value_error_code);
	if (!geometry_property_value)
	{
		QMessageBox::critical(this,
				tr("Failed to access isochron geometry property."),
				tr(GPlatesModel::ModelUtils::get_error_message(get_property_value_error_code)),
				QMessageBox::Ok);
		return;
	}

	// Get the present-day geometry from the property value.
    boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> present_day_geometry =
			GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(
					*geometry_property_value.get());
	if (!present_day_geometry)
	{
		// Should never happen.
		QMessageBox::critical(this, tr("Cannot convert property value to geometry"),
			tr("There was an error retrieving the present day geometry from its property value."));
		// FIXME: Exception.
		return;
	}

	// The default reconstruction tree.
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type default_reconstruction_tree =
			d_application_state_ptr->get_current_reconstruction()
					.get_default_reconstruction_layer_output()->get_reconstruction_tree();

	// Use the isochron feature properties (which should all be added by now) to reconstruct the
	// isochron's present-day geometry to the current reconstruction time.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type reconstructed_geometry =
			GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
					present_day_geometry.get(),
					isochron_feature,
					*default_reconstruction_tree);

	// Reverse reconstruct, using the *conjugate* plate id, back to present-day.
	// Note that we're reversing the plate-id and conjugate-plate-ids, so we use the conjugate here.
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type conjugate_present_day_geometry =
			GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
					reconstructed_geometry,
					conjugate_plate_id->value(),
					*default_reconstruction_tree,
					true /*reverse_reconstruct*/);

	// Create a property value using the present-day geometry for the conjugate isochron.
	const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> conjugate_geometry_property_value =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
					conjugate_present_day_geometry);
	if (!conjugate_geometry_property_value)
	{
		return;
	}

	// Add the geometry property to the conjugate isochron feature.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
	if (!GPlatesModel::ModelUtils::add_property(
			conjugate_isochron_feature->reference(),
			(*geometry_property_iterator)->property_name(),
			conjugate_geometry_property_value.get(),
			d_gpgim,
			true/*check_property_name_allowed_for_feature_type*/,
			&add_property_error_code))
	{
		// Not successful in adding geometry; show error message.
		QMessageBox::warning(this,
				QObject::tr("Failed to add geometry property to the conjugate isochron."),
				QObject::tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)),
				QMessageBox::Ok);
		return;
	}

	// Add the conjugate isochron feature to the feature collection.
	feature_collection->add(conjugate_isochron_feature);
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_canvas_tool_triggered(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	d_canvas_tool_last_chosen_by_user = std::make_pair(workflow, tool);
}
