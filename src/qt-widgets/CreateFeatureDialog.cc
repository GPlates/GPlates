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
#include "qt-widgets/ApplicationState.h"
#include "model/PropertyName.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlLineString.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	/**
	 * This struct is used to build a static table of all possible FeatureTypes we can
	 * create with this dialog.
	 * For now, it only has one member, the type - but it will ideally have extra metadata
	 * about which properties the user should be presented with.
	 */
	struct FeatureTypeInfo
	{
		const GPlatesModel::FeatureType type;
	};
	
	/**
	 * This list was created with the command:
	 *
	 * perl -ne 'print "\t\t{ GPlatesModel::FeatureType::create_gpml(\"$1\") },\n" if /^class\s+(\w+)/;' GPGIM_1.6.juuml | sort
	 *
	 * plus some manual pruning. It could probably be more intelligent, but then
	 * it would have to be a separate script.
	 */
	static const FeatureTypeInfo feature_type_info_table[] = {
		{ GPlatesModel::FeatureType::create_gpml("AseismicRidge") },
		{ GPlatesModel::FeatureType::create_gpml("BasicRockUnit") },
		{ GPlatesModel::FeatureType::create_gpml("Basin") },
		{ GPlatesModel::FeatureType::create_gpml("Bathymetry") },
		{ GPlatesModel::FeatureType::create_gpml("ClosedContinentalBoundary") },
		{ GPlatesModel::FeatureType::create_gpml("Coastline") },
		{ GPlatesModel::FeatureType::create_gpml("ContinentalFragment") },
		{ GPlatesModel::FeatureType::create_gpml("ContinentalRift") },
		{ GPlatesModel::FeatureType::create_gpml("Craton") },
		{ GPlatesModel::FeatureType::create_gpml("CrustalThickness") },
		{ GPlatesModel::FeatureType::create_gpml("DynamicTopography") },
		{ GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust") },
		{ GPlatesModel::FeatureType::create_gpml("Fault") },
		{ GPlatesModel::FeatureType::create_gpml("FoldPlane") },
		{ GPlatesModel::FeatureType::create_gpml("FractureZone") },
		{ GPlatesModel::FeatureType::create_gpml("FractureZoneIdentification") },
		{ GPlatesModel::FeatureType::create_gpml("GeologicalLineation") },
		{ GPlatesModel::FeatureType::create_gpml("GeologicalPlane") },
		{ GPlatesModel::FeatureType::create_gpml("GlobalElevation") },
		{ GPlatesModel::FeatureType::create_gpml("Gravimetry") },
		{ GPlatesModel::FeatureType::create_gpml("HeatFlow") },
		{ GPlatesModel::FeatureType::create_gpml("HotSpot") },
		{ GPlatesModel::FeatureType::create_gpml("HotSpotTrail") },
		{ GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary") },
		{ GPlatesModel::FeatureType::create_gpml("IslandArc") },
		{ GPlatesModel::FeatureType::create_gpml("Isochron") },
		{ GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince") },
		{ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification") },
		{ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyShipTrack") },
		{ GPlatesModel::FeatureType::create_gpml("Magnetics") },
		{ GPlatesModel::FeatureType::create_gpml("MantleDensity") },
		{ GPlatesModel::FeatureType::create_gpml("MidOceanRidge") },
		{ GPlatesModel::FeatureType::create_gpml("OceanicAge") },
		{ GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark") },
		{ GPlatesModel::FeatureType::create_gpml("OrogenicBelt") },
		{ GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary") },
		{ GPlatesModel::FeatureType::create_gpml("PseudoFault") },
		{ GPlatesModel::FeatureType::create_gpml("Roughness") },
		{ GPlatesModel::FeatureType::create_gpml("Seamount") },
		{ GPlatesModel::FeatureType::create_gpml("SedimentThickness") },
		{ GPlatesModel::FeatureType::create_gpml("SpreadingAsymmetry") },
		{ GPlatesModel::FeatureType::create_gpml("SpreadingRate") },
		{ GPlatesModel::FeatureType::create_gpml("Stress") },
		{ GPlatesModel::FeatureType::create_gpml("SubductionZone") },
		{ GPlatesModel::FeatureType::create_gpml("Suture") },
		{ GPlatesModel::FeatureType::create_gpml("TerraneBoundary") },
		{ GPlatesModel::FeatureType::create_gpml("Topography") },
		{ GPlatesModel::FeatureType::create_gpml("Transform") },
		{ GPlatesModel::FeatureType::create_gpml("TransitionalCrust") },
		{ GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature") },
		{ GPlatesModel::FeatureType::create_gpml("Unconformity") },
		{ GPlatesModel::FeatureType::create_gpml("UnknownContact") },
		{ GPlatesModel::FeatureType::create_gpml("Volcano") },
	};
	
	
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
				const GPlatesModel::PropertyName name):
			QListWidgetItem(GPlatesUtils::make_qstring_from_icu_string(name.build_aliased_name())),
			d_name(name)
		{  }
	
		const GPlatesModel::PropertyName
		get_name()
		{
			return d_name;
		}

	private:
		const GPlatesModel::PropertyName d_name;
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
		FeatureCollectionItem(
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				const QString &label):
			QListWidgetItem(label),
			d_collection(collection)
		{  }
		
		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_collection()
		{
			return d_collection;
		}
	
	private:
		GPlatesModel::FeatureCollectionHandle::weak_ref d_collection;
	};


	/**
	 * Fill the list with possible feature types we can create with this dialog.
	 */
	void
	populate_feature_types_list(
			QListWidget &list)
	{
		list.clear();
		// Add all the feature types from the feature_type_info_table.
		const FeatureTypeInfo *begin = feature_type_info_table;
		const FeatureTypeInfo *end = begin + NUM_ELEMS(feature_type_info_table);
		for ( ; begin != end; ++begin) {
			list.addItem(new FeatureTypeItem(begin->type));
		}
		list.setCurrentRow(0);
	}

	/**
	 * Fill the list with possible property names we can assign geometry to.
	 */
	void
	populate_geometry_destinations_list(
			QListWidget &list)
	{
		// FIXME: This list should ideally be dynamic, depending on:
		//  - the type of GeometryOnSphere we are given (e.g. gpml:position for gml:Point)
		//  - the type of feature the user has selected in the first list (since different
		//    feature types are supposed to have a different selection of valid properties)
		list.clear();
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("centerLineOf")));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("outlineOf")));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("errorBounds")));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("boundary")));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("position")));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry")));
		list.setCurrentRow(0);
	}
	
	/**
	 * Fill the list with currently loaded FeatureCollections we can add the feature to.
	 */
	void
	populate_feature_collections_list(
			QListWidget &list)
	{
		GPlatesAppState::ApplicationState *state = GPlatesAppState::ApplicationState::instance();
		
		GPlatesAppState::ApplicationState::file_info_iterator it = state->files_begin();
		GPlatesAppState::ApplicationState::file_info_iterator end = state->files_end();
		
		list.clear();
		for (; it != end; ++it) {
			// Get the FeatureCollectionHandle for this file.
			boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> collection_opt =
					it->get_feature_collection();
					
			// Get a suitable label; we will use the full filename.
			QString label = it->get_qfileinfo().absoluteFilePath();
			
			// We are only interested in loaded files which have valid FeatureCollections.
			if (collection_opt) {
				list.addItem(new FeatureCollectionItem(*collection_opt, label));
			}
		}
		list.setCurrentRow(0);
	}
	
	
	/**
	 * ConstGeometryVisitor to wrap a GeometryOnSphere with the appropriate
	 * PropertyValue (e.g. GmlLineString for PolylineOnSphere)
	 *
	 * FIXME: This should ideally be in the geometry-visitors/ directory.
	 */
	class GeometricPropertyValueConstructor : 
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometricPropertyValueConstructor():
			GPlatesMaths::ConstGeometryOnSphereVisitor()
		{  }
		
		virtual
		~GeometricPropertyValueConstructor()
		{  }
		
		/**
		 * Call this to visit a GeometryOnSphere and (attempt to) create a PropertyValue.
		 *
		 * May return boost::none.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		convert(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
		{
			d_property_value_opt = boost::none;
			geometry_ptr->accept_visitor(*this);
			return d_property_value_opt;
		}
		
		// Please keep these geometries ordered alphabetically.
		
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			// Convert PointOnSphere to GmlPoint.
			d_property_value_opt = GPlatesPropertyValues::GmlPoint::create(*point_on_sphere);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			// Convert PolygonOnSphere to GmlPolygon with one exterior ring.
			// FIXME: We could make this more intelligent and open up the possibility of making
			// polygons with interiors.
			d_property_value_opt = GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);
		}
		
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			// Convert PolylineOnSphere to GmlLineString.
			// FIXME: OrientableCurve??
			d_property_value_opt = GPlatesPropertyValues::GmlLineString::create(polyline_on_sphere);
		}
	
	private:
		// This operator should never be defined, because we don't want to allow copy-assignment.
		GeometricPropertyValueConstructor &
		operator=(
				const GeometricPropertyValueConstructor &);
		
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value_opt;
	};
}


GPlatesQtWidgets::CreateFeatureDialog::CreateFeatureDialog(
		GPlatesModel::ModelInterface &model_interface,
		QWidget *parent_):
	QDialog(parent_),
	d_model_ptr(&model_interface),
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
	// the QDialogButtonBox to guarentee that "Previous" comes before "Next".
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
	d_name_widget->label()->setText(tr("Na&me:"));
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

	// Populate the listwidget_geometry_destinations.
	populate_geometry_destinations_list(*listwidget_geometry_destinations);
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
			listwidget_geometry_destinations->setFocus();
			d_button_create->setEnabled(false);
			break;

	case 2:
			listwidget_feature_collections->setFocus();
			button_next->setEnabled(false);
			break;
	}
}


void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
{
	// Convert the GeometryOnSphere supplied from the DigitisationWidget to
	// a PropertyValue we can add to the feature.
	if ( ! d_geometry_opt_ptr) {
		// Should never happen, as we can't open the dialog (legitimately) without geometry!
		QMessageBox::critical(this, tr("No geometry"),
				tr("No geometry was supplied to this dialog. Please try digitising again."));
		// FIXME: Exception.
		reject();
		return;
	}
	GeometricPropertyValueConstructor geometry_constructor;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
			geometry_constructor.convert(*d_geometry_opt_ptr);
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
	FeatureTypeItem *type_item = dynamic_cast<FeatureTypeItem *>(
			listwidget_feature_types->currentItem());
	if (type_item == NULL) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType type = type_item->get_type();


	// Get the PropertyName the user has selected for geometry to go into.
	PropertyNameItem *geom_prop_name_item = dynamic_cast<PropertyNameItem *>(
			listwidget_geometry_destinations->currentItem());
	if (geom_prop_name_item == NULL) {
		QMessageBox::critical(this, tr("No geometry destination selected"),
				tr("Please select a property name to use for your digitised geometry."));
		return;
	}
	const GPlatesModel::PropertyName geom_prop_name = geom_prop_name_item->get_name();
	
	
	// Get the FeatureCollection the user has selected.
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
			listwidget_feature_collections->currentItem());
	if (collection_item == NULL) {
		QMessageBox::critical(this, tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
	GPlatesModel::FeatureCollectionHandle::weak_ref collection = collection_item->get_collection();
	
	// Actually create the Feature!
	GPlatesModel::FeatureHandle::weak_ref feature = d_model_ptr->create_feature(type, collection);
	
	// Add a Geometry Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			*geometry_value_opt,
			geom_prop_name,
			feature);

	// Add a gpml:reconstructionPlateId Property.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			d_plate_id_widget->create_property_value_from_widget(),
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

