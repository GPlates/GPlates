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

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <map>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QDebug>

#include "CreateFeatureDialog.h"

#include "EditPlateIdWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditStringWidget.h"
#include "InformationDialog.h"
#include "ViewportWindow.h"
#include "qt-widgets/ApplicationState.h"
#include "model/types.h"
#include "model/PropertyName.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "gui/GeometricPropertyValueConstructor.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	typedef std::multimap<GPlatesModel::FeatureType, GPlatesModel::PropertyName> feature_geometric_prop_map_type;
	typedef feature_geometric_prop_map_type::const_iterator feature_geometric_prop_map_const_iterator;

	typedef std::map<GPlatesModel::PropertyName, QString> geometry_prop_name_map_type;
	typedef geometry_prop_name_map_type::const_iterator geometry_prop_name_map_const_iterator;

	typedef std::map<GPlatesModel::PropertyName, bool> geometry_prop_timedependency_map_type;
	typedef geometry_prop_timedependency_map_type::const_iterator geometry_prop_timedependency_map_const_iterator;
	
	/**
	 * This struct is used to build a static table of all possible geometric properties
	 * we can fill in with this dialog.
	 */
	struct GeometryPropInfo
	{
		/**
		 * The name of the geometric property, without the gpml: prefix.
		 */
		const char *prop_name;
		
		/**
		 * The human-friendly name of the geometric property, made ready
		 * for translation by being wrapped in a QT_TR_NOOP() macro.
		 */
		const char *friendly_name;
		
		/**
		 * Whether the property should have a time-dependent wrapper.
		 */
		bool expects_time_dependent_wrapper;
	};
	
	/**
	 * Information about geometric properties we can fill in with this dialog.
	 */
	static const GeometryPropInfo geometry_prop_info_table[] = {
		{ "centerLineOf", QT_TR_NOOP("Centre line"), true },
		{ "outlineOf", QT_TR_NOOP("Outline"), true },
		{ "errorBounds", QT_TR_NOOP("Error boundary"), false },
		{ "boundary", QT_TR_NOOP("Boundary"), false },
		{ "position", QT_TR_NOOP("Position"), false },
		{ "locations", QT_TR_NOOP("Locations"), false },
		{ "unclassifiedGeometry", QT_TR_NOOP("Unclassified / miscellaneous"), true },
	};

	
	/**
	 * Converts the above table into a map of PropertyName -> QString.
	 */
	const geometry_prop_name_map_type &
	build_geometry_prop_name_map()
	{
		static geometry_prop_name_map_type map;
		// Add all the friendly names from the table.
		const GeometryPropInfo *it = geometry_prop_info_table;
		const GeometryPropInfo *end = it + NUM_ELEMS(geometry_prop_info_table);
		for ( ; it != end; ++it) {
			map.insert(std::make_pair(GPlatesModel::PropertyName::create_gpml(it->prop_name),
					QObject::tr(it->friendly_name) ));
		}
		return map;
	}	


	/**
	 * Converts the above table into a map of PropertyName -> bool.
	 */
	const geometry_prop_timedependency_map_type &
	build_geometry_prop_timedependency_map()
	{
		static geometry_prop_timedependency_map_type map;
		// Add all the expects_time_dependent_wrapper flags from the table.
		const GeometryPropInfo *it = geometry_prop_info_table;
		const GeometryPropInfo *end = it + NUM_ELEMS(geometry_prop_info_table);
		for ( ; it != end; ++it) {
			map.insert(std::make_pair(GPlatesModel::PropertyName::create_gpml(it->prop_name),
					it->expects_time_dependent_wrapper ));
		}
		return map;
	}	


	/**
	 * This struct is used to build a static table of all possible FeatureTypes we can
	 * create with this dialog.
	 */
	struct FeatureTypeInfo
	{
		/**
		 * The name of the feature, without the gpml: prefix.
		 */
		const char *gpml_type;
		
		/**
		 * The name of a geometric property you can associate with this feature.
		 */
		const char *geometric_property;
	};
	
	/**
	 * This list was created by a Python script which processed the maps of
	 *   feature-type -> feature-creation function
	 * and
	 *   feature-creation function -> property-creation function
	 * in "src/file-io/FeaturePropertiesMap.cc" (part of the GPML parser).
	 */
	static const FeatureTypeInfo feature_type_info_table[] = {
		{ "AseismicRidge", "centerLineOf" },
		{ "AseismicRidge", "outlineOf" },
		{ "AseismicRidge", "unclassifiedGeometry" },
		{ "BasicRockUnit", "outlineOf" },
		{ "BasicRockUnit", "unclassifiedGeometry" },
		{ "Basin", "outlineOf" },
		{ "Basin", "unclassifiedGeometry" },
		{ "Bathymetry", "outlineOf" },
		{ "ClosedContinentalBoundary", "boundary" },
		{ "ClosedPlateBoundary", "boundary" },
		{ "Coastline", "centerLineOf" },
		{ "Coastline", "unclassifiedGeometry" },
		{ "ComputationalMesh", "locations" },
		{ "ContinentalFragment", "outlineOf" },
		{ "ContinentalFragment", "unclassifiedGeometry" },
		{ "ContinentalRift", "centerLineOf" },
		{ "ContinentalRift", "outlineOf" },
		{ "ContinentalRift", "unclassifiedGeometry" },
		{ "Craton", "outlineOf" },
		{ "Craton", "unclassifiedGeometry" },
		{ "CrustalThickness", "outlineOf" },
		{ "DynamicTopography", "outlineOf" },
		{ "ExtendedContinentalCrust", "outlineOf" },
		{ "ExtendedContinentalCrust", "unclassifiedGeometry" },
		{ "Fault", "centerLineOf" },
		{ "Fault", "unclassifiedGeometry" },
		{ "FoldPlane", "centerLineOf" },
		{ "FoldPlane", "unclassifiedGeometry" },
		{ "FractureZone", "centerLineOf" },
		{ "FractureZone", "outlineOf" },
		{ "FractureZone", "unclassifiedGeometry" },
		{ "FractureZoneIdentification", "position" },
		{ "GeologicalLineation", "centerLineOf" },
		{ "GeologicalLineation", "unclassifiedGeometry" },
		{ "GeologicalPlane", "centerLineOf" },
		{ "GeologicalPlane", "unclassifiedGeometry" },
		{ "GlobalElevation", "outlineOf" },
		{ "Gravimetry", "outlineOf" },
		{ "HeatFlow", "outlineOf" },
		{ "HotSpot", "position" },
		{ "HotSpot", "unclassifiedGeometry" },
		{ "HotSpotTrail", "errorBounds" },
		{ "HotSpotTrail", "unclassifiedGeometry" },
		{ "InferredPaleoBoundary", "centerLineOf" },
		{ "InferredPaleoBoundary", "errorBounds" },
		{ "InferredPaleoBoundary", "unclassifiedGeometry" },
		{ "IslandArc", "outlineOf" },
		{ "IslandArc", "unclassifiedGeometry" },
		{ "Isochron", "centerLineOf" },
		{ "Isochron", "unclassifiedGeometry" },
		{ "LargeIgneousProvince", "outlineOf" },
		{ "LargeIgneousProvince", "unclassifiedGeometry" },
		{ "MagneticAnomalyIdentification", "position" },
		{ "MagneticAnomalyShipTrack", "centerLineOf" },
		{ "MagneticAnomalyShipTrack", "unclassifiedGeometry" },
		{ "Magnetics", "outlineOf" },
		{ "MantleDensity", "outlineOf" },
		{ "MidOceanRidge", "centerLineOf" },
		{ "MidOceanRidge", "outlineOf" },
		{ "MidOceanRidge", "unclassifiedGeometry" },
		{ "OceanicAge", "outlineOf" },
		{ "OldPlatesGridMark", "centerLineOf" },
		{ "OldPlatesGridMark", "unclassifiedGeometry" },
		{ "OrogenicBelt", "centerLineOf" },
		{ "OrogenicBelt", "outlineOf" },
		{ "OrogenicBelt", "unclassifiedGeometry" },
		{ "PassiveContinentalBoundary", "centerLineOf" },
		{ "PassiveContinentalBoundary", "outlineOf" },
		{ "PassiveContinentalBoundary", "unclassifiedGeometry" },
		{ "PseudoFault", "centerLineOf" },
		{ "PseudoFault", "unclassifiedGeometry" },
		{ "Roughness", "outlineOf" },
		{ "Seamount", "outlineOf" },
		{ "Seamount", "position" },
		{ "Seamount", "unclassifiedGeometry" },
		{ "SedimentThickness", "outlineOf" },
		{ "SpreadingAsymmetry", "outlineOf" },
		{ "SpreadingRate", "outlineOf" },
		{ "Stress", "outlineOf" },
		{ "SubductionZone", "centerLineOf" },
		{ "SubductionZone", "outlineOf" },
		{ "SubductionZone", "unclassifiedGeometry" },
		{ "Suture", "centerLineOf" },
		{ "Suture", "outlineOf" },
		{ "Suture", "unclassifiedGeometry" },
		{ "TerraneBoundary", "centerLineOf" },
		{ "TerraneBoundary", "unclassifiedGeometry" },
		{ "Topography", "outlineOf" },
		{ "TopologicalClosedPlateBoundary", "boundary" },
		{ "Transform", "centerLineOf" },
		{ "Transform", "outlineOf" },
		{ "Transform", "unclassifiedGeometry" },
		{ "TransitionalCrust", "outlineOf" },
		{ "TransitionalCrust", "unclassifiedGeometry" },
		{ "UnclassifiedFeature", "centerLineOf" },
		{ "UnclassifiedFeature", "outlineOf" },
		{ "UnclassifiedFeature", "unclassifiedGeometry" },
		{ "Unconformity", "centerLineOf" },
		{ "Unconformity", "unclassifiedGeometry" },
		{ "UnknownContact", "centerLineOf" },
		{ "UnknownContact", "unclassifiedGeometry" },
		{ "Volcano", "outlineOf" },
		{ "Volcano", "position" },
		{ "Volcano", "unclassifiedGeometry" },
	};
	
	/**
	 * Converts the above table into a multimap.
	 */
	const feature_geometric_prop_map_type &
	build_feature_geometric_prop_map()
	{
		static feature_geometric_prop_map_type map;
		// Add all the feature types -> geometric props from the feature_type_info_table.
		const FeatureTypeInfo *it = feature_type_info_table;
		const FeatureTypeInfo *end = it + NUM_ELEMS(feature_type_info_table);
		for ( ; it != end; ++it) {
			map.insert(std::make_pair(GPlatesModel::FeatureType::create_gpml(it->gpml_type),
					GPlatesModel::PropertyName::create_gpml(it->geometric_property) ));
		}
		return map;
	}
		
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
	class FeatureTypeItem :
			public QListWidgetItem
	{
	public:
		FeatureTypeItem(
				const GPlatesModel::FeatureType type_):
			QListWidgetItem(GPlatesUtils::make_qstring_from_icu_string(type_.build_aliased_name())),
			d_type(type_)
		{  }
		
		const GPlatesModel::FeatureType
		get_type()
		{
			return d_type;
		}
			
	private:
		const GPlatesModel::FeatureType d_type;
	};


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
	 * Subclass of QListWidgetItem so that we can display a list of FeatureCollection in
	 * the list widget using the filename as the label, while keeping track of which
	 * list item corresponds to which FeatureCollection.
	 */
	class FeatureCollectionItem :
			public QListWidgetItem
	{
	public:
		// Standard constructor for creating FeatureCollection entry.
		FeatureCollectionItem(
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				const QString &label):
			QListWidgetItem(label),
			d_collection(collection),
			d_is_create_new_collection_item(false)
		{  }

		// Constructor for creating fake "Make a new Feature Collection" entry.
		FeatureCollectionItem(
				const QString &label):
			QListWidgetItem(label),
			d_is_create_new_collection_item(true)
		{  }
		
		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_collection()
		{
			return d_collection;
		}
		
		bool
		is_create_new_collection_item()
		{
			return d_is_create_new_collection_item;
		}
	
	private:
		GPlatesModel::FeatureCollectionHandle::weak_ref d_collection;
		bool d_is_create_new_collection_item;
	};


	/**
	 * Fill the list with possible feature types we can create with this dialog.
	 */
	void
	populate_feature_types_list(
			QListWidget &list_widget)
	{
		std::list<GPlatesModel::FeatureType> list;
		// Build a list of all FeatureTypes mentioned in the table.
		const FeatureTypeInfo *table_it = feature_type_info_table;
		const FeatureTypeInfo *table_end = table_it + NUM_ELEMS(feature_type_info_table);
		for ( ; table_it != table_end; ++table_it) {
			list.push_back(GPlatesModel::FeatureType::create_gpml(table_it->gpml_type));
		}
		list.sort();
		list.unique();

		// FIXME: For extra brownie points, filter -this- list based on features
		// which you couldn't possibly create given the digitised geometry.
		// E.g. no Cratons made from PolylineOnSphere!
		
		list_widget.clear();
		// Add all the feature types from the finished list.
		std::list<GPlatesModel::FeatureType>::const_iterator list_it = list.begin();
		std::list<GPlatesModel::FeatureType>::const_iterator list_end = list.end();
		for ( ; list_it != list_end; ++list_it) {
			list_widget.addItem(new FeatureTypeItem(*list_it));
		}
		list_widget.setCurrentRow(0);
	}

	/**
	 * Fill the list with possible property names we can assign geometry to.
	 */
	void
	populate_geometry_destinations_list(
			QListWidget &list_widget,
			GPlatesModel::FeatureType target_feature_type)
	{
		static const geometry_prop_name_map_type geometry_prop_names =
				build_geometry_prop_name_map();
		static const geometry_prop_timedependency_map_type geometry_time_dependencies =
				build_geometry_prop_timedependency_map();
		// FIXME: This list should ideally be dynamic, depending on:
		//  - the type of GeometryOnSphere we are given (e.g. gpml:position for gml:Point)
		//  - the type of feature the user has selected in the first list (since different
		//    feature types are supposed to have a different selection of valid properties)
		list_widget.clear();
		// Iterate over the feature_type_info_table, and add all property names
		// that match our desired feature.
		static const feature_geometric_prop_map_type map = build_feature_geometric_prop_map();
		feature_geometric_prop_map_const_iterator it = map.lower_bound(target_feature_type);
		feature_geometric_prop_map_const_iterator end = map.upper_bound(target_feature_type);
		for ( ; it != end; ++it) {
			GPlatesModel::PropertyName property = it->second;
			// Display name defaults to the QualifiedXmlName.
			QString display_name = GPlatesUtils::make_qstring_from_icu_string(property.build_aliased_name());
			geometry_prop_name_map_const_iterator display_name_it = geometry_prop_names.find(property);
			if (display_name_it != geometry_prop_names.end()) {
				display_name = display_name_it->second;
			}
			// Now we have to look up the time-dependent flag somewhere, too.
			bool expects_time_dependent_wrapper = true;
			geometry_prop_timedependency_map_const_iterator time_dependency_it = geometry_time_dependencies.find(property);
			if (time_dependency_it != geometry_time_dependencies.end()) {
				expects_time_dependent_wrapper = time_dependency_it->second;
			}
			// Finally, add the item to the QListWidget.
			list_widget.addItem(new PropertyNameItem(property, display_name, expects_time_dependent_wrapper));
		}
		list_widget.setCurrentRow(0);
	}
	
	/**
	 * Fill the list with currently loaded FeatureCollections we can add the feature to.
	 */
	void
	populate_feature_collections_list(
			QListWidget &list_widget)
	{
		GPlatesAppState::ApplicationState *state = GPlatesAppState::ApplicationState::instance();
		
		GPlatesAppState::ApplicationState::file_info_iterator it = state->files_begin();
		GPlatesAppState::ApplicationState::file_info_iterator end = state->files_end();
		
		list_widget.clear();
		for (; it != end; ++it) {
			// Get the FeatureCollectionHandle for this file.
			boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> collection_opt =
					it->get_feature_collection();
					
			// Get a suitable label; we will prefer the full filename.
			QString label = it->get_display_name(true);
			
			// We are only interested in loaded files which have valid FeatureCollections.
			if (collection_opt) {
				list_widget.addItem(new FeatureCollectionItem(*collection_opt, label));
			}
		}
		// Add a final option for creating a brand new FeatureCollection.
		list_widget.addItem(new FeatureCollectionItem(QObject::tr(" < Create a new Feature Collection > ")));
		// Default to first entry.
		list_widget.setCurrentRow(0);
	}

	/**
	 * Get the FeatureType the user has selected.
	 */
	boost::optional<const GPlatesModel::FeatureType>
	currently_selected_feature_type(
			const QListWidget *listwidget_feature_types)
	{
		FeatureTypeItem *type_item = dynamic_cast<FeatureTypeItem *>(
				listwidget_feature_types->currentItem());
		if (type_item == NULL) {
			return boost::none;
		}
		return type_item->get_type();
	}
}



GPlatesQtWidgets::CreateFeatureDialog::CreateFeatureDialog(
		GPlatesModel::ModelInterface &model_interface,
		GPlatesQtWidgets::ViewportWindow &view_state_,
		QWidget *parent_):
	QDialog(parent_),
	d_model_ptr(&model_interface),
	d_view_state_ptr(&view_state_),
	d_geometry_opt_ptr(boost::none),
	d_plate_id_widget(new EditPlateIdWidget(this)),
	d_time_period_widget(new EditTimePeriodWidget(this)),
	d_name_widget(new EditStringWidget(this))
{
	setupUi(this);
	
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
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_type_page()
{
	// Populate list of feature types.
	populate_feature_types_list(*listwidget_feature_types);
	
	// Pushing Enter or double-clicking should cause the page to advance.
	QObject::connect(listwidget_feature_types, SIGNAL(itemActivated(QListWidgetItem *)),
			this, SLOT(handle_next()));
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
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);
	
	// Create the edit widgets we'll need, and add them to the Designer-created widgets.
	QVBoxLayout *edit_layout;
	edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setMargin(4);
	edit_layout->addWidget(d_plate_id_widget);
	edit_layout->addWidget(d_time_period_widget);
	edit_layout->addWidget(d_name_widget);
	groupbox_properties->setLayout(edit_layout);
	
	// Note that the geometric properties list must be populated dynamically
	// on page change, see handle_page_change() below.
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_feature_collection_page()
{
	// Populate list of feature collections.
	// Note that this should also be done any time the user opens the dialog with
	// fresh geometry they wish to create a Feature with.
	populate_feature_collections_list(*listwidget_feature_collections);
	
	// Pushing Enter or double-clicking should cause the buttonbox to focus.
	QObject::connect(listwidget_feature_collections, SIGNAL(itemActivated(QListWidgetItem *)),
			buttonbox, SLOT(setFocus()));
}


void
GPlatesQtWidgets::CreateFeatureDialog::set_up_geometric_property_list()
{
	// Get the FeatureType the user has selected.
	boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
			currently_selected_feature_type(listwidget_feature_types);
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
	populate_feature_collections_list(*listwidget_feature_collections);
	
	// Show the dialog modally.
	return exec();
}


bool
GPlatesQtWidgets::CreateFeatureDialog::display()
{
	// Set the stack back to the first page.
	stack->setCurrentIndex(0);
	// The Feature Collections list needs to be repopulated each time.
	populate_feature_collections_list(*listwidget_feature_collections);
	
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
	d_button_create->setEnabled(true);
	
	// Disable buttons which are not valid for the page,
	// and focus the first widget.
	switch (page)
	{
	case 0:
			listwidget_feature_types->setFocus();
			button_prev->setEnabled(false);
			d_button_create->setEnabled(false);
			break;

	case 1:
			// Populate the listwidget_geometry_destinations based on what is legal right now.
			set_up_geometric_property_list();
			listwidget_geometry_destinations->setFocus();
			d_button_create->setEnabled(false);
			break;

	case 2:
			listwidget_feature_collections->setFocus();
			button_next->setEnabled(false);
			break;
	}
}


// FIXME: This method is getting too long.
void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
{
	if (d_type == TOPOLOGICAL)
	{
		handle_create_topological();
		return;
	}
	// Get the PropertyName the user has selected for geometry to go into.
	// Also find out if we should be wrapping geometry in a GpmlConstantValue.
	PropertyNameItem *geom_prop_name_item = dynamic_cast<PropertyNameItem *>(
			listwidget_geometry_destinations->currentItem());
	if (geom_prop_name_item == NULL) {
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geom_prop_name = geom_prop_name_item->get_name();
	bool geom_prop_needs_constant_value = geom_prop_name_item->expects_time_dependent_wrapper();


	// Check we have a valid GeometryOnSphere supplied from the DigitisationWidget.
	if ( ! d_geometry_opt_ptr) {
		// Should never happen, as we can't open the dialog (legitimately) without geometry!
		QMessageBox::critical(this, tr("No geometry"),
				tr("No geometry was supplied to this dialog. Please try digitising again."));
		// FIXME: Exception.
		reject();
		return;
	}
	
	// Invoke GeometricPropertyValueConstructor.
	// This will Un-Reconstruct the temporary geometry so that it's coordinates are
	// expressed in terms of present day location, given the plate ID that is associated
	// with it and the current reconstruction time.
	GPlatesModel::ReconstructionTree &recon_tree = 
			d_view_state_ptr->reconstruction().reconstruction_tree();
	GPlatesModel::integer_plate_id_type int_plate_id = 
			d_plate_id_widget->create_integer_plate_id_from_widget();	
	// It will also wrap the present-day GeometryOnSphere in a suitable PropertyValue,
	// possibly including a GpmlConstantValue wrapper.
	GPlatesGui::GeometricPropertyValueConstructor geometry_constructor;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
			geometry_constructor.convert(*d_geometry_opt_ptr, recon_tree, int_plate_id,
					geom_prop_needs_constant_value);
	
	if ( ! geometry_value_opt) {
		// Might happen, if DigitisationWidget and CreateFeatureDialog (specifically, the
		// GeometricPropertyValueConstructor) disagree on what is implemented and what is not.
		// FIXME: Exception?
		QMessageBox::critical(this, tr("Cannot convert geometry to property value"),
				tr("There was an error converting the digitised geometry to a usable property value."));
		reject();
		return;
	}
	
	
	// Get the FeatureType the user has selected.
	boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
			currently_selected_feature_type(listwidget_feature_types);
	if ( ! feature_type_opt) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType type = *feature_type_opt;
	
	
	// Get the FeatureCollection the user has selected.
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
			listwidget_feature_collections->currentItem());
	if (collection_item == NULL) {
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
	GPlatesModel::FeatureCollectionHandle::weak_ref collection;
	if (collection_item->is_create_new_collection_item()) {
		GPlatesAppState::ApplicationState::file_info_iterator new_file = 
				d_view_state_ptr->create_empty_reconstructable_file();
		collection = *(new_file->get_feature_collection());
	} else {
		collection = collection_item->get_collection();
	}
	// Actually create the Feature!
	GPlatesModel::FeatureHandle::weak_ref feature = d_model_ptr->create_feature(type, collection);
	

	// Add a (possibly ConstantValue-wrapped, see GeometricPropertyValueConstructor)
	// Geometry Property using present-day geometry.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			*geometry_value_opt,
			geom_prop_name,
			feature);

	// Add a (ConstantValue-wrapped) gpml:reconstructionPlateId Property.
	GPlatesModel::PropertyValue::non_null_ptr_type plate_id_value =
			d_plate_id_widget->create_property_value_from_widget();
	GPlatesPropertyValues::TemplateTypeParameterType plate_id_value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId");
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			GPlatesPropertyValues::GpmlConstantValue::create(plate_id_value, plate_id_value_type),
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			feature);

	// Add a gml:validTime Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			d_time_period_widget->create_property_value_from_widget(),
			GPlatesModel::PropertyName::create_gml("validTime"),
			feature);

	// Add a gml:name Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			d_name_widget->create_property_value_from_widget(),
			GPlatesModel::PropertyName::create_gml("name"),
			feature);
	
	emit feature_created(feature);
	accept();
}

void
GPlatesQtWidgets::CreateFeatureDialog::handle_create_topological()
{
	// Get the PropertyName the user has selected for geometry to go into.
	// Also find out if we should be wrapping geometry in a GpmlConstantValue.
	PropertyNameItem *geom_prop_name_item = dynamic_cast<PropertyNameItem *>(
			listwidget_geometry_destinations->currentItem());
	if (geom_prop_name_item == NULL) {
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geom_prop_name = geom_prop_name_item->get_name();
	// bool geom_prop_needs_constant_value = geom_prop_name_item->expects_time_dependent_wrapper();


	// Get the FeatureType the user has selected.
	boost::optional<const GPlatesModel::FeatureType> feature_type_opt =
			currently_selected_feature_type(listwidget_feature_types);
	if ( ! feature_type_opt) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType type = *feature_type_opt;
	
	
	// Get the FeatureCollection the user has selected.
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
			listwidget_feature_collections->currentItem());
	if (collection_item == NULL) {
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
	GPlatesModel::FeatureCollectionHandle::weak_ref collection = 
		collection_item->get_collection();
	
	// Actually create the Feature!
	GPlatesModel::FeatureHandle::weak_ref feature = 
		d_model_ptr->create_feature(type, collection);
	

	// Add a (ConstantValue-wrapped) gpml:reconstructionPlateId Property.
	GPlatesModel::PropertyValue::non_null_ptr_type plate_id_value =
			d_plate_id_widget->create_property_value_from_widget();

	GPlatesPropertyValues::TemplateTypeParameterType plate_id_value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId");

	GPlatesModel::ModelUtils::append_property_value_to_feature(
			GPlatesPropertyValues::GpmlConstantValue::create(
				plate_id_value, 
				plate_id_value_type),
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			feature);

	// Add a gml:validTime Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			d_time_period_widget->create_property_value_from_widget(),
			GPlatesModel::PropertyName::create_gml("validTime"),
			feature);

	// Add a gml:name Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			d_name_widget->create_property_value_from_widget(),
			GPlatesModel::PropertyName::create_gml("name"),
			feature);

	emit feature_created(feature);
	accept();
}

