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
#include "ViewportWindow.h"
#include "qt-widgets/ApplicationState.h"
#include "model/types.h"
#include "model/PropertyName.h"
#include "model/FeatureType.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "maths/FiniteRotation.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "property-values/GmlMultiPoint.h"
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
		const char *gpml_type;
	};
	
	/**
	 * This list was created with the command:
	 *
	 * perl -ne 'print "\t\t{ \"$1\" },\n" if /^class\s+(\w+)/;' GPGIM_1.6.juuml | sort
	 *
	 * plus some manual pruning. It could probably be more intelligent, but then
	 * it would have to be a separate script.
	 */
	static const FeatureTypeInfo feature_type_info_table[] = {
		{ "AseismicRidge" },
		{ "BasicRockUnit" },
		{ "Basin" },
		{ "Bathymetry" },
		{ "ClosedContinentalBoundary" },
		{ "Coastline" },
		{ "ContinentalFragment" },
		{ "ContinentalRift" },
		{ "Craton" },
		{ "CrustalThickness" },
		{ "DynamicTopography" },
		{ "ExtendedContinentalCrust" },
		{ "Fault" },
		{ "FoldPlane" },
		{ "FractureZone" },
		{ "FractureZoneIdentification" },
		{ "GeologicalLineation" },
		{ "GeologicalPlane" },
		{ "GlobalElevation" },
		{ "Gravimetry" },
		{ "HeatFlow" },
		{ "HotSpot" },
		{ "HotSpotTrail" },
		{ "InferredPaleoBoundary" },
		{ "IslandArc" },
		{ "Isochron" },
		{ "LargeIgneousProvince" },
		{ "MagneticAnomalyIdentification" },
		{ "MagneticAnomalyShipTrack" },
		{ "Magnetics" },
		{ "MantleDensity" },
		{ "MidOceanRidge" },
		{ "OceanicAge" },
		{ "OldPlatesGridMark" },
		{ "OrogenicBelt" },
		{ "PassiveContinentalBoundary" },
		{ "PseudoFault" },
		{ "Roughness" },
		{ "Seamount" },
		{ "SedimentThickness" },
		{ "SpreadingAsymmetry" },
		{ "SpreadingRate" },
		{ "Stress" },
		{ "SubductionZone" },
		{ "Suture" },
		{ "TerraneBoundary" },
		{ "Topography" },
		{ "Transform" },
		{ "TransitionalCrust" },
		{ "UnclassifiedFeature" },
		{ "Unconformity" },
		{ "UnknownContact" },
		{ "Volcano" },
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
				const GPlatesModel::PropertyName name,
				bool expects_time_dependent_wrapper_):
			QListWidgetItem(GPlatesUtils::make_qstring_from_icu_string(name.build_aliased_name())),
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
			list.addItem(new FeatureTypeItem(
					GPlatesModel::FeatureType::create_gpml(begin->gpml_type) ));
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
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("centerLineOf"), true));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("outlineOf"), true));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("errorBounds"), true));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("boundary"), false));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("position"), false));
		list.addItem(new PropertyNameItem(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), true));
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
	 * Apply a reverse reconstruction to the given temporary geometry, so that the
	 * coordinates are set to present-day location given the supplied plate id and
	 * current reconstruction tree.
	 *
	 * @a G should be a non_null_ptr_to_const_type of some suitable GeometryOnSphere
	 * derivation, with an implementation of FiniteRotation::operator*() available.
	 */
	template <class G>
	G
	reverse_reconstruct(
			G geometry,
			const GPlatesModel::integer_plate_id_type plate_id,
			GPlatesModel::ReconstructionTree &recon_tree)
	{
		// Get the composed absolute rotation needed to bring a thing on that plate
		// in the present day to this time.
		const GPlatesMaths::FiniteRotation rotation =
				recon_tree.get_composed_absolute_rotation(plate_id).first;
		const GPlatesMaths::FiniteRotation reverse = GPlatesMaths::get_reverse(rotation);
		
		// Apply the reverse rotation.
		G present_day_geometry = reverse * geometry;
		
		return present_day_geometry;
	}
	
	
	/**
	 * ConstGeometryVisitor to wrap a GeometryOnSphere with the appropriate
	 * PropertyValue (e.g. GmlLineString for PolylineOnSphere)
	 *
	 * FIXME: This should ideally be in the geometry-visitors/ directory.
	 * FIXME 2: We should pass a flag indicating whether the resulting
	 * geometry PropertyValue should be GpmlConstantValue-wrapped.
	 */
	class GeometricPropertyValueConstructor : 
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometricPropertyValueConstructor(
				GPlatesModel::ReconstructionTree &reconstruction_tree_):
			GPlatesMaths::ConstGeometryOnSphereVisitor(),
			d_property_value_opt(boost::none),
			d_plate_id_opt(boost::none),
			d_wrap_with_gpml_constant_value(true),
			d_recon_tree_ptr(&reconstruction_tree_)
		{  }
		
		virtual
		~GeometricPropertyValueConstructor()
		{  }
		
		/**
		 * Call this to visit a GeometryOnSphere and (attempt to)
		 *  a) reverse-reconstruct the geometry to appropriate present-day coordinates,
		 *  b) create a suitable geometric PropertyValue out of it.
		 *  c) wrap that property value in a GpmlConstantValue.
		 *
		 * May return boost::none.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		convert(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_opt,
				bool wrap_with_gpml_constant_value)
		{
			// Set parameters for this visitation.
			d_plate_id_opt = plate_id_opt;
			d_wrap_with_gpml_constant_value = wrap_with_gpml_constant_value;

         // Clear any previous return value and do the visit.
			d_property_value_opt = boost::none;
			geometry_ptr->accept_visitor(*this);
			return d_property_value_opt;
		}
		
		// Please keep these geometries ordered alphabetically.
		
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
			
			if (d_plate_id_opt) {
				// Reverse reconstruct the geometry to present-day.
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type present_day =
						reverse_reconstruct(multi_point_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
				// Convert MultiPointOnSphere to GmlMultiPoint.
				prop_val_opt = GPlatesPropertyValues::GmlMultiPoint::create(present_day);
			} else {
				// No reverse-reconstruction, just make a PropertyValue.
				prop_val_opt = GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);
			}
			// At this point, we should have a valid PropertyValue.
			// Now - do we want to add a GpmlConstantValue wrapper around it?
			if (d_wrap_with_gpml_constant_value) {
				// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
				// wrapper around geometry, although GPlates is happy enough to display geometry
				// without it.
				static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
						GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint");
				prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
						*prop_val_opt, value_type);
			}
			
			// Return the prepared PropertyValue.
			d_property_value_opt = prop_val_opt;
		}
		
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
			
			if (d_plate_id_opt) {
				// Reverse reconstruct the geometry to present-day.
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type present_day =
						reverse_reconstruct(point_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
				// Convert PointOnSphere to GmlPoint.
				prop_val_opt = GPlatesPropertyValues::GmlPoint::create(*present_day);
			} else {
				// No reverse-reconstruction, just make a PropertyValue.
				prop_val_opt = GPlatesPropertyValues::GmlPoint::create(*point_on_sphere);
			}
			// At this point, we should have a valid PropertyValue.
			// Now - do we want to add a GpmlConstantValue wrapper around it?
			if (d_wrap_with_gpml_constant_value) {
				// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
				// wrapper around geometry, although GPlates is happy enough to display geometry
				// without it.
				static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
						GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point");
				prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
						*prop_val_opt, value_type);
			}
			
			// Return the prepared PropertyValue.
			d_property_value_opt = prop_val_opt;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			// Convert PolygonOnSphere to GmlPolygon with one exterior ring.
			// FIXME: We could make this more intelligent and open up the possibility of making
			// polygons with interiors.
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
			
			if (d_plate_id_opt) {
				// Reverse reconstruct the geometry to present-day.
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type present_day =
						reverse_reconstruct(polygon_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
				// Convert PolygonOnSphere to GmlPolygon.
				prop_val_opt = GPlatesPropertyValues::GmlPolygon::create(present_day);
			} else {
				// No reverse-reconstruction, just make a PropertyValue.
				prop_val_opt = GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);
			}
			// At this point, we should have a valid PropertyValue.
			// Now - do we want to add a GpmlConstantValue wrapper around it?
			if (d_wrap_with_gpml_constant_value) {
				// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
				// wrapper around geometry, although GPlates is happy enough to display geometry
				// without it.
				static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
						GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon");
				prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
						*prop_val_opt, value_type);
			}
			
			// Return the prepared PropertyValue.
			d_property_value_opt = prop_val_opt;
		}
		
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			// Convert PolylineOnSphere to GmlLineString.
			// FIXME: OrientableCurve??
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
			
			if (d_plate_id_opt) {
				// Reverse reconstruct the geometry to present-day.
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type present_day =
						reverse_reconstruct(polyline_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
				// Convert PolylineOnSphere to GmlLineString.
				prop_val_opt = GPlatesPropertyValues::GmlLineString::create(present_day);
			} else {
				// No reverse-reconstruction, just make a PropertyValue.
				prop_val_opt = GPlatesPropertyValues::GmlLineString::create(polyline_on_sphere);
			}
			// At this point, we should have a valid PropertyValue.
			// Now - do we want to add a GpmlConstantValue wrapper around it?
			if (d_wrap_with_gpml_constant_value) {
				// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
				// wrapper around geometry, although GPlates is happy enough to display geometry
				// without it.
				static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
						GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
				prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
						*prop_val_opt, value_type);
			}
			
			// Return the prepared PropertyValue.
			d_property_value_opt = prop_val_opt;
		}
	
	private:
		// This operator should never be defined, because we don't want to allow copy-assignment.
		GeometricPropertyValueConstructor &
		operator=(
				const GeometricPropertyValueConstructor &);
		
		/**
		 * The return value of convert(), assigned during the visit.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value_opt;
		
		/**
		 * The plate id parameter for reverse reconstructing the geometry.
		 * Set from convert().
		 * If it is set to boost::none we will assume that, for whatever reason, the caller
		 * does not want us to do reverse reconstructions today.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id_opt;
		
		/**
		 * The parameter indicating the caller wants the final PropertyValue to be
		 * wrapped up in a suitable GpmlConstantValue.
		 * Set from convert().
		 */
		bool d_wrap_with_gpml_constant_value;
		
		/**
		 * This is the reconstruction tree, used to perform the reverse reconstruction
		 * and obtain present-day geometry appropriate for the given plate id.
		 */
		GPlatesModel::ReconstructionTree *d_recon_tree_ptr;
	};
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


// FIXME: This method is getting too long.
void
GPlatesQtWidgets::CreateFeatureDialog::handle_create()
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
	GeometricPropertyValueConstructor geometry_constructor(recon_tree);
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
			geometry_constructor.convert(*d_geometry_opt_ptr, int_plate_id,
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
	FeatureTypeItem *type_item = dynamic_cast<FeatureTypeItem *>(
			listwidget_feature_types->currentItem());
	if (type_item == NULL) {
		QMessageBox::critical(this, tr("No feature type selected"),
				tr("Please select a feature type to create."));
		return;
	}
	const GPlatesModel::FeatureType type = type_item->get_type();
	
	
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

