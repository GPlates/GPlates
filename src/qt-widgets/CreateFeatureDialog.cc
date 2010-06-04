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

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <map>
#include <QSet>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QDebug>

#include "CreateFeatureDialog.h"

#include "ChooseFeatureCollectionWidget.h"
#include "ChooseFeatureTypeWidget.h"
#include "EditPlateIdWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditStringWidget.h"
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


namespace
{
	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;


	/**
	 * Subclass of QListWidgetItem so that we can display QualifiedXmlNames in the QListWidget
	 * without converting them to a QString (and thus forgetting that we had a QualifiedXmlName
	 * in the first place).
	 */
	class PropertyNameItem :
			public QListWidgetItem
	{
	public:
		PropertyNameItem(
				const GPlatesModel::PropertyName name,
				const QString &display_name,
				bool expects_time_dependent_wrapper_):
			QListWidgetItem(display_name),
			d_name(name),
			d_expects_time_dependent_wrapper(expects_time_dependent_wrapper_)
		{  }
	
		const GPlatesModel::PropertyName
		get_name()
		{
			return d_name;
		}

		bool
		expects_time_dependent_wrapper()
		{
			return d_expects_time_dependent_wrapper;
		}

	private:
		const GPlatesModel::PropertyName d_name;
		bool d_expects_time_dependent_wrapper;
	};


	/**
	 * Fill the list with possible property names we can assign geometry to.
	 */
	void
	populate_geometry_destinations_list(
			QListWidget &list_widget,
			GPlatesModel::FeatureType target_feature_type)
	{
		typedef GPlatesModel::GPGIMInfo::geometry_prop_name_map_type geometry_prop_name_map_type;
		static const geometry_prop_name_map_type geometry_prop_names =
				GPlatesModel::GPGIMInfo::get_geometry_prop_name_map();
		typedef GPlatesModel::GPGIMInfo::geometry_prop_timedependency_map_type geometry_prop_timedependency_map_type;
		static const geometry_prop_timedependency_map_type geometry_time_dependencies =
				GPlatesModel::GPGIMInfo::get_geometry_prop_timedependency_map();
		// FIXME: This list should ideally be dynamic, depending on:
		//  - the type of GeometryOnSphere we are given (e.g. gpml:position for gml:Point)
		//  - the type of feature the user has selected in the first list (since different
		//    feature types are supposed to have a different selection of valid properties)
		list_widget.clear();
		// Iterate over the feature_type_info_table, and add all property names
		// that match our desired feature.
		typedef GPlatesModel::GPGIMInfo::feature_geometric_prop_map_type feature_geometric_prop_map_type;
		static const feature_geometric_prop_map_type map =
			GPlatesModel::GPGIMInfo::get_feature_geometric_prop_map();
		feature_geometric_prop_map_type::const_iterator it = map.lower_bound(target_feature_type);
		feature_geometric_prop_map_type::const_iterator end = map.upper_bound(target_feature_type);
		for ( ; it != end; ++it) {
			GPlatesModel::PropertyName property = it->second;
			// Display name defaults to the QualifiedXmlName.
			QString display_name = GPlatesUtils::make_qstring_from_icu_string(property.build_aliased_name());
			geometry_prop_name_map_type::const_iterator display_name_it = geometry_prop_names.find(property);
			if (display_name_it != geometry_prop_names.end()) {
				display_name = display_name_it->second;
			}
			// Now we have to look up the time-dependent flag somewhere, too.
			bool expects_time_dependent_wrapper = true;
			geometry_prop_timedependency_map_type::const_iterator time_dependency_it = geometry_time_dependencies.find(property);
			if (time_dependency_it != geometry_time_dependencies.end()) {
				expects_time_dependent_wrapper = time_dependency_it->second;
			}
			// Finally, add the item to the QListWidget.
			list_widget.addItem(new PropertyNameItem(property, display_name, expects_time_dependent_wrapper));
		}
		list_widget.setCurrentRow(0);
	}


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
		boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
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
	d_choose_feature_type_widget(new ChooseFeatureTypeWidget(this)),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
				d_file_state,
				d_file_io,
				this))
{
	setupUi(this);

	// Add sub-widgets to placeholders.
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_type_widget,
			widget_choose_feature_type_placeholder);
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);
	
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
	d_choose_feature_type_widget->initialise(d_creation_type == TOPOLOGICAL);
	
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
	QObject::connect(listwidget_geometry_destinations, SIGNAL(itemActivated(QListWidgetItem *)),
			d_plate_id_widget, SLOT(setFocus()));
	// The various Edit widgets need pass focus along the chain if Enter is pressed.
	QObject::connect(d_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::connect(d_time_period_widget, SIGNAL(enter_pressed()),
			d_name_widget, SLOT(setFocus()));
	QObject::connect(d_name_widget, SIGNAL(enter_pressed()),
			button_next, SLOT(setFocus()));
	
	// Reconfigure some accelerator keys that conflict.
	d_plate_id_widget->label()->setText(tr("Plate &ID:"));
	d_conjugate_plate_id_widget->label()->setText(tr("C&onjugate ID:"));
	// Conjugate Plate IDs are optional.
	d_conjugate_plate_id_widget->set_null_value_permitted(true);
	d_conjugate_plate_id_widget->reset_widget_to_default_values();
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);
	
	// Create the edit widgets we'll need, and add them to the Designer-created widgets.
	QHBoxLayout *plate_id_layout;
	plate_id_layout = new QHBoxLayout;
	plate_id_layout->setSpacing(2);
	plate_id_layout->setMargin(0);
	plate_id_layout->addWidget(d_plate_id_widget);
	plate_id_layout->addWidget(d_conjugate_plate_id_widget);

	QVBoxLayout *edit_layout;
	edit_layout = new QVBoxLayout;
	edit_layout->addItem(plate_id_layout);
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
	// Pushing Enter or double-clicking should cause the buttonbox to focus.
	QObject::connect(d_choose_feature_collection_widget, SIGNAL(item_activated()),
			buttonbox, SLOT(setFocus()));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_geometric_property_list()
{
	// Get the FeatureType the user has selected.
	boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
		d_choose_feature_type_widget->get_feature_type();
	if ( ! feature_type_opt) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	// Populate the listwidget_geometry_destinations based on what is legal right now.
	populate_geometry_destinations_list(*listwidget_geometry_destinations, *feature_type_opt);
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
			listwidget_geometry_destinations->setFocus();
			d_button_create->setEnabled(false);
			button_create_and_save->setEnabled(false);
			d_conjugate_plate_id_widget->setVisible(
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
	PropertyNameItem *geom_prop_name_item = dynamic_cast<PropertyNameItem *>(
			listwidget_geometry_destinations->currentItem());
	if (geom_prop_name_item == NULL)
	{
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geom_prop_name = geom_prop_name_item->get_name();
	
	// Get the FeatureType the user has selected.
	boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
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
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_iterator, bool> collection_file_iter =
			d_choose_feature_collection_widget->get_file_iterator();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
			(collection_file_iter.first)->get_feature_collection();

		// Actually create the Feature!
		GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(collection, type);

		// Add a (possibly ConstantValue-wrapped, see GeometricPropertyValueConstructor)
		// Geometry Property using present-day geometry.
		if (!topological)
		{
			bool geom_prop_needs_constant_value = geom_prop_name_item->expects_time_dependent_wrapper();

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
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
					GPlatesAppLogic::ReconstructUtils::reconstruct(
							d_geometry_opt_ptr.get(),
							d_plate_id_widget->create_integer_plate_id_from_widget(),
							d_application_state_ptr->get_current_reconstruction().reconstruction_tree(),
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

		// We've just modified the feature collection so let the feature collection file state
		// know this so it can reclassify it.
		// TODO: This is not ideal since we have to manually call this whenever a feature in
		// the feature collection is modified - remove this call when feature/feature-collection
		// callbacks have been implemented and then utilised inside FeatureCollectionFileState to
		// listen on feature collection changes.
		d_file_state.reclassify_feature_collection(collection_file_iter.first);
		
		emit feature_created(feature);
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

