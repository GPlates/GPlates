/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010, 2015 The University of Sydney, Australia
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
#include "ResizeToContentsTextEdit.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/ReconstructLayerProxy.h"
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
#include "model/GpgimFeatureClass.h"
#include "model/GpgimProperty.h"
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
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/StructuralType.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"

namespace
{
	bool
	is_topological_line(
			const GPlatesPropertyValues::StructuralType &property_type)
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_LINE =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");

		return property_type == GPML_TOPOLOGICAL_LINE;
	}

	bool
	is_topological_line(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_topological_line(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	bool
	is_topological_polygon(
			const GPlatesPropertyValues::StructuralType &property_type)
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");

		return property_type == GPML_TOPOLOGICAL_POLYGON;
	}

	bool
	is_topological_polygon(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_topological_polygon(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	bool
	is_topological_network(
			const GPlatesPropertyValues::StructuralType &property_type)
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_NETWORK =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

		return property_type == GPML_TOPOLOGICAL_NETWORK;
	}

	bool
	is_topological_network(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_topological_network(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	bool
	is_topological_geometry(
			const GPlatesPropertyValues::StructuralType &property_type)
	{
		return is_topological_line(property_type) ||
				is_topological_polygon(property_type) ||
				is_topological_network(property_type);
	}

	bool
	is_topological_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_topological_geometry(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	bool
	is_non_topological_geometry(
			const GPlatesPropertyValues::StructuralType &property_type)
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

		return property_type == GML_LINE_STRING ||
			property_type == GML_ORIENTABLE_CURVE ||
			property_type == GML_MULTI_POINT ||
			property_type == GML_POINT ||
			property_type == GML_POLYGON;
	}

	bool
	is_non_topological_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_non_topological_geometry(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	bool
	is_geometry(
			const GPlatesPropertyValues::StructuralType &property_type)
	{
		return is_non_topological_geometry(property_type) ||
			is_topological_geometry(property_type);
	}

	bool
	is_geometry(
			const GPlatesModel::PropertyValue &property_value)
	{
		return is_geometry(
				GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(property_value));
	}


	/**
	 * Query the GPGIM to determine whether we can add the 'gpml:geometryImportTime' property.
	 * This is based on FeatureType.
	 */
	bool
	should_add_import_geometry_time_prop(
			const boost::optional<GPlatesModel::FeatureType> &feature_type)
	{
		if (!feature_type)
		{
			return false;
		}

		static const GPlatesModel::PropertyName GEOMETRY_IMPORT_TIME_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("geometryImportTime");

		// See if the feature type supports a geometry import time property.
		return GPlatesModel::Gpgim::instance().get_feature_property(
				feature_type.get(),
				GEOMETRY_IMPORT_TIME_PROPERTY_NAME);
	}


	/**
	 * Query the GPGIM to determine whether we should present the user
	 * with a conjugatePlateId property value edit widget.
	 * This is based on FeatureType.
	 */
	bool
	should_offer_conjugate_plate_id_prop(
			const boost::optional<GPlatesModel::FeatureType> &feature_type)
	{
		if (!feature_type)
		{
			return false;
		}

		static const GPlatesModel::PropertyName CONJUGATE_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

		// See if the feature type supports a conjugate plate id property.
		return GPlatesModel::Gpgim::instance().get_feature_property(
				feature_type.get(),
				CONJUGATE_PLATE_ID_PROPERTY_NAME);
	}

	/**
	 * Query the GPGIM to determine whether we should link conjugate features together using
	 * the gpml:conjugate property.
	 * This is based on FeatureType.
	 */
	bool
	should_link_with_conjugate_prop(
			const boost::optional<GPlatesModel::FeatureType> &feature_type)
	{
		if ( ! feature_type)
		{
			return false;
		}

		static const GPlatesModel::PropertyName CONJUGATE_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("conjugate");

		// See if the feature type supports a 'conjugate' reference property.
		return GPlatesModel::Gpgim::instance().get_feature_property(feature_type.get(), CONJUGATE_PROPERTY_NAME);
	}
	
	/**
	 *  Returns whether or not we offer an additional checkbox for creating a conjugate feature.
	 *
	 *  Returns true if the selected feature type supports a conjugate plate id according
	 *  to the GPGIM (such as an "Isochron" feature).
	 */
	bool
	should_offer_create_conjugate_feature_checkbox(
			const boost::optional<GPlatesModel::FeatureType> &feature_type,
			const boost::optional<GPlatesPropertyValues::StructuralType> &geometry_property_value)
	{
		// Currently only allow user to select if there's a non-topological geometry because creating
		// conjugate requires reverse reconstructing using non-topological reconstruction...
		return geometry_property_value &&
		       is_non_topological_geometry(geometry_property_value.get()) &&
		       should_offer_conjugate_plate_id_prop(feature_type);
	}	

	/**
	 *  Returns whether or not we offer an additional page for adjusting conjugate feature properties.
	 *
	 *  Returns true if:-
	 *   * the selected feature type supports a conjugate plate id according to the GPGIM
	 *   * it has non-topological geometry
	 *   * we have displayed the create conjugate feature checkbox on page 1
	 *   * said checkbox is checked.
	 *
	 *  Note that it is insufficient to simply query the checkedness of the checkbox, as it is
	 *  capable of remaining checked even while invisible.
	 */
	bool
	should_offer_conjugate_properties_page(
			const boost::optional<GPlatesModel::FeatureType> &feature_type,
			const boost::optional<GPlatesPropertyValues::StructuralType> &geometry_property_value,
			const QCheckBox &create_conjugate_checkbox)
	{
		// Currently only allow user to select if there's a non-topological geometry because creating
		// conjugate requires reverse reconstructing using non-topological reconstruction...
		return create_conjugate_checkbox.isEnabled() && create_conjugate_checkbox.isChecked() &&
		       should_offer_create_conjugate_feature_checkbox(feature_type, geometry_property_value);
	}	

	/**
	 * Query the GPGIM to determine whether we should present the user
	 * with a relativePlate property value edit widget.
	 * This is based on FeatureType.
	 */
	bool
	should_offer_relative_plate_id_prop(
			const boost::optional<GPlatesModel::FeatureType> &feature_type)
	{
		// Get currently selected feature type.
		if (!feature_type)
		{
			return false;
		}

		// Exclude unclassified feature type because it supports *all* properties and, since
		// we don't set the relative plate id widget to null (like conjugate plate id), then
		// it will add the default relative plate id zero as a property value which we don't want.
		static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		if (feature_type.get() == UNCLASSIFIED_FEATURE_TYPE)
		{
			return false;
		}

		static const GPlatesModel::PropertyName RELATIVE_PLATE_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("relativePlate");

		// See if the feature type supports a relative plate id property.
		return GPlatesModel::Gpgim::instance().get_feature_property(
				feature_type.get(),
				RELATIVE_PLATE_PROPERTY_NAME);
	}

	/**
	 * Finds a property value in the sequence of top-level properties that has the specified
	 * property name and type, and returns a clone of it (so it can be modified).
	 */
	template <class PropertyValueType>
	boost::optional<typename PropertyValueType::non_null_ptr_type>
	find_property_value(
			const GPlatesModel::PropertyName &property_name,
			const GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type &feature_properties)
	{
		GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type::const_iterator
				feature_properties_iter = feature_properties.begin();
		GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type::const_iterator
				feature_properties_end = feature_properties.end();
		for ( ; feature_properties_iter != feature_properties_end; ++feature_properties_iter)
		{
			const GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *feature_properties_iter;

			if (feature_property->property_name() != property_name)
			{
				continue;
			}

			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> property_value =
					GPlatesModel::ModelUtils::get_property_value(*feature_property);
			if (!property_value)
			{
				continue;
			}

			boost::optional<typename PropertyValueType::non_null_ptr_to_const_type> derived_property_value =
					GPlatesFeatureVisitors::get_property_value<PropertyValueType>(*property_value.get());
			if (!derived_property_value)
			{
				continue;
			}

			return derived_property_value.get()->deep_clone();
		}

		return boost::none;
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
			const boost::optional<GPlatesModel::FeatureType> &feature_type,
			const GPlatesAppLogic::ApplicationState &application_state,
			GPlatesQtWidgets::CreateFeatureDialog *create_feature_dialog_ptr)
	{
		static const GPlatesModel::FeatureType flowline_type = GPlatesModel::FeatureType::create_gpml("Flowline");

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
	d_model_ptr(view_state_.get_application_state().get_model_interface()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io()),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_viewport_window_ptr(&viewport_window_),
	d_plate_id_widget(new EditPlateIdWidget(this)),
	d_conjugate_plate_id_widget(new EditPlateIdWidget(this)),
	d_relative_plate_id_widget(new EditPlateIdWidget(this)),
	d_time_period_widget(new EditTimePeriodWidget(this)),
	d_name_widget(new EditStringWidget(this)),
	d_choose_feature_type_widget(
			new ChooseFeatureTypeWidget(
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_feature_type_description_widget(
			new ResizeToContentsTextEdit(
					convert_qualified_xml_name_to_qstring(
							GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature")),
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
				view_state_,
				this)),
    d_create_conjugate_properties_page(
			new CreateFeaturePropertiesPage(
				view_state_,
				this)),
	d_listwidget_geometry_destinations(
			new ChoosePropertyWidget(
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_recon_method(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID),
	d_create_conjugate_feature_checkbox(new QCheckBox(this)),
	d_current_page(FEATURE_TYPE_PAGE)
{
	setupUi(this);

	// Add sub-widgets to placeholders.
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_type_widget,
			widget_choose_feature_type_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_feature_type_description_widget,
			widget_description_feature_type_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_listwidget_geometry_destinations,
			listwidget_geometry_destinations_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_create_feature_properties_page,
			widget_feature_properties_page_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_create_conjugate_properties_page,
			widget_conjugate_properties_page_placeholder);
	
	set_up_button_box();
	
	set_up_feature_type_page();
	set_up_common_properties_page();
	set_up_feature_properties_page();
	set_up_conjugate_properties_page();
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
	
	// These buttons desperately need some icons.
	d_button_create->setIcon(QIcon(":/gnome_emblem_new_16.png"));
	buttonbox->button(QDialogButtonBox::Cancel)->setIcon(QIcon(":/gnome_window_close_16.png"));
	
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

	// Set up the feature type description QTextEdit.
	d_feature_type_description_widget->setReadOnly(true);
	d_feature_type_description_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed/*Use sizeHint() since we've overridden it*/));
	d_feature_type_description_widget->setLineWrapMode(QTextEdit::WidgetWidth);
	d_feature_type_description_widget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	// Limit the maximum height of the feature type description so it doesn't push the dialog off the screen.
	d_feature_type_description_widget->setMaximumHeight(100);
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_common_properties_page()
{
	// Pushing Enter or double-clicking a geometric property should cause focus to advance.
	// Advance to something we know will always be visible.
	QObject::connect(d_listwidget_geometry_destinations, SIGNAL(item_activated()),
			d_time_period_widget, SLOT(setFocus()));
	// The various Edit widgets need pass focus along the chain if Enter is pressed.
	QObject::connect(d_time_period_widget, SIGNAL(enter_pressed()),
			d_name_widget, SLOT(setFocus()));
	QObject::connect(d_name_widget, SIGNAL(enter_pressed()),
			button_next, SLOT(setFocus()));
	QObject::connect(d_recon_method_combobox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(recon_method_changed(int)));
	QObject::connect(d_conjugate_plate_id_widget, SIGNAL(value_changed()),
			this, SLOT(handle_conjugate_plate_id_changed()));
	
	// Reconfigure some accelerator keys that conflict.
	d_plate_id_widget->label()->setText(tr("Plate &ID:"));
	d_relative_plate_id_widget->label()->setText(tr("R&elative Plate ID:"));
	d_conjugate_plate_id_widget->label()->setText(tr("C&onjugate ID:"));
	// Conjugate Plate IDs are optional.
	d_conjugate_plate_id_widget->set_null_value_permitted(true);
	d_conjugate_plate_id_widget->reset_widget_to_default_values();
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);
	
	// Set up checkbox for creating conjugate feature. 
	d_create_conjugate_feature_checkbox->setChecked(false);
	d_create_conjugate_feature_checkbox->setText("Create con&jugate feature");
	QString tool_tip_string("Create an additional feature using the same geometry, \
							and with plate id and conjugate plate id reversed.");
	d_create_conjugate_feature_checkbox->setToolTip(tool_tip_string);
	d_create_conjugate_feature_checkbox->setEnabled(false);

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
	plate_id_layout->addWidget(d_relative_plate_id_widget);
	// Relative plate id widget invisible by default since only used for MotionPath feature type.
	d_relative_plate_id_widget->setVisible(false);

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
	edit_layout->addWidget(d_create_conjugate_feature_checkbox);
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
GPlatesQtWidgets::CreateFeatureDialog::set_up_conjugate_properties_page()
{
	// Adding a new property changes the focus to the "Next" button.
	QObject::connect(d_create_conjugate_properties_page, SIGNAL(finished()),
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
	// Populate list of feature types that support the geometric property type.
	// If no geometric property type (not selected by user yet) then select all feature types.
	d_choose_feature_type_widget->populate(d_geometry_property_type);

	// Note that we don't set the feature type selection.
	// If the previously selected feature type is in the new list of feature types then
	// the selection will be retained (ie, it'll remember the last selection).
}


void
GPlatesQtWidgets::CreateFeatureDialog::select_default_feature_type()
{
	// Default feature type 'gpml:UnclassifiedFeature' (if it supports the geometric property type).
	static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	GPlatesModel::FeatureType default_feature_type = UNCLASSIFIED_FEATURE_TYPE;

	// Change the default feature types for topological polygons and networks (requested by Dietmar and Sabin).
	// It is not needed for topological lines since they were added later when pretty much any
	// feature type could have topological geometries and hence there was no feature type
	// designated as a 'topological line'.
	if (d_geometry_property_value)
	{
		if (is_topological_polygon(d_geometry_property_type.get()))
		{
			static const GPlatesModel::FeatureType TOPOLOGICAL_CLOSED_PLATE_BOUNDARY_FEATURE_TYPE =
					GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");
			default_feature_type = TOPOLOGICAL_CLOSED_PLATE_BOUNDARY_FEATURE_TYPE;
		}
		else if (is_topological_network(d_geometry_property_type.get()))
		{
			static const GPlatesModel::FeatureType TOPOLOGICAL_NETWORK_FEATURE_TYPE =
					GPlatesModel::FeatureType::create_gpml("TopologicalNetwork");
			default_feature_type = TOPOLOGICAL_NETWORK_FEATURE_TYPE;
		}
	}

	d_choose_feature_type_widget->set_feature_type(default_feature_type);
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_geometric_property_list()
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Populate the listwidget_geometry_destinations based on what is legal right now.
	d_listwidget_geometry_destinations->populate(d_feature_type.get(), d_geometry_property_type.get());
}


void
GPlatesQtWidgets::CreateFeatureDialog::select_default_geometry_property_name()
{
	//
	// Set the default geometry property name (if there is one) for the feature type.
	//

	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Get the GPGIM feature class for the feature type.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			GPlatesModel::Gpgim::instance().get_feature_class(d_feature_type.get());
	// Our list of features was obtained from the GPGIM so we should be able to find it.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			gpgim_feature_class,
			GPLATES_ASSERTION_SOURCE);

	// Get the default geometry property for the feature type.
	boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> default_gpgim_geometry_property =
			gpgim_feature_class.get()->get_default_geometry_feature_property();
	if (default_gpgim_geometry_property)
	{
		const GPlatesModel::PropertyName &default_geometry_property_name =
				default_gpgim_geometry_property.get()->get_property_name();

		// Get the list of geometry properties actually populated in the list widget.
		std::vector<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_geometry_properties;
		ChoosePropertyWidget::get_properties_to_populate(
				gpgim_geometry_properties,
				d_feature_type.get(),
				d_geometry_property_type.get());

		// If the default geometry property name appears in the list then select it.
		BOOST_FOREACH(
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_geometry_property,
				gpgim_geometry_properties)
		{
			if (gpgim_geometry_property->get_property_name() == default_geometry_property_name)
			{
				d_listwidget_geometry_destinations->set_property_name(default_geometry_property_name);
				break;
			}
		}
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_common_properties()
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Copy gml:name property into widget.
	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> name_property =
			find_property_value<GPlatesPropertyValues::XsString>(
					GPlatesModel::PropertyName::create_gml("name"),
					d_feature_properties);
	if (name_property)
	{
		d_name_widget->update_widget_from_string(*name_property.get());
	}
	else
	{
		d_name_widget->reset_widget_to_default_values();
	}

	// Copy gml:validTime property into widget.
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type> valid_time_property =
			find_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					GPlatesModel::PropertyName::create_gml("validTime"),
					d_feature_properties);
	if (valid_time_property)
	{
		d_time_period_widget->update_widget_from_time_period(*valid_time_property.get());
	}
	else
	{
		d_time_period_widget->reset_widget_to_default_values();
	}

	// Force flowline feature type to be reconstructed as HALF_STAGE_ROTATION.
	if (d_feature_type.get() == GPlatesModel::FeatureType::create_gpml("Flowline"))
	{
		d_recon_method_combobox->setEnabled(false); // Prevent user from changing option.
		d_recon_method_combobox->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
	}
	// Force motion path feature type to be reconstructed as BY_PLATE_ID.
	// (Later we should allow changing to HALF_STAGE_ROTATION; the MotionPathGeometryPopulator
	// won't currently handle this correctly, so disable this option until we do handle it).
	else if (d_feature_type.get() == GPlatesModel::FeatureType::create_gpml("MotionPath"))
	{
		d_recon_method_combobox->setEnabled(false); // Prevent user from changing option.
		d_recon_method_combobox->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	}
	else
	{
		// See if the feature type supports a reconstruction method property.
		if (GPlatesModel::Gpgim::instance().get_feature_property(
				d_feature_type.get(),
				GPlatesModel::PropertyName::create_gpml("reconstructionMethod")))
		{
			d_recon_method_combobox->setEnabled(true);
		}
		else
		{
			d_recon_method_combobox->setEnabled(false);
		}

		// Copy gpml:reconstructionMethod property into widget.
		boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_type> reconstruction_method_property =
				find_property_value<GPlatesPropertyValues::Enumeration>(
						GPlatesModel::PropertyName::create_gpml("reconstructionMethod"),
						d_feature_properties);
		if (reconstruction_method_property)
		{
			if (reconstruction_method_property.get()->value().get() == "ByPlateId")
			{
				d_recon_method_combobox->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			}
			else
			{
				d_recon_method_combobox->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
			}
		}
		else
		{
			d_recon_method_combobox->setCurrentIndex(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
		}
	}

	// Copy gpml:leftPlate property into widget.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type> left_plate_property =
			find_property_value<GPlatesPropertyValues::GpmlPlateId>(
					GPlatesModel::PropertyName::create_gpml("leftPlate"),
					d_feature_properties);
	if (left_plate_property)
	{
		d_left_plate_id->update_widget_from_plate_id(*left_plate_property.get());
	}
	else
	{
		d_left_plate_id->reset_widget_to_default_values();
	}

	// Copy gpml:rightPlate property into widget.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type> right_plate_property =
			find_property_value<GPlatesPropertyValues::GpmlPlateId>(
					GPlatesModel::PropertyName::create_gpml("rightPlate"),
					d_feature_properties);
	if (right_plate_property)
	{
		d_right_plate_id->update_widget_from_plate_id(*right_plate_property.get());
	}
	else
	{
		d_right_plate_id->reset_widget_to_default_values();
	}

	// Copy gpml:relativePlate property into widget.
	if (should_offer_relative_plate_id_prop(d_feature_type))
	{
		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type> relative_plate_property =
				find_property_value<GPlatesPropertyValues::GpmlPlateId>(
						GPlatesModel::PropertyName::create_gpml("relativePlate"),
						d_feature_properties);
		if (relative_plate_property)
		{
			d_relative_plate_id_widget->update_widget_from_plate_id(*relative_plate_property.get());
		}
		else
		{
			d_relative_plate_id_widget->reset_widget_to_default_values();
		}

		d_relative_plate_id_widget->setVisible(
				d_recon_method == GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	}
	else
	{
		d_relative_plate_id_widget->setVisible(false);
	}

	// Copy gpml:reconstructionPlateId property into widget.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type> plate_id_property =
			find_property_value<GPlatesPropertyValues::GpmlPlateId>(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					d_feature_properties);
	if (plate_id_property)
	{
		d_plate_id_widget->update_widget_from_plate_id(*plate_id_property.get());
	}
	else
	{
		d_plate_id_widget->reset_widget_to_default_values();
	}

	// Copy gpml:conjugatePlateId property into widget.
	if (should_offer_conjugate_plate_id_prop(d_feature_type))
	{
		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type> conjugate_plate_id_property =
				find_property_value<GPlatesPropertyValues::GpmlPlateId>(
						GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
						d_feature_properties);
		if (conjugate_plate_id_property)
		{
			d_conjugate_plate_id_widget->update_widget_from_plate_id(*conjugate_plate_id_property.get());
		}
		else
		{
			d_conjugate_plate_id_widget->reset_widget_to_default_values();
		}

		d_conjugate_plate_id_widget->setVisible(
				d_recon_method == GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
	}
	else
	{
		d_conjugate_plate_id_widget->setVisible(false);
	}


	// Note that we leave the 'create conjugate feature' checkbox state to what it was previously
	// but we hide it if it's not applicable for the current feature/geometry type.
	d_create_conjugate_feature_checkbox->setVisible(
			should_offer_create_conjugate_feature_checkbox(d_feature_type, d_geometry_property_type));

	//
	// Enable user to hit 'enter' to quickly move through the visible options.
	//
	// These are here instead of 'set_up_common_properties_page()' because these options
	// depend on the widget visibility which can change.
	//

	// Disconnect signals first so we don't end up with duplicate connections.
	QObject::disconnect(
			d_left_plate_id, SIGNAL(enter_pressed()),
			d_right_plate_id, SLOT(setFocus()));
	QObject::disconnect(
			d_right_plate_id, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_plate_id_widget, SIGNAL(enter_pressed()),
			d_conjugate_plate_id_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_plate_id_widget, SIGNAL(enter_pressed()),
			d_relative_plate_id_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_conjugate_plate_id_widget, SIGNAL(enter_pressed()),
			d_relative_plate_id_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_conjugate_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::disconnect(
			d_relative_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));

	// Connect the signals.
	if (d_recon_method == GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION)
	{
		d_left_plate_id->setFocus();

		// The various Edit widgets need pass focus along the chain if Enter is pressed.
		QObject::connect(d_left_plate_id, SIGNAL(enter_pressed()),
				d_right_plate_id, SLOT(setFocus()));
		QObject::connect(d_right_plate_id, SIGNAL(enter_pressed()),
				d_time_period_widget, SLOT(setFocus()));
	}
	else // GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID...
	{
		d_plate_id_widget->setFocus();

		// The various Edit widgets need pass focus along the chain if Enter is pressed.
		if (should_offer_conjugate_plate_id_prop(d_feature_type))
		{
			QObject::connect(d_plate_id_widget, SIGNAL(enter_pressed()),
					d_conjugate_plate_id_widget, SLOT(setFocus()));
			if (should_offer_relative_plate_id_prop(d_feature_type))
			{
				QObject::connect(d_conjugate_plate_id_widget, SIGNAL(enter_pressed()),
						d_relative_plate_id_widget, SLOT(setFocus()));
				QObject::connect(d_relative_plate_id_widget, SIGNAL(enter_pressed()),
						d_time_period_widget, SLOT(setFocus()));
			}
			else
			{
				QObject::connect(d_conjugate_plate_id_widget, SIGNAL(enter_pressed()),
						d_time_period_widget, SLOT(setFocus()));
			}
		}
		else if (should_offer_relative_plate_id_prop(d_feature_type))
		{
			QObject::connect(d_plate_id_widget, SIGNAL(enter_pressed()),
					d_relative_plate_id_widget, SLOT(setFocus()));
			QObject::connect(d_relative_plate_id_widget, SIGNAL(enter_pressed()),
					d_time_period_widget, SLOT(setFocus()));
		}
		else
		{
			QObject::connect(d_plate_id_widget, SIGNAL(enter_pressed()),
					d_time_period_widget, SLOT(setFocus()));
		}
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_all_properties_gui()
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Get the PropertyName the user has selected for geometry to go into.
	// This property will be unavailable for the user to add via the create-feature-properties page.
	boost::optional<GPlatesModel::PropertyName> geometry_property_name =
			d_listwidget_geometry_destinations->get_property_name();
	if (!geometry_property_name)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	
	// What adjective should be displayed in front of the feature type? Should be '' when we're just
	// creating one feature at a time, but if we are also going to make a conjugate feature, we should
	// differentiate this by calling it the 'primary ' feature.
	QString adjective = "";
	if (should_offer_conjugate_properties_page(
				d_feature_type, d_geometry_property_type, *d_create_conjugate_feature_checkbox)) {
		adjective = tr("primary ");
	}

	// Set the feature type and initial feature properties.
	// The user can then add more properties supported by the feature type.
	d_create_feature_properties_page->initialise(
			d_feature_type.get(),
			d_feature_properties,
			CreateFeaturePropertiesPage::property_name_seq_type(1, geometry_property_name.get()),
			adjective
			);
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_conjugate_properties_gui()
{
	// We should not be able to reach here without having first gone through the Feature Type,
	// Common Properties, and All Properties pages.
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Get the PropertyName the user has selected for geometry to go into.
	// This property will be unavailable for the user to add via the create-feature-properties page.
	boost::optional<GPlatesModel::PropertyName> geometry_property_name =
			d_listwidget_geometry_destinations->get_property_name();
	if (!geometry_property_name)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}

	// Set the feature type and initial conjugate feature properties.
	// The user can then tweak those conjugate properties before moving on to feature creation.
	d_create_conjugate_properties_page->initialise(
			d_feature_type.get(),
			d_conjugate_properties,
			CreateFeaturePropertiesPage::property_name_seq_type(1, geometry_property_name.get()),
			tr("conjugate ")
			);
}


void
GPlatesQtWidgets::CreateFeatureDialog::clear_properties_not_allowed_for_current_feature_type()
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Get the GPGIM feature class for the feature type.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			GPlatesModel::Gpgim::instance().get_feature_class(d_feature_type.get());
	// Our list of features was obtained from the GPGIM so we should be able to find it.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			gpgim_feature_class,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the current properties and remove any not allowed by GPGIM for current feature type.
	GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type::iterator
			feature_properties_iter = d_feature_properties.begin();
	while (feature_properties_iter != d_feature_properties.end())
	{
		const GPlatesModel::PropertyName &feature_property_name =
				(*feature_properties_iter)->property_name();

		if (gpgim_feature_class.get()->get_feature_property(feature_property_name))
		{
			++feature_properties_iter;
		}
		else
		{
			feature_properties_iter = d_feature_properties.erase(feature_properties_iter);
		}
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::copy_common_properties_into_all_properties()
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	//
	// Update the 'all' properties list with the common feature properties from the data the user
	// entered into the common properties page.
	//
	// Note: Non-common properties can exist, even though we're transitioning away from the
	// 'common properties' page, when we're keeping properties around from the last digitised feature
	// (to make it quicker/easier for users to repeatedly digitise features of the same type that
	// have some or all, excluding geometry, properties that are the same).
	//

	try
	{
		// Add a gml:name Property.
		copy_common_property_into_all_properties(
				GPlatesModel::PropertyName::create_gml("name"),
				d_name_widget->create_property_value_from_widget());

		// Add a gml:validTime Property.
		copy_common_property_into_all_properties(
				GPlatesModel::PropertyName::create_gml("validTime"),
				d_time_period_widget->create_property_value_from_widget());

		// Add a gpml:geometryImportTime property if the feature type supports it.
		//
		// This gets added to all reconstructable features.
		// It is used by MidOceanRidge features, along with left plate ID, to reconstruct ridge position to the
		// import time and then perform ridge-spreading from there. This enables the user to subsequently change
		// the spreading asymmetry without significantly affecting the ridge position (with no effect at the import time)
		//
		// Also used when a feature's geometry is reconstructed using topologies
		// (instead of using the feature's properties, eg, reconstruction plate ID).
		// This enables paleo-geometries to be used (eg, fracture zones prior to subduction) and masked by topologies
		// through time (mid-ocean ridges going backward in time and subduction zones going forward in time).
		if (should_add_import_geometry_time_prop(d_feature_type))
		{
			copy_common_property_into_all_properties(
					GPlatesModel::PropertyName::create_gpml("geometryImportTime"),
					GPlatesModel::ModelUtils::create_gml_time_instant(
							GPlatesPropertyValues::GeoTimeInstant(
									d_application_state_ptr->get_current_reconstruction_time())));
		}

		// If we are using half stage rotation.
		// Note that the 'if' and 'else' parts do the reverse of each other.
		if (GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION == d_recon_method)
		{
			// Add a gpml:reconstructionMethod property.
			copy_common_property_into_all_properties(
					GPlatesModel::PropertyName::create_gpml("reconstructionMethod"),
					GPlatesPropertyValues::Enumeration::create(
							GPlatesPropertyValues::EnumerationType::create_gpml("ReconstructionMethodEnumeration"),
							"HalfStageRotationVersion3"));

			// Add gpml:leftPlate and gpml:rightPlate properties.
			copy_common_property_into_all_properties(
					GPlatesModel::PropertyName::create_gpml("leftPlate"),
					d_left_plate_id->create_property_value_from_widget());
			copy_common_property_into_all_properties(
					GPlatesModel::PropertyName::create_gpml("rightPlate"),
					d_right_plate_id->create_property_value_from_widget());

			// Remove gpml:relativePlate Property.
			if (should_offer_relative_plate_id_prop(d_feature_type))
			{
				remove_common_property_from_all_properties(
						GPlatesModel::PropertyName::create_gpml("relativePlate"));
			}

			// Remove gpml:reconstructionPlateId Property.
			remove_common_property_from_all_properties(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"));

			// Remove gpml:conjugatePlateId Property.
			if (should_offer_conjugate_plate_id_prop(d_feature_type))
			{
				remove_common_property_from_all_properties(
						GPlatesModel::PropertyName::create_gpml("conjugatePlateId"));
			}
		}
		else // Not using a half-stage rotation...
		{
			// Remove gpml:reconstructionMethod property.
			remove_common_property_from_all_properties(
					GPlatesModel::PropertyName::create_gpml("reconstructionMethod"));

			// Remove gpml:leftPlate and gpml:rightPlate properties.
			remove_common_property_from_all_properties(
					GPlatesModel::PropertyName::create_gpml("leftPlate"));
			remove_common_property_from_all_properties(
					GPlatesModel::PropertyName::create_gpml("rightPlate"));

			// Add a gpml:relativePlate Property.
			if (should_offer_relative_plate_id_prop(d_feature_type))
			{
				copy_common_property_into_all_properties(
						GPlatesModel::PropertyName::create_gpml("relativePlate"),
						d_relative_plate_id_widget->create_property_value_from_widget());
			}

			// Add a gpml:reconstructionPlateId Property.
			copy_common_property_into_all_properties(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					d_plate_id_widget->create_property_value_from_widget());

			if (should_offer_conjugate_plate_id_prop(d_feature_type))
			{
				if (d_conjugate_plate_id_widget->is_null())
				{
					// Remove gpml:conjugatePlateId Property.
					remove_common_property_from_all_properties(
							GPlatesModel::PropertyName::create_gpml("conjugatePlateId"));
				}
				else
				{
					// Add a gpml:conjugatePlateId Property.
					copy_common_property_into_all_properties(
							GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
							d_conjugate_plate_id_widget->create_property_value_from_widget());
				}
			}
		}
	}
	catch (const InvalidPropertyValueException &exc)
	{
		QMessageBox::critical(this, tr("Property Value Invalid"),
				tr("A feature property could not be added: %1.").arg(exc.reason()),
				QMessageBox::Ok);
		return;
	}

	//
	// If the geometry property (we'll be adding later) only supports at most one property per feature
	// (according to GPGIM) then delete any extra properties of same name that user might have
	// added via the 'all properties' page.
	//
	// Alternatively we could have prevented the user from adding an extra geometry property
	// (of same name) in the 'all properties' page (if at most one property is allowed), but this
	// would have limited the user's choices for the geometry property name.
	// So it comes down to a tradeoff and we've chosen to avoid limiting the choice of geometry
	// property names. Besides it's very unlikely the user will explicitly create a geometry property
	// in the 'all properties' page - after all it's the digitised geometry that they're interested in.
	//

	// Get the PropertyName the user has selected for geometry to go into.
	// This property will be unavailable for the user to add via the create-feature-properties page.
	boost::optional<GPlatesModel::PropertyName> geometry_property_name =
			d_listwidget_geometry_destinations->get_property_name();
	if (!geometry_property_name)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}

	boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> geometry_gpgim_property =
			GPlatesModel::Gpgim::instance().get_feature_property(d_feature_type.get(), geometry_property_name.get());

	// If the selected geometry property name is allowed to occur at most once per feature then
	// remove any occurrences in the current feature properties so that there's room when we add
	// the geometry property later.
	if (geometry_gpgim_property.get()->get_multiplicity() == GPlatesModel::GpgimProperty::ZERO_OR_ONE ||
		geometry_gpgim_property.get()->get_multiplicity() == GPlatesModel::GpgimProperty::ONE)
	{
		// Remove if exists.
		remove_common_property_from_all_properties(geometry_property_name.get());
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::copy_common_property_into_all_properties(
		const GPlatesModel::PropertyName &property_name,
		const GPlatesModel::PropertyValue::non_null_ptr_type &property_value)
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// Create a new top-level property for the common property.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> top_level_property =
			GPlatesModel::ModelUtils::create_top_level_property(
					property_name,
					property_value,
					d_feature_type.get(),
					true/*check_property_value_type*/,
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

	// If the property already exists then just replace it with the new property.
	// This will replace the first property found if multiple properties with same name exist.
	CreateFeaturePropertiesPage::property_seq_type::iterator feature_properties_iter = d_feature_properties.begin();
	CreateFeaturePropertiesPage::property_seq_type::iterator feature_properties_end = d_feature_properties.end();
	for ( ; feature_properties_iter != feature_properties_end; ++feature_properties_iter)
	{
		if ((*feature_properties_iter)->property_name() == property_name)
		{
			*feature_properties_iter = top_level_property.get();
			return;
		}
	}

	// Property does not already exist so add it to the list.
	d_feature_properties.push_back(top_level_property.get());
}


void
GPlatesQtWidgets::CreateFeatureDialog::remove_common_property_from_all_properties(
		const GPlatesModel::PropertyName &property_name)
{
	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

	// If the property already exists then just remove it (but we only remove the first occurrence
	// in case user has added extra occurrences, if allowed by GPGIM, in the 'all properties' page).
	CreateFeaturePropertiesPage::property_seq_type::iterator feature_properties_iter = d_feature_properties.begin();
	CreateFeaturePropertiesPage::property_seq_type::iterator feature_properties_end = d_feature_properties.end();
	for ( ; feature_properties_iter != feature_properties_end; ++feature_properties_iter)
	{
		if ((*feature_properties_iter)->property_name() == property_name)
		{
			d_feature_properties.erase(feature_properties_iter);
			return;
		}
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::generate_conjugate_properties_from_all_properties()
{
	// This function is called in the transition from the "all properties" page and the "conjugate properties"
	// page, presuming that a conjugate feature is to be created. It takes the "all properties" list
	// that the user is presumably satisfied with, flips the names of a few of them, and uses it for the
	// conjugate properties that the user can then tweak via the GUI.
	
	static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	static const GPlatesModel::PropertyName CONJUGATE_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("conjugatePlateId");
	static const GPlatesModel::PropertyName NAME =
			GPlatesModel::PropertyName::create_gml("name");
	static const GPlatesModel::PropertyName QUALITY =
			GPlatesModel::PropertyName::create_gpml("quality");
	
	d_conjugate_properties.clear();
	CreateFeaturePropertiesPage::property_seq_type::iterator it = d_feature_properties.begin();
	CreateFeaturePropertiesPage::property_seq_type::iterator end = d_feature_properties.end();
	for ( ; it != end; ++it) {
		const GPlatesModel::PropertyName &prop_name = (*it)->property_name();
		
		if (prop_name == RECONSTRUCTION_PLATE_ID) {
			// If we come across a gpml:reconstructionPlateId property in the primary feature,
			// assign it to the conjugate feature as gpml:conjugatePlateId.
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> prop_maybe = 
			        GPlatesModel::ModelUtils::rename_property(**it, CONJUGATE_PLATE_ID);
			if (prop_maybe) {
				d_conjugate_properties.push_back(prop_maybe.get());
			}

		} else if (prop_name == CONJUGATE_PLATE_ID) {
			// If we come across a gpml:conjugatePlateId property in the primary feature,
			// assign it to the conjugate feature as gpml:reconstructionPlateId.
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> prop_maybe = 
			        GPlatesModel::ModelUtils::rename_property(**it, RECONSTRUCTION_PLATE_ID);
			if (prop_maybe) {
				d_conjugate_properties.push_back(prop_maybe.get());
			}
			
		} else if (prop_name == NAME) {
			// Get the name from the feature.
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_value_maybe =
					GPlatesModel::ModelUtils::get_property_value(**it);
			if (prop_value_maybe) {
				// We know the name (should be) an XsString.
				boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> prop_value_string =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(*prop_value_maybe.get());
				if (!prop_value_string)
				{
					continue;
				}
				
				// We'd like it as a QString, momentarily, so we can do stuff to it.
				QString conjugate_name_string = GPlatesUtils::make_qstring_from_icu_string(
						prop_value_string.get()->value().get());
				conjugate_name_string.prepend("Conjugate of ");
				
				// Then marshal it back into a property value, and that into a TopLevelProperty, and add it.
				GPlatesModel::PropertyValue::non_null_ptr_type conjugate_name_prop_value =
						GPlatesPropertyValues::XsString::create(
								GPlatesUtils::make_icu_string_from_qstring(conjugate_name_string));
				boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> conjugate_prop_maybe =
				        generate_conjugate_property(prop_name, conjugate_name_prop_value);
				if (conjugate_prop_maybe) {
					d_conjugate_properties.push_back(conjugate_prop_maybe.get());
				}
			}
			
		} else if (prop_name == QUALITY) {
			// Get the quality enum value from the feature.
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_value_maybe =
					GPlatesModel::ModelUtils::get_property_value(**it);
			if (prop_value_maybe) {
				// We know the quality (should be) an Enumeration.
				boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> prop_value_enum =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::Enumeration>(*prop_value_maybe.get());
				if (!prop_value_enum)
				{
					continue;
				}
				
				// Depending on what was chosen for the primary feature's gpml:quality, we may want to default
				// to a different quality setting on the conjugate. Otherwise, we copy the original value.
				QString enum_value = prop_value_enum.get()->value().get().qstring();
				if (enum_value == "OneFlankPreserved") {
					enum_value = "SyntheticConstrainedByGeology";
				}
				
				// Create a new Enumeration property using the maybe-modified content.
				GPlatesModel::PropertyValue::non_null_ptr_type conjugate_quality_prop_value =
						GPlatesPropertyValues::Enumeration::create(
								prop_value_enum.get()->type(), GPlatesUtils::UnicodeString(enum_value));
				boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> conjugate_prop_maybe =
				        generate_conjugate_property(prop_name, conjugate_quality_prop_value);
				if (conjugate_prop_maybe) {
					d_conjugate_properties.push_back(conjugate_prop_maybe.get());
				}
			}
			
		} else {
			// Just an ordinary property we don't need to mess around with. Give the conjugate a clone.
			GPlatesModel::TopLevelProperty::non_null_ptr_type prop = (*it)->deep_clone();
			d_conjugate_properties.push_back(prop);
		}
	}
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesQtWidgets::CreateFeatureDialog::generate_conjugate_property(
		const GPlatesModel::PropertyName &property_name,
		const GPlatesModel::PropertyValue::non_null_ptr_type &property_value)
{
	if ( ! d_feature_type)
	{
		return boost::none;
	}

	// Are we really not allowed to just ->set_property_name? I can understand if it was a property
	// that was already attached to a feature, but if it's one I've freshly cloned... well, Fine,
	// let's create a brand new one.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> top_level_property =
			GPlatesModel::ModelUtils::create_top_level_property(
					property_name,
					property_value,
					d_feature_type.get(),
					true/*check_property_value_type*/,
					&error_code);
	if ( ! top_level_property)
	{
		// Not successful in creating property; shouldn't happen.
		QMessageBox::warning(this,
				QObject::tr("Failed to create a conjugate feature property."),
				QObject::tr(GPlatesModel::ModelUtils::get_error_message(error_code)) +
						" " + QObject::tr("Property '") +
						convert_qualified_xml_name_to_qstring(property_name) +
						QObject::tr("' will not be added to the feature."),
				QMessageBox::Ok);
		return boost::none;
	}

	return top_level_property;
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

	// Get the structural type of the geometric property.
	d_geometry_property_type =
			GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(
					*geometry_property_value);

	// Display dialog.
	return display();
}


bool
GPlatesQtWidgets::CreateFeatureDialog::display()
{
	// Reset the current page to the start.
	// This is needed so the page transitions are handled correctly (even if user previously
	// aborted dialog and hence didn't necessarily get to the last page).
	d_current_page = FEATURE_TYPE_PAGE;

	// Populate the d_choose_feature_type_widget based on what features support
	// the geometric property type.
	//
	// NOTE: If the dialog was last left on the first page (by the user) then just selecting
	// the first page will not result in a page change and hence no event.
	// So we need to explicitly set up the feature type list - normally done in the first page
	// (the feature type page).
	set_up_feature_list();

	// Set the stack back to the first page.
	stack->setCurrentIndex(FEATURE_TYPE_PAGE);

	// Select the default feature type based on the geometry property type.
	// We only do this once for each time this dialog is invoked.
	// And only if the feature type has changed - meaning the above 'set_up_feature_list()' could not
	// find the same feature type compatible with the new geometry property type and so selected a
	// random one - in this case we want to set the default feature type (based on the new geometry
	// property type) instead of the random one. Otherwise we want to keep the previously selected
	// feature type (from the last Create Feature dialog invocation) so user can quickly digitise
	// the same feature type repeatedly.
	if (d_feature_type != d_choose_feature_type_widget->get_feature_type())
	{
		select_default_feature_type();
	}

	// The Feature Collections list needs to be repopulated each time.
	d_choose_feature_collection_widget->initialise();

	// Show the dialog modally.
	const int dialog_result = exec();

	// If the user created a feature then we keep the feature properties for the next digitised
	// feature in case it's the same geometry/feature type and user wants to re-use the properties.
	//
	// If the user aborted then we also keep the feature properties for next time, but the user
	// could have aborted while on any page, so we might need to update the feature properties
	// from the corresonding page widgets.
	if (dialog_result != QDialog::Accepted)
	{
		if (stack->currentIndex() == COMMON_PROPERTIES_PAGE)
		{
			// Copy the common properties (from widgets) into the 'all' properties list.
			copy_common_properties_into_all_properties();
		}
		else if (stack->currentIndex() == ALL_PROPERTIES_PAGE || stack->currentIndex() == CONJUGATE_PROPERTIES_PAGE)
		{
			// Extract all the feature properties from the 'all properties' page widget.
			// This will include the common properties (from the previous page).
			d_feature_properties.clear();
			d_create_feature_properties_page->get_feature_properties(d_feature_properties);
		}
	}

	return dialog_result;
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_prev()
{
	// Note: Any code here should only be to prevent transitioning to previous page.
	// Otherwise page change code should go into 'handle_page_change()'.

	// Transition to the previous page.
	int prev_index = stack->currentIndex() - 1;
	// Skip over the "Conjugate Properties" page if we are not creating a Feature/Conjugate pair.
	if (prev_index == CONJUGATE_PROPERTIES_PAGE && ! should_offer_conjugate_properties_page(
			d_feature_type, d_geometry_property_type, *d_create_conjugate_feature_checkbox)) {
		prev_index--;
	}
	
	if (prev_index >= 0 && prev_index < stack->count())
	{
		stack->setCurrentIndex(prev_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_next()
{
	// Note: Any code here should only be to prevent transitioning to next page.
	// Otherwise page change code should go into 'handle_page_change()'.

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
	else if (stack->currentIndex() == CONJUGATE_PROPERTIES_PAGE)
	{
		// If there are required feature properties missing from the conjugate too, do not change page.
		if (!d_create_conjugate_properties_page->is_finished())
		{
			QMessageBox::warning(this,
				tr("Conjugate Feature is missing required properties"),
				tr("Please add properties that are required for the feature type.\n"
					"These are properties that have a minimum multiplicity of one."),
				QMessageBox::Ok);
			return;
		}
	}

	// Transition to the next page.
	int next_index = stack->currentIndex() + 1;
	// Skip over the "Conjugate Properties" page if we are not creating a Feature/Conjugate pair.
	if (next_index == CONJUGATE_PROPERTIES_PAGE && ! should_offer_conjugate_properties_page(
			d_feature_type, d_geometry_property_type, *d_create_conjugate_feature_checkbox)) {
		next_index++;
	}

	if (next_index < stack->count())
	{
		stack->setCurrentIndex(next_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_page_change(
		int page)
{
	handle_leave_page(d_current_page);
	handle_enter_page(page, d_current_page);

	// Update the current page (helps determine page transitions).
	d_current_page = static_cast<StackedWidgetPage>(page);
}	


void
GPlatesQtWidgets::CreateFeatureDialog::handle_leave_page(
		int page)
{
	// Store any data entered in widgets into our lists before leaving a page.
	switch (page)
	{
	case FEATURE_TYPE_PAGE:
		// Every time we leave the initial 'select feature type' page, we want to store that type
		// in d_feature_type; however, it is also important for us to record what the previously
		// selected feature type was, so we can keep or discard properties as needed.
		d_previously_selected_feature_type = d_feature_type;
		
		// Set the current feature type.
		d_feature_type = d_choose_feature_type_widget->get_feature_type();
		
		// Leaving the Feature Type page also needs to do some special initialisation of the rest
		// of the dialog, if we're digitising a different feature type from before.
		if (d_feature_type != d_previously_selected_feature_type)
		{
			// If the feature type has changed then clear out those existing feature properties that
			// are not allowed (by the GPGIM) for the new feature type.
			// These are properties from the currently or previously digitised feature that are no longer
			// applicable due to changing the feature type (repeatedly digitising the same feature/geometry
			// type is meant to be a fast path for the user - where they can re-use previous properties).
			clear_properties_not_allowed_for_current_feature_type();
		}
		// Also clear the conjugate properties list; this forces it to be repopulated when
		// the user enters the 'conjugate properties' page from the 'all properties' page
		// if they are creating a conjugate feature.
		d_conjugate_properties.clear();
		
		break;
	
	case COMMON_PROPERTIES_PAGE:
		// We're transitioning away from the common properties page so copy
		// common properties into the 'all' properties list.
		copy_common_properties_into_all_properties();
		break;

	case ALL_PROPERTIES_PAGE:
		// We're transitioning away from the 'all properties' page.
		// So extract all the feature properties now that the user has finished adding/editing them.
		// They don't need to be complete - we just need them so we can copy their values
		// into the common properties widgets, or generate the conjugate properties list from them.
		d_feature_properties.clear();
		d_create_feature_properties_page->get_feature_properties(d_feature_properties);
		break;
		
	case CONJUGATE_PROPERTIES_PAGE:
		// We're transitioning away from the 'conjugate properties' page.
		// So extract all the conjugate feature properties now that the user has finished adding/editing them.
		// They will probably be complete - assuming we are transitioning to the next 'feature collection' page,
		// although it does no harm to store incomplete properties while we go back to the primary feature's
		// property list. Either way, we want to ensure the properties are extracted when we leave this page.
		d_conjugate_properties.clear();
		d_create_conjugate_properties_page->get_feature_properties(d_conjugate_properties);
		break;

	case FEATURE_COLLECTION_PAGE:
		break;
		
	default:
		// Shouldn't get here. Every page must have its own handle_leave_page and handle_enter_page logic,
		// even if it's just a break;
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_enter_page(
		int page,
		int last_page)
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
		// Disable buttons appropriately.
		button_prev->setEnabled(false);
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);

		// Populate the d_choose_feature_type_widget based on what features support
		// the geometric property type.
		set_up_feature_list();

		d_choose_feature_type_widget->setFocus();
		break;

	case COMMON_PROPERTIES_PAGE:
		// Disable buttons appropriately.
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);

		// Populate the listwidget_geometry_destinations based on what is legal right now.
		// If the previously selected geometry property is in the freshly-populated list,
		// that selection is retained.
		set_up_geometric_property_list();

		// If we're starting from scratch (eg, first time digitised or changed feature type)
		// then select the default geometry property name based on the feature type.
		//
		// Note that we really want to encourage users to use the 'default' geometry property
		// for a feature type as this makes retrieving geometries (eg, in python pygplates API)
		// much easier for users (since they don't have to specify the geometry property name -
		// when not specified pygplates will extract the 'default' geometry property).
		if (last_page < COMMON_PROPERTIES_PAGE && d_feature_type != d_previously_selected_feature_type)
		{
			select_default_geometry_property_name();
		}

		// Set up the common properties widgets based on the current feature properties (if any).
		set_up_common_properties();
		break;

	case ALL_PROPERTIES_PAGE:
		// Disable buttons appropriately.
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);

		// Get the GUI CreateFeaturePropertiesPage element to initialise itself based on the 'all properties'
		// list we've either built from the 'common properties' or had lying around from last time.
		set_up_all_properties_gui();
		d_create_feature_properties_page->setFocus();
		break;

	case CONJUGATE_PROPERTIES_PAGE:
		// Disable buttons appropriately.
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);

		if (last_page < CONJUGATE_PROPERTIES_PAGE)
		{
			// We're transitioning from the previous ("all" feature properties) page so copy
			// all those properties into the 'conjugate' properties list, swapping a few
			// of the property names as necessary.
			//
			// Since the user may want to do a bit of back-and-forth between the two pages, we want the
			// behaviour to be "generate conjugate properties list based on primary feature properties,
			// but only the first time we are viewed for the creation of this particular feature".
			// To accomplish this we ensure d_conjugate_properties is cleared after choosing a feature
			// type, and only generate it based on d_feature_properties if it is empty when we reach
			// this point.
			if (d_conjugate_properties.empty()) {
				generate_conjugate_properties_from_all_properties();
			}
		}
		
		// Get the GUI CreateFeaturePropertiesPage element to initialise itself based on the 'conjugate properties'
		// list we've built from the 'all properties' list.
		set_up_conjugate_properties_gui();
		d_create_conjugate_properties_page->setFocus();
		break;
		
	case FEATURE_COLLECTION_PAGE:
		// Disable buttons appropriately.
		button_next->setEnabled(false);

		d_choose_feature_collection_widget->setFocus();
		break;

	default:
		// Shouldn't get here. Every page must have its own handle_leave_page and handle_enter_page logic,
		// even if it's just a break;
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
{
	static const GPlatesModel::PropertyName CONJUGATE =
			GPlatesModel::PropertyName::create_gpml("conjugate");

	if (!d_feature_type)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}

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
	
	try
	{
		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool>
				feature_collection_file_iter = d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				(feature_collection_file_iter.first).get_file().get_feature_collection();

		// We want to merge model events across this scope so that only one model event
		// is generated instead of multiple.
		GPlatesModel::NotificationGuard model_notification_guard(
				*d_application_state_ptr->get_model_interface().access_model());

		
		// Create the primary feature based on the list of properties we have assembled.
		GPlatesModel::FeatureHandle::non_null_ptr_type primary_feature = create_feature(
				d_feature_type.get(), d_feature_properties, geometry_property_name, feature_collection);

		// Add the primary feature to the feature collection.
		feature_collection->add(primary_feature);
		
		// If we presented the user with the Conjugate Properties page, use them to construct
		// a conjugate feature in the same manner and add that, too.
		if (should_offer_conjugate_properties_page(
				d_feature_type, d_geometry_property_type, *d_create_conjugate_feature_checkbox))
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type conjugate_feature = create_feature(
					d_feature_type.get(), d_conjugate_properties, geometry_property_name, feature_collection);
			feature_collection->add(conjugate_feature);
			
			// Since we are in the position to know with absolute certainty that these two
			// features we are creating are related, we can express that relationship in the data.
			// Future work on the EditFeatureReferenceWidget could provide a button that you
			// could click to take you from one to the other! Amazing.
			if (should_link_with_conjugate_prop(d_feature_type)) {
				create_feature_link(primary_feature, CONJUGATE, conjugate_feature);
				create_feature_link(conjugate_feature, CONJUGATE, primary_feature);
			}
		}

				
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before we emit signals because we don't
		// know whose listening on those signals and they may be expecting model observers to
		// be up-to-date with the modified model.
		model_notification_guard.release_guard();

		Q_EMIT feature_created(primary_feature->reference());

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
			d_relative_plate_id_widget->setVisible(false);
			d_right_plate_id->setVisible(true);
			d_left_plate_id->setVisible(true);
			d_recon_method = GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION;
			break;
		
		case GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID:
			d_right_plate_id->setVisible(false);
			d_left_plate_id->setVisible(false);
			d_plate_id_widget->setVisible(true);
			d_conjugate_plate_id_widget->setVisible(
					should_offer_conjugate_plate_id_prop(d_feature_type));
			d_relative_plate_id_widget->setVisible(
					should_offer_relative_plate_id_prop(d_feature_type));
			d_recon_method = GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID;
			break;
	}

	d_create_conjugate_feature_checkbox->setEnabled(
			!d_conjugate_plate_id_widget->is_null() &&
			d_recon_method == GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_conjugate_plate_id_changed()
{
	d_create_conjugate_feature_checkbox->setEnabled(
			!d_conjugate_plate_id_widget->is_null() &&
			d_recon_method == GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_feature_type_changed()
{
	//
	// NOTE: In this function we use 'd_choose_feature_type_widget->get_feature_type()' instead of
	// 'd_feature_type'. The latter is set only once we've moved on from the feature type page -
	// this enables us to use 'd_feature_type' to detect changes in feature type without being
	// affected by feature type resets (which can get signaled here).
	//

	//
	// Set up a custom properties widget if necessary.
	//

	if (d_custom_properties_widget)
	{
		delete d_custom_properties_widget.get();
		d_custom_properties_widget = boost::none;
	}

	// See if the current feature type needs a custom properties widget.
	boost::optional<GPlatesQtWidgets::AbstractCustomPropertiesWidget *> custom_properties_widget =
			get_custom_properties_widget(
					d_choose_feature_type_widget->get_feature_type(),
					*d_application_state_ptr,
					this);
	if (custom_properties_widget)
	{
		d_custom_properties_widget = custom_properties_widget.get();
		set_up_custom_properties_page();
	}

	//
	// Update the feature type description widget.
	//

	// Get the feature type description text.
	QString feature_type_description_string;
	if (d_choose_feature_type_widget->get_feature_type())
	{
		// Get the GPGIM feature class.
		boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
				GPlatesModel::Gpgim::instance().get_feature_class(d_choose_feature_type_widget->get_feature_type().get());
		if (gpgim_feature_class)
		{
			feature_type_description_string = gpgim_feature_class.get()->get_feature_description();
		}
	}

	// Set the feature type description QTextEdit.
	d_feature_type_description_widget->setPlainText(feature_type_description_string);
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


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesQtWidgets::CreateFeatureDialog::create_feature(
        const GPlatesModel::FeatureType feature_type,
        const CreateFeaturePropertiesPage::property_seq_type feature_properties,
        const GPlatesModel::PropertyName geometry_property_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Create a feature with no properties (yet).
	GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			GPlatesModel::FeatureHandle::create(d_feature_type.get());

	// Add all feature properties (including the common properties) to the feature.
	BOOST_FOREACH(
			const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property,
			feature_properties)
	{
		// Add a clone of the property so we can re-use the properties for the next feature.
		feature->add(feature_property->deep_clone());
	}
	
	// Add the (reconstruction-time) geometry property to the feature.
	const boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property_iterator =
			add_geometry_property(feature->reference(), geometry_property_name);
	if ( ! geometry_property_iterator)
	{
		throw InvalidPropertyValueException(GPLATES_EXCEPTION_SOURCE, 
				tr("There was an error creating the geometry."));
	}

	// Reverse-reconstruct the geometry (just added) back to present day.
	// This does nothing for topological geometries since they reference another feature's geometry.
	if ( ! reverse_reconstruct_geometry_property(feature->reference(), geometry_property_iterator.get(), feature_collection))
	{
		throw InvalidPropertyValueException(GPLATES_EXCEPTION_SOURCE, 
				tr("There was an error reverse-reconstructing the geometry to create the feature. Please check that it has a valid plate ID."));
	}
	
	return feature;
}


boost::optional<GPlatesModel::FeatureHandle::iterator>
GPlatesQtWidgets::CreateFeatureDialog::add_geometry_property(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::PropertyName &geometry_property_name)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_geometry_property_value,
			GPLATES_ASSERTION_SOURCE);

	// Get and clone the geometry property value, since we are potentially re-using it
	// for a conjugate feature. 
	GPlatesModel::PropertyValue::non_null_ptr_type geometry_property_value =
	        d_geometry_property_value.get()->deep_clone_as_prop_val();

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
					feature->feature_type(),
					true/*check_property_value_type*/,
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
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
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

	//
    // Un-Reconstruct the temporary geometry so that it's coordinates are
    // expressed in terms of present day location, given the plate ID that is associated
    // with it and the current reconstruction time.
    //

	// Get the reconstruct layers (if any) that reconstruct the feature collection that our
	// feature will be added to.
	std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_outputs;
	GPlatesAppLogic::LayerProxyUtils::find_reconstruct_layer_outputs_of_feature_collection(
			reconstruct_layer_outputs,
			feature_collection,
			d_application_state_ptr->get_reconstruct_graph());

	// Use the feature properties added so far to the new feature to determine how to
	// reconstruct the geometry back to present-day.
	// This takes advantage of the reconstruct-method framework and avoids a bunch of if-else
	// statements here.
	// NOTE: The feature must have a geometry property present (even if it's not the correct present
	// day geometry) because some reconstruct methods will only be chosen if a geometry is present.
	//
	// If we found a reconstruct layer then use its reconstruct method context (contains the
	// reconstruct parameters, reconstruction tree creator and optional deformation).
    // This could fail if feature collection is reconstructed by multiple layers (for example if the user wants
    // to reconstruct the same features using two different reconstruction trees).
	// We could detect this case and ask the user which reconstruction tree to use (for reverse reconstructing).
	//
	// If there's no reconstruct layers then use the default reconstruction tree creator and
	// default reconstruct parameters.
    // This can happen if the user is adding the created feature to a new feature collection
    // in which case we cannot know which reconstruction tree they will choose when they wrap
    // the new feature collection in a new layer. Although when the new feature collection is
    // created it will automatically create a new layer and set the "default" reconstruction tree
    // layer as its input (where 'default' will probably be the most recently created
    // reconstruction tree layer that is currently active). So currently we just use this
    // reconstruction tree layer. But this is not ideal because the user may then immediately switch
	// to a different reconstruction tree input layer and our reverse reconstruction will not be the
	// one they wanted.
	//
    // TODO: So in summary:
    // * if adding feature to an existing feature collection:
    //   * if feature collection is being processed by only one layer then reverse reconstruct
    //     using that layer,
    //   * if feature collection is being processed by more than one layer then gather the
    //     those layers and ask user which one to reverse reconstruct with,
    // * if adding feature to a new feature collection gather all reconstruction tree layers
    //   including inactive ones and ask user which one to use for the new layer that will
    //   wrap the new feature collection.
    //
	const GPlatesAppLogic::Reconstruction &reconstruction = d_application_state_ptr->get_current_reconstruction();
	const GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
			!reconstruct_layer_outputs.empty()
			? GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
					reconstructed_geometry.get(),
					reconstruct_method_registry,
					feature,
					reconstruction.get_reconstruction_time(),
					reconstruct_layer_outputs.front()->get_reconstruct_method_context(),
					true/*reverse_reconstruct*/)
			: GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
					reconstructed_geometry.get(),
					reconstruct_method_registry,
					feature,
					reconstruction.get_reconstruction_time(),
					reconstruction.get_default_reconstruction_layer_output()->get_reconstruction_tree_creator(),
					GPlatesAppLogic::ReconstructParams(),
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
GPlatesQtWidgets::CreateFeatureDialog::create_feature_link(
        GPlatesModel::FeatureHandle::non_null_ptr_type feature,
        const GPlatesModel::PropertyName &property_name,
        GPlatesModel::FeatureHandle::non_null_ptr_to_const_type target_feature)
{
	GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type target_reference =
			GPlatesPropertyValues::GpmlFeatureReference::create(
					target_feature->feature_id(), target_feature->feature_type());

	GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> link_to_target =
			GPlatesModel::ModelUtils::create_top_level_property(
					property_name,
					target_reference,
					feature->feature_type(),
					true/*check_property_value_type*/,
					&error_code);
	if ( ! link_to_target)
	{
		// Not successful in creating property; shouldn't happen.
		QMessageBox::warning(this,
				QObject::tr("Failed to create a feature reference property."),
				QObject::tr(GPlatesModel::ModelUtils::get_error_message(error_code)) +
						" " + QObject::tr("Property '") +
						convert_qualified_xml_name_to_qstring(property_name) +
						QObject::tr("' will not be added to the feature."),
				QMessageBox::Ok);
		return;
	}
	feature->add(link_to_target.get());
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_canvas_tool_triggered(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	d_canvas_tool_last_chosen_by_user = std::make_pair(workflow, tool);
}
