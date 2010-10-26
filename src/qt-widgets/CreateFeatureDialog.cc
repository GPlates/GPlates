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
#include "EditPlateIdWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditStringWidget.h"
#include "ChooseGeometryPropertyWidget.h"
#include "InformationDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
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
	d_listwidget_geometry_destinations(
			new ChooseGeometryPropertyWidget(
				SelectionWidget::Q_LIST_WIDGET,
				this)),
	d_recon_method(GPlatesAppLogic::ReconstructionMethod::BY_PLATE_ID)
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
	
	// Reconfigure some accelerator keys that conflict.
	d_plate_id_widget->label()->setText(tr("Plate &ID:"));
	d_conjugate_plate_id_widget->label()->setText(tr("C&onjugate ID:"));
	// Conjugate Plate IDs are optional.
	d_conjugate_plate_id_widget->set_null_value_permitted(true);
	d_conjugate_plate_id_widget->reset_widget_to_default_values();
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);
	
	//Add reconstruction method combobox
	QHBoxLayout *recon_method_layout;
	QLabel * recon_method_label;
	recon_method_label = new QLabel(this);
	d_recon_method_combobox->insertItem(GPlatesAppLogic::ReconstructionMethod::BY_PLATE_ID, tr("By Plate ID"));
	d_recon_method_combobox->insertItem(GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION, tr("Half Stage Rotation"));
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
	right_and_left_plate_id_layout->addWidget(d_right_plate_id);
	right_and_left_plate_id_layout->addWidget(d_left_plate_id);
	d_left_plate_id->setVisible(false);
	d_right_plate_id->setVisible(false);

	QVBoxLayout *edit_layout;
	edit_layout = new QVBoxLayout;
	edit_layout->addItem(recon_method_layout);
	edit_layout->addItem(plate_id_layout);
	edit_layout->addItem(right_and_left_plate_id_layout);
	edit_layout->addWidget(d_time_period_widget);
	edit_layout->addWidget(d_name_widget);
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


void
GPlatesQtWidgets::CreateFeatureDialog::handle_prev()
{
	int prev_index = stack->currentIndex() - 1;
	if (prev_index >= 0) {
		stack->setCurrentIndex(prev_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_next()
{
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
			break;

	case 2:
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
		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
			d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
			(collection_file_iter.first).get_file().get_feature_collection();

		// Actually create the Feature!
		GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(collection, type);

		// Add a (possibly ConstantValue-wrapped, see GeometricPropertyValueConstructor)
		// Geometry Property using present-day geometry.
		if (!topological)
		{
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
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							d_geometry_opt_ptr.get(),
							d_plate_id_widget->create_integer_plate_id_from_widget(),
							*d_application_state_ptr->get_current_reconstruction()
									.get_default_reconstruction_tree(),
							true /*reverse_reconstruct*/);

			// Create a property value using the present-day GeometryOnSphere and optionally
			// wrap with a GpmlConstantValue wrapper.
			const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
					GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
							present_day_geometry,
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
		if (GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION == d_recon_method)
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

		// Ensure a layer gets created for the new feature.
		d_application_state_ptr->update_layers(collection_file_iter.first);
		
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
		case GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION:
			d_plate_id_widget->setVisible(false);
			d_conjugate_plate_id_widget->setVisible(false);
			d_right_plate_id->setVisible(true);
			d_left_plate_id->setVisible(true);
			d_recon_method = GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION;
			break;
		
		case GPlatesAppLogic::ReconstructionMethod::BY_PLATE_ID:
			d_right_plate_id->setVisible(false);
			d_left_plate_id->setVisible(false);
			d_plate_id_widget->setVisible(true);
			d_conjugate_plate_id_widget->setVisible(
					should_offer_conjugate_plate_id_prop(
							d_choose_feature_type_widget));
			d_recon_method = GPlatesAppLogic::ReconstructionMethod::BY_PLATE_ID;
			break;
	}
}

