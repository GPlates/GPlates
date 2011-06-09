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

#include "ChooseFeatureCollectionWidget.h"
#include "ChooseFeatureTypeWidget.h"
#include "ChooseGeometryPropertyWidget.h"
#include "EditPlateIdWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditStringWidget.h"
#include "FlowlinePropertiesWidget.h"
#include "InformationDialog.h"
#include "MotionPathPropertiesWidget.h"
#include "QtWidgetUtils.h"
#include "TopologyNetworkPropertiesWidget.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/types.h"
#include "model/Model.h"
#include "model/PropertyName.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/GPGIMInfo.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ViewState.h"

#include "utils/UnicodeStringUtils.h"

#include "property-values/Enumeration.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;


	/**
	 * Constructs the set used in @a should_offer_conjugate_plate_id_prop().
	 * Quick and dirty. Replace with proper GPGIM-querying thing.
	 */
	const QSet<QString> &
	build_feature_conjugate_plate_prop_set()
	{
		static QSet<QString> set;
		set << "gpml:Isochron" << "gpml:MidOceanRidge" << "gpml:ContinentalRift" << 
				"gpml:SubductionZone" << "gpml:OrogenicBelt" << "gpml:Transform" <<
				"gpml:FractureZone" << "gpml:PassiveContinentalBoundary";
		return set;
	}

	/**
	 * Quick and dirty way to ascertain whether we should present the user
	 * with a conjugatePlateId property value edit widget. This is based on
	 * FeatureType. Ideally, we'd be querying an internal GPGIM structure
	 * for all of these things.
	 */
	bool
	should_offer_conjugate_plate_id_prop(
			const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget)
	{
		// Build set of feature types that allow conjugate plate id.
		static const QSet<QString> &conjugate_feature_set =
				build_feature_conjugate_plate_prop_set();
		
		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
				choose_feature_type_widget->get_feature_type();
		if (feature_type_opt) {
			QString t = GPlatesUtils::make_qstring_from_icu_string(feature_type_opt->build_aliased_name());
			return conjugate_feature_set.contains(t);
		} else {
			return false;
		}
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
	 * Creates an isochron feature using the provided geometry and properties,
	 * but reversing the plate-id and conjugate-plate-id properties. 
	 *
	 * The geometry will be reconstructed to present day given its new plate-id,
	 * i.e. the conjugate-plate-id passed to this function.
	 */
	void
	create_conjugate_isochron(
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection, 
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
		const GPlatesModel::PropertyName &geom_prop_name,
		const GPlatesQtWidgets::EditPlateIdWidget *plate_id_widget,
		const GPlatesQtWidgets::EditPlateIdWidget *conjugate_plate_id_widget,
		const GPlatesQtWidgets::EditTimePeriodWidget *time_period_widget,
		const GPlatesQtWidgets::EditStringWidget *name_widget,
		GPlatesAppLogic::ApplicationState *application_state_ptr)
	{
		bool geom_prop_needs_constant_value =
				GPlatesModel::GPGIMInfo::expects_time_dependent_wrapper(geom_prop_name);

		// Un-Reconstruct the temporary geometry so that it's coordinates are
		// expressed in terms of present day location, given the plate ID that is associated
		// with it and the current reconstruction time.
		// 
		// Note that we're reversing the plate-id and conjugate-plate-ids which have been 
		// passed to this function, so we use the conjugate here.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
			GPlatesAppLogic::ReconstructUtils::reconstruct(
			geometry,
			conjugate_plate_id_widget->create_integer_plate_id_from_widget(),
			*application_state_ptr->get_current_reconstruction()
				.get_default_reconstruction_layer_output()->get_reconstruction_tree(),
			true /*reverse_reconstruct*/);

		// Create a property value using the present-day GeometryOnSphere and optionally
		// wrap with a GpmlConstantValue wrapper.
		const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
			present_day_geometry,
			geom_prop_needs_constant_value);


		if (!geometry_value_opt)
		{
			return;
		}
		
		// Actually create the Feature!
		GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(collection,		
					GPlatesModel::FeatureType::create_gpml("Isochron"));

		// Add a (ConstantValue-wrapped) gpml:reconstructionPlateId Property.
		GPlatesModel::PropertyValue::non_null_ptr_type plate_id_value =
			conjugate_plate_id_widget->create_property_value_from_widget();
		GPlatesPropertyValues::TemplateTypeParameterType plate_id_value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId");
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			GPlatesPropertyValues::GpmlConstantValue::create(plate_id_value, plate_id_value_type)));

		// Add a gpml:conjugatePlateId Property.
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
				plate_id_widget->create_property_value_from_widget()));

		// Add a gml:validTime Property.
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gml("validTime"),
			time_period_widget->create_property_value_from_widget()));

		// Add a gml:name Property.
		// FIXME: we should give the user the chance to enter a new name.
		QString name_string = name_widget->get_string();
		QString new_string_name = name_string.prepend("Conjugate of ");
			
		GPlatesModel::PropertyValue::non_null_ptr_type name_property_value = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(new_string_name));			
			
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			GPlatesModel::PropertyName::create_gml("name"),
			name_property_value));
			


		// Add a (possibly ConstantValue-wrapped, see GeometricPropertyValueConstructor)
		// Geometry Property using present-day geometry.
		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			geom_prop_name,
			*geometry_value_opt));			
	}

	/**
	 * Set some default states and/or restrictions on the reconstruction method, depending on the selected feature type.
	 *
	 * Flowline feature types will be reconstructed as HALF_STAGE_ROTATION.
	 *
	 * Motion track types will be set to BY_PLATE_ID. (Later we should allow changing to HALF_STAGE_ROTATION; the
	 * MotionPathGeometryPopulator won't currently handle this correctly, so disable this option until we do handle
	 * it).
	 *
	 * MOR types will be set to HALF_STAGE_ROTATION but can be changed to BY_PLATE_ID.
	 */
	void
	set_recon_method_state(
		QComboBox *recon_method_combo_box,
		const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget)
	{
		static const GPlatesModel::FeatureType flowline_type = GPlatesModel::FeatureType::create_gpml("Flowline");
		static const GPlatesModel::FeatureType motion_path_type = GPlatesModel::FeatureType::create_gpml("MotionPath");
		static const GPlatesModel::FeatureType mor_type = GPlatesModel::FeatureType::create_gpml("MidOceanRidge");

		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			choose_feature_type_widget->get_feature_type();

		if (feature_type_opt && *feature_type_opt == flowline_type) {
			recon_method_combo_box->setCurrentIndex(
				GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
			recon_method_combo_box->setEnabled(false);
		}
		else if (feature_type_opt && *feature_type_opt == motion_path_type)
		{
			recon_method_combo_box->setCurrentIndex(
				GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			recon_method_combo_box->setEnabled(false);
		}
		else if (feature_type_opt && *feature_type_opt == mor_type)
		{
			recon_method_combo_box->setCurrentIndex(
				GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION);
			recon_method_combo_box->setEnabled(true);
		}
		else {
			recon_method_combo_box->setCurrentIndex(
				GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID);
			recon_method_combo_box->setEnabled(true);
		}
	}

	/**
	 * Constructs a set of customisable feature types, i.e. those to 
	 * which we might add additional properties, or those where additional tasks
	 * are required during creation of the feature.       
	 *
	 * Admittedly it's a very short list at the moment.                                                             
	 */
	const QSet<QString> &
	build_customisable_feature_type_set()
	{
		static QSet<QString> set;
		set << "gpml:Flowline"
			<< "gpml:MotionPath" 
			<< "gpml:TopologicalNetwork" 
			;
		return set;
	}

	/**
	 * Returns whether or not the provided feature type is customisable.                                                                     
	 */	
	bool
	feature_is_customisable(
		const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget)
	{
		// Build set of feature types that are customisable.
		static const QSet<QString> &customisable_feature_set =
			build_customisable_feature_type_set();

		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			choose_feature_type_widget->get_feature_type();
		if (feature_type_opt) {
			QString t = GPlatesUtils::make_qstring_from_icu_string(feature_type_opt->build_aliased_name());
			return customisable_feature_set.contains(t);
		} else {
			return false;
		}	
	}

	/**
	 * Creates a custom properties widget appropriate to the feature type selected in 
	 * the @a choose_feature_type_widget.
	 *
	 * Returns NULL if the feature has no corresponding custom properties widget.
	 *
	 * Currently we only have FlowlinePropertiesWidget and MotionPathPropertiesWidget;
	 * all other feature types will return NULL.
	 */
	GPlatesQtWidgets::AbstractCustomPropertiesWidget *
	get_appropriate_custom_widget(
		const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget,
                GPlatesAppLogic::ApplicationState *application_state_ptr,
                GPlatesQtWidgets::CreateFeatureDialog *create_feature_dialog_ptr)
	{
		static const GPlatesModel::FeatureType flowline_type = GPlatesModel::FeatureType::create_gpml("Flowline");
		static const GPlatesModel::FeatureType motion_path_type = GPlatesModel::FeatureType::create_gpml("MotionPath");
		static const GPlatesModel::FeatureType topology_network_type = GPlatesModel::FeatureType::create_gpml("TopologicalNetwork");

		// Get currently selected feature type
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			choose_feature_type_widget->get_feature_type();

		if (feature_type_opt)
		{
			if (*feature_type_opt == flowline_type)
			{
				return new GPlatesQtWidgets::FlowlinePropertiesWidget(application_state_ptr,
					create_feature_dialog_ptr);
			}
			else if (*feature_type_opt == motion_path_type)
			{
				return new GPlatesQtWidgets::MotionPathPropertiesWidget(application_state_ptr);
			}
			else if (*feature_type_opt == topology_network_type)
			{
				return new GPlatesQtWidgets::TopologyNetworkPropertiesWidget(application_state_ptr);
			}
		}

		return NULL;

	}	

	bool
	flowline_selected(
		const GPlatesQtWidgets::ChooseFeatureTypeWidget *choose_feature_type_widget)
	{
	    static const GPlatesModel::FeatureType flowline_type = GPlatesModel::FeatureType::create_gpml("Flowline");

	    // Get currently selected feature type
	    boost::optional<GPlatesModel::FeatureType> feature_type_opt =
		    choose_feature_type_widget->get_feature_type();

	    return (feature_type_opt && (*feature_type_opt == flowline_type));
	}
}



GPlatesQtWidgets::CreateFeatureDialog::CreateFeatureDialog(
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		FeatureType creation_type,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_model_ptr(view_state_.get_application_state().get_model_interface()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io()),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_viewport_window_ptr(&viewport_window_),
	d_creation_type(creation_type),
	d_geometry_opt_ptr(boost::none),
	d_plate_id_widget(new EditPlateIdWidget(this)),
	d_conjugate_plate_id_widget(new EditPlateIdWidget(this)),
	d_time_period_widget(new EditTimePeriodWidget(this)),
	d_name_widget(new EditStringWidget(this)),
	d_choose_feature_type_widget(
			new ChooseFeatureTypeWidget(
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
				d_file_state,
				d_file_io,
				this)),
	d_recon_method_combobox(new QComboBox(this)),
	d_right_plate_id(new EditPlateIdWidget(this)),
	d_left_plate_id(new EditPlateIdWidget(this)),
	d_custom_properties_widget(NULL),
	d_listwidget_geometry_destinations(
			new ChooseGeometryPropertyWidget(
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_recon_method(GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID),
	d_customisable_feature_type_selected(false),
	d_create_conjugate_isochron_checkbox(new QCheckBox(this))
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
	
	set_up_button_box();
	
	set_up_feature_type_page();
	set_up_feature_properties_page();
	set_up_feature_collection_page();
		
	// When the current page is changed, we need to enable and disable some buttons.
	QObject::connect(stack, SIGNAL(currentChanged(int)),
			this, SLOT(handle_page_change(int)));
	// Send a fake page change event to ensure buttons are set up properly at start.
	handle_page_change(0);
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
	// Populate list of feature types.
	bool topological = (d_creation_type == TOPOLOGICAL);
	d_choose_feature_type_widget->populate(topological);

	// Default to gpml:UnclassifiedFeature for non-topological features.
	if (!topological)
	{
		static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE =
			GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		d_choose_feature_type_widget->set_feature_type(UNCLASSIFIED_FEATURE);
	}
	
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
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_properties_page()
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
	recon_method_layout = new QHBoxLayout;
	QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d_recon_method_combobox->setSizePolicy(sizePolicy1);
	recon_method_label->setText(tr("Reconstruction Method:"));
	recon_method_layout->addWidget(recon_method_label);
	recon_method_layout->addWidget(d_recon_method_combobox);
	recon_method_layout->setSpacing(6);

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
	edit_layout->addItem(recon_method_layout);
	edit_layout->addItem(plate_id_layout);
	edit_layout->addItem(right_and_left_plate_id_layout);
	edit_layout->addWidget(d_time_period_widget);
	edit_layout->addWidget(d_name_widget);
	edit_layout->addWidget(d_create_conjugate_isochron_checkbox);
	edit_layout->insertStretch(-1);
	groupbox_properties->setLayout(edit_layout);
	
	// Note that the geometric properties list must be populated dynamically
	// on page change, see handle_page_change() below.
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_collection_page()
{
	// Pushing Enter or double-clicking should be the same as clicking create.
	QObject::connect(d_choose_feature_collection_widget, SIGNAL(item_activated()),
			this, SLOT(handle_create()));
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
	// Populate the listwidget_geometry_destinations based on what is legal right now.
	d_listwidget_geometry_destinations->populate(*feature_type_opt);
}


bool
GPlatesQtWidgets::CreateFeatureDialog::set_geometry_and_display(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_)
{
	// The geometry is passed in as a GeometryOnSphere::non_null_ptr_to_const_type
	// because we want to enforce that this dialog should be given valid geometry
	// if you want it to display itself. However, we must set our d_geometry_ptr member
	// as a boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, because
	// it cannot be initialised with any meaningful value at this dialog's creation time.
	d_geometry_opt_ptr = geometry_;
	
	if ( ! d_geometry_opt_ptr) {
		// We shouldn't let the dialog open without a valid geometry.
		return false;
	}
	
	// Set the stack back to the first page.
	stack->setCurrentIndex(0);
	// The Feature Collections list needs to be repopulated each time.
	d_choose_feature_collection_widget->initialise();
	
	// Show the dialog modally.
	return exec();
}


bool
GPlatesQtWidgets::CreateFeatureDialog::display()
{
	// Set the stack back to the first page.
	stack->setCurrentIndex(0);
	// The Feature Collections list needs to be repopulated each time.
	d_choose_feature_collection_widget->initialise();
	
	// Show the dialog modally.
	return exec();
}

bool
GPlatesQtWidgets::CreateFeatureDialog::display(int index)
{
	// Set the stack back to the first page.
	stack->setCurrentIndex(0);
	// The Feature Collections list needs to be repopulated each time.
	d_choose_feature_collection_widget->initialise();

#if 0
	// FIXME
	// set the items on the list
	static const GPlatesModel::FeatureType UNCLASSIFIED_FEATURE =
		GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
	d_choose_feature_type_widget->set_feature_type(UNCLASSIFIED_FEATURE);
#endif
	
	// Show the dialog modally.
	return exec();
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_prev()
{
	int index_decrement = 1;
	if ((stack->currentIndex() == COLLECTION_PAGE) && !d_customisable_feature_type_selected)
	{
		index_decrement = 2;		
	}

	int prev_index = stack->currentIndex() - index_decrement;
	if (prev_index < stack->count()) {
		stack->setCurrentIndex(prev_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_next()
{
	int index_increment = 1;
	if ((stack->currentIndex() == PROPERTIES_PAGE) && !d_customisable_feature_type_selected)
	{
		index_increment = 2;		
	}

	int next_index = stack->currentIndex() + index_increment;
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
	case 0:
		d_choose_feature_type_widget->setFocus();
		button_prev->setEnabled(false);
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		break;

	case 1:
		// Populate the listwidget_geometry_destinations based on what is legal right now.
		set_up_geometric_property_list();
		d_listwidget_geometry_destinations->setFocus();
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		d_conjugate_plate_id_widget->setVisible(
			d_recon_method_combobox->currentIndex() == 0 /* plate id */ &&
			should_offer_conjugate_plate_id_prop(
			d_choose_feature_type_widget));
		d_create_conjugate_isochron_checkbox->setVisible(
			should_offer_create_conjugate_isochron_checkbox(
			d_choose_feature_type_widget));
		set_recon_method_state(
			d_recon_method_combobox,
			d_choose_feature_type_widget);
		break;

	case 2:
		// We should only be here if we've chosen a feature type for
		// which there is a custom feature properties dialog. 
		d_button_create->setEnabled(false);
		button_create_and_save->setEnabled(false);
		d_custom_properties_widget->update();
		break;
	case 3:
		d_choose_feature_collection_widget->setFocus();
		button_next->setEnabled(false);
		break;
	}
}


// FIXME: This method is getting too long.
void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
{
	bool topological = (d_creation_type == TOPOLOGICAL);

	// Get the PropertyName the user has selected for geometry to go into.
	boost::optional<GPlatesModel::PropertyName> geom_prop_name_item =
		d_listwidget_geometry_destinations->get_property_name();
	if (!geom_prop_name_item)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geom_prop_name = *geom_prop_name_item;
	
	// Get the FeatureType the user has selected.
	boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			d_choose_feature_type_widget->get_feature_type();
	if (!feature_type_opt)
	{
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType type = *feature_type_opt;

	try
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state_ptr->get_model_interface().access_model());

		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
			d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
			(collection_file_iter.first).get_file().get_feature_collection();

		// Actually create the Feature!
		GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(collection, type);



		// Add a (ConstantValue-wrapped) gpml:reconstructionPlateId Property.
		GPlatesModel::PropertyValue::non_null_ptr_type plate_id_value =
				d_plate_id_widget->create_property_value_from_widget();
		GPlatesPropertyValues::TemplateTypeParameterType plate_id_value_type =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId");
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GPlatesPropertyValues::GpmlConstantValue::create(plate_id_value, plate_id_value_type)));

		// If we are using half stage rotation, add right and left plate id.
		if (GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION == d_recon_method)
		{
			feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("rightPlate"),
							d_right_plate_id->create_property_value_from_widget()));
			feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("leftPlate"),
							d_left_plate_id->create_property_value_from_widget()));

			const GPlatesPropertyValues::Enumeration::non_null_ptr_type recon_method_value =
					GPlatesPropertyValues::Enumeration::create(
							"gpml:ReconstructionMethodEnumeration", "HalfStageRotation");
			feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("reconstructionMethod"),
							recon_method_value));
		}

		// Add a gpml:conjugatePlateId Property.
		if (!topological)
		{
			if (!d_conjugate_plate_id_widget->is_null() &&
					should_offer_conjugate_plate_id_prop(d_choose_feature_type_widget))
			{
				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
							d_conjugate_plate_id_widget->create_property_value_from_widget()));
			}
		}
	

		// Add a gml:validTime Property.
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("validTime"),
					d_time_period_widget->create_property_value_from_widget()));

		// Add a gml:name Property.
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("name"),
					d_name_widget->create_property_value_from_widget()));

		if (d_customisable_feature_type_selected)
		{
		    d_custom_properties_widget->add_properties_to_feature(feature);
		}

		if (!topological)
		{
		    add_geometry(feature,geom_prop_name);
		}

		if (d_create_conjugate_isochron_checkbox->isChecked())
		{
			create_conjugate_isochron(
				collection,
				d_geometry_opt_ptr.get(),
				geom_prop_name,
				d_plate_id_widget,
				d_conjugate_plate_id_widget,
				d_time_period_widget,
				d_name_widget,
				d_application_state_ptr
				);
		}
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before we emit signals because we don't
		// know whose listening on those signals and they may be expecting model observers to
		// be up-to-date with the modified model.
		model_notification_guard.release_guard();

		emit feature_created(feature);

		// If the user got into digitisation mode because they clicked the
		// "Clone Geometry" button whilst in the Click Geometry tool, for
		// example, they get taken back to the Click Geometry tool instead of
		// remaining in a digitisation tool.
		d_viewport_window_ptr->restore_canvas_tool_last_chosen_by_user();

		accept();
	}
	catch (const ChooseFeatureCollectionWidget::NoFeatureCollectionSelectedException &)
	{
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_create_and_save()
{
	// do the regular creation process
	handle_create();

	// and now open the manage feature collections dialog
	d_viewport_window_ptr->pop_up_manage_feature_collections_dialog();
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
					should_offer_conjugate_plate_id_prop(
							d_choose_feature_type_widget));
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
	if (feature_is_customisable(d_choose_feature_type_widget))
	{
		if (d_custom_properties_widget != NULL)
		{
			delete d_custom_properties_widget;
		}
		d_custom_properties_widget = get_appropriate_custom_widget(
			d_choose_feature_type_widget,
			d_application_state_ptr,
			this);
		d_customisable_feature_type_selected = true;
		set_up_custom_properties_page();
	}
	else
	{
		delete d_custom_properties_widget;
		d_custom_properties_widget = NULL;
		d_customisable_feature_type_selected = false;
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
	QLayout *layout_ = groupbox_custom->layout();
	if (layout_)
	{
		delete (layout_);
	}

	// Create the edit widgets we'll need, and add them to the Designer-created widgets.
	QGridLayout *custom_layout;
	custom_layout = new QGridLayout;
	custom_layout->setSpacing(0);
	custom_layout->setMargin(4);
	custom_layout->addWidget(d_custom_properties_widget);
	groupbox_custom->setLayout(custom_layout);
}

boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesQtWidgets::CreateFeatureDialog::get_left_plate()
{
    if (!d_left_plate_id->is_null())
    {
        return boost::optional<GPlatesModel::integer_plate_id_type>
                (d_left_plate_id->create_integer_plate_id_from_widget());
    }
    else
    {
        return boost::none;
    }
}

boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesQtWidgets::CreateFeatureDialog::get_right_plate()
{
    if (!d_right_plate_id->is_null())
    {
        return boost::optional<GPlatesModel::integer_plate_id_type>
                (d_right_plate_id->create_integer_plate_id_from_widget());
    }
    else
    {
        return boost::none;
    }
}

void
GPlatesQtWidgets::CreateFeatureDialog::add_geometry(
    GPlatesModel::FeatureHandle::weak_ref &feature,
    const GPlatesModel::PropertyName &geom_prop_name)
{
    // Add a (possibly ConstantValue-wrapped, see GeometricPropertyValueConstructor)
    // Geometry Property using present-day geometry.
    //

    bool geom_prop_needs_constant_value =
	    GPlatesModel::GPGIMInfo::expects_time_dependent_wrapper(geom_prop_name);

    // Check we have a valid GeometryOnSphere supplied from the DigitisationWidget.
    if (!d_geometry_opt_ptr)
    {
		// Should never happen, as we can't open the dialog (legitimately) without geometry!
		QMessageBox::critical(this, tr("No geometry"),
			tr("No geometry was supplied to this dialog. Please try digitising again."));
		// FIXME: Exception.
		reject();
		return;
    }

    if (d_customisable_feature_type_selected)
    {
		d_geometry_opt_ptr.reset(
			d_custom_properties_widget->do_geometry_tasks(
			d_geometry_opt_ptr.get(),
			feature));
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
    boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> present_day_geometry;

	// The default reconstruction tree.
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type default_reconstruction_tree =
			d_application_state_ptr->get_current_reconstruction()
					.get_default_reconstruction_layer_output()->get_reconstruction_tree();
	// A function to get reconstruction trees with.
	GPlatesAppLogic::ReconstructionTreeCreator
			reconstruction_tree_creator =
					GPlatesAppLogic::get_cached_reconstruction_tree_creator(
							default_reconstruction_tree->get_reconstruction_features(),
							default_reconstruction_tree->get_reconstruction_time(),
							default_reconstruction_tree->get_anchor_plate_id());

    // Until we have a reliable MOR reconstruction method, handle flowlines specially.
    if (flowline_selected(d_choose_feature_type_widget))
    {
		present_day_geometry = GPlatesAppLogic::FlowlineUtils::reconstruct_flowline_seed_points(
			d_geometry_opt_ptr.get(),
			default_reconstruction_tree->get_reconstruction_time(),
			reconstruction_tree_creator,
			feature,
			true /* reverse reconstruct */);
    }
    else if (d_recon_method == GPlatesAppLogic::ReconstructMethod::HALF_STAGE_ROTATION)
    {
		present_day_geometry = GPlatesAppLogic::ReconstructUtils::reconstruct_as_half_stage(
		    d_geometry_opt_ptr.get(),
		    d_left_plate_id->create_integer_plate_id_from_widget(),
		    d_right_plate_id->create_integer_plate_id_from_widget(),
		    *default_reconstruction_tree,
		    true /* reverse reconstruct */);
    }
    else
    {
		present_day_geometry = GPlatesAppLogic::ReconstructUtils::reconstruct(
			d_geometry_opt_ptr.get(),
			d_plate_id_widget->create_integer_plate_id_from_widget(),
			*default_reconstruction_tree,
			true /*reverse_reconstruct*/);
    }

    if (present_day_geometry)
    {
		// Create a property value using the present-day GeometryOnSphere and optionally
		// wrap with a GpmlConstantValue wrapper.
		const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
			GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
			present_day_geometry.get(),
			geom_prop_needs_constant_value);

		if (!geometry_value_opt)
		{
			// Might happen, if DigitisationWidget and CreateFeatureDialog (specifically, the
			// GeometricPropertyValueConstructor) disagree on what is implemented and what is not.
			// FIXME: Exception?
			QMessageBox::critical(this, tr("Cannot convert geometry to property value"),
				tr("There was an error converting the digitised geometry to a usable property value."));
			reject();
			return;
		}

		feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
			geom_prop_name,
			*geometry_value_opt));
    }

}
