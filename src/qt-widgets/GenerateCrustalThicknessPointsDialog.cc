/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include "GenerateCrustalThicknessPointsDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ScalarCoverageFeatureProperties.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/FeatureFocus.h"

#include "maths/GeneratePoints.h"
#include "maths/MultiPointOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ViewState.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"

#include "qt-widgets/ChooseFeatureCollectionWidget.h"
#include "qt-widgets/EditPlateIdWidget.h"
#include "qt-widgets/EditStringWidget.h"
#include "qt-widgets/EditTimePeriodWidget.h"
#include "qt-widgets/QtWidgetUtils.h"


namespace
{
	const QString HELP_SCALAR_TYPE_DIALOG_TITLE =
			QObject::tr("Crustal scalar types");
	const QString HELP_SCALAR_TYPE_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Two types of crustal scalar values are possible:</p>"
			"<ul>"
			"<li><i>crustal thinning factor</i>: Represents the amount of stretching/thinning. "
			"Typically starts with an initial value of 1.0 and has no units.</li>"
			"<li><i>crustal thickness</i>: Represents the thickness in kms.</li>"
			"</ul>"
			"</body></html>\n");

	const QString HELP_POINT_DISTRIBUTION_DIALOG_TITLE =
			QObject::tr("Distribution of points");
	const QString HELP_POINT_DISTRIBUTION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>The initial crustal point positions are uniformly distributed within the polygon boundary "
			"of the currently focused feature. Also a random offset can be applied to each position.</p>"
			"<p><i>Density level</i>: Points at level zero are spaced roughly 20 degrees apart. "
			"Each increment of the density level halves the spacing between points.</p>"
			"<p><i>Random offset</i>: The amount of random offset can vary between 0 and 100%. "
			"At 100% each point is randomly offset within a circle of radius half the spacing between points.</p>"
			"</body></html>\n");
}


const GPlatesPropertyValues::ValueObjectType
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::GPML_CRUSTAL_THINNING_FACTOR =
		GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThinningFactor");

const GPlatesPropertyValues::ValueObjectType
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::GPML_CRUSTAL_THICKNESS =
		GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");


GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::GenerateCrustalThicknessPointsDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	GPlatesDialog(
			parent_,
			Qt::CustomizeWindowHint |
			Qt::WindowTitleHint |
			Qt::WindowSystemMenuHint |
			Qt::MSWindowsFixedSizeDialogHint),
	d_application_state(view_state.get_application_state()),
	d_feature_focus(view_state.get_feature_focus()),
	d_plate_id_widget(new EditPlateIdWidget(this)),
	d_time_period_widget(new EditTimePeriodWidget(this)),
	d_name_widget(new EditStringWidget(this)),
	d_choose_feature_collection_widget(
			new ChooseFeatureCollectionWidget(
					d_application_state.get_reconstruct_method_registry(),
					d_application_state.get_feature_collection_file_state(),
					d_application_state.get_feature_collection_file_io(),
					this)),
	d_crustal_scalar_type(GPML_CRUSTAL_THINNING_FACTOR),
	d_help_scalar_type_dialog(
			new InformationDialog(
					HELP_SCALAR_TYPE_DIALOG_TEXT,
					HELP_SCALAR_TYPE_DIALOG_TITLE,
					this)),
	d_help_point_distribution_dialog(
			new InformationDialog(
					HELP_POINT_DISTRIBUTION_DIALOG_TEXT,
					HELP_POINT_DISTRIBUTION_DIALOG_TITLE,
					this))
{
	setupUi(this);

	QtWidgetUtils::add_widget_to_placeholder(
			d_choose_feature_collection_widget,
			widget_choose_feature_collection_placeholder);

	// Set these to false to prevent buttons from stealing Enter events from the spinboxes in the enclosed widget.
	button_create->setAutoDefault(false);
	button_cancel->setAutoDefault(false);
	button_create->setDefault(false);
	button_cancel->setDefault(false);

	setup_pages();
}


bool
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::initialise()
{
	d_focused_boundary_polygon = boost::none;

	make_generate_points_page_current();

	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry;
	if (!d_feature_focus.is_valid() ||
		!(focused_geometry = d_feature_focus.associated_reconstruction_geometry()))
	{
		QMessageBox::critical(this,
				tr("No focused feature selected"),
				tr("Please use the Choose Feature tool to select a topological plate, "
					"a topological network or a static polygon."));
		return false;
	}

	// Get the boundary polygon of the focused feature (topological plate/network or static polygon). 
	d_focused_boundary_polygon = GPlatesAppLogic::ReconstructionGeometryUtils::get_boundary_polygon(focused_geometry);
	if (!d_focused_boundary_polygon)
	{
		QMessageBox::critical(this,
				tr("Focused feature does not have a polygon boundary"),
				tr("Please use the Choose Feature tool to select a topological plate, "
					"a topological network or a static polygon."));
		return false;
	}

	// Default to plate ID zero.
	GPlatesModel::integer_plate_id_type reconstruction_plate_id = 0;

	// If the focused feature is *not* a topological network then initialise using its plate ID,
	// otherwise set to zero (because plate IDs for topological networks currently don't have a
	// well-defined meaning since they are not used for anything, eg, velocity calculations).
	const bool is_resolved_topological_network =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ResolvedTopologicalNetwork *>(focused_geometry);
	if (!is_resolved_topological_network)
	{
		// Get the reconstruction plate ID of the focused feature.
		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> gpml_reconstruction_plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						d_feature_focus.focused_feature(),
						GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"));
		if (gpml_reconstruction_plate_id)
		{
			reconstruction_plate_id = gpml_reconstruction_plate_id.get()->value();
		}
	}

	// Set the plate ID in the edit widget.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_reconstruction_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(reconstruction_plate_id);
	d_plate_id_widget->update_widget_from_plate_id(*gpml_reconstruction_plate_id);

	// Set the time period widget to all time (we don't use the time period of the focused feature).
	const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time =
			GPlatesModel::ModelUtils::create_gml_time_period(
					GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
					GPlatesPropertyValues::GeoTimeInstant(0.0));
	d_time_period_widget->update_widget_from_time_period(*gml_valid_time);

	// Set the name of the focused feature in the edit widget.
	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> gml_name =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
					d_feature_focus.focused_feature(),
					GPlatesModel::PropertyName::create_gml("name"));
	if (gml_name)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_name_clone = gml_name.get()->clone();
		d_name_widget->update_widget_from_string(*gml_name_clone);
	}
	else
	{
		d_name_widget->reset_widget_to_default_values();
	}

	return true;
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_create()
{
	try
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				*d_application_state.get_model_interface().access_model());

		// Get the FeatureCollection the user has selected.
		std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
				d_choose_feature_collection_widget->get_file_reference();
		GPlatesModel::FeatureCollectionHandle::weak_ref collection =
				collection_file_iter.first.get_file().get_feature_collection();
			
		// Create the feature.
		GPlatesModel::FeatureHandle::non_null_ptr_type feature =
				GPlatesModel::FeatureHandle::create(
						GPlatesModel::FeatureType::create_gpml("ScalarCoverage"));
		const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_focused_boundary_polygon,
				GPLATES_ASSERTION_SOURCE);

		// The density of points and random offset (converted from percentage to [0,1]).
		const unsigned int point_density_level = point_density_spin_box->value();
		const double point_random_offset = 0.01 * random_offset_spin_box->value();

		// Generate a uniform distribution of points (with some amount random offset).
		std::vector<GPlatesMaths::PointOnSphere> domain_points;
		GPlatesMaths::GeneratePoints::create_uniform_points_in_polygon(
					domain_points,
					*d_focused_boundary_polygon.get(),
					point_density_level,
					point_random_offset);

		const unsigned int num_domain_points = domain_points.size();
		if (num_domain_points == 0)
		{
			QMessageBox::critical(this,
					tr("Polygon boundary, of focused feature, was too small to contain points"),
					tr("Please either select a different focused feature or try increasing the density of points."));
			return;
		}

		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point =
				GPlatesMaths::MultiPointOnSphere::create_on_heap(domain_points);

		const double reconstruction_time = d_application_state.get_current_reconstruction_time();

		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
				reverse_reconstruct_geometry(multi_point, reconstruction_time, collection);

		// Get the initial crustal scalar value.
		double initial_crustal_scalar_value = 1.0;
		if (d_crustal_scalar_type == GPML_CRUSTAL_THINNING_FACTOR)
		{
			initial_crustal_scalar_value = crustal_thinning_factor_spin_box->value();
		}
		if (d_crustal_scalar_type == GPML_CRUSTAL_THICKNESS)
		{
			initial_crustal_scalar_value = crustal_thickness_spin_box->value();
		}

		std::vector<double> initial_crustal_scalar_values;
		initial_crustal_scalar_values.resize(num_domain_points, initial_crustal_scalar_value);

		// The domain (geometry) property.
		const GPlatesModel::PropertyValue::non_null_ptr_type domain_property =
				GPlatesAppLogic::GeometryUtils::create_geometry_property_value(present_day_geometry);

		// The range (scalars) property.
		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type range_property = GPlatesPropertyValues::GmlDataBlock::create();
		GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type crustal_scalar_xml_attrs;
		GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type crustal_scalar_range =
				GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
						d_crustal_scalar_type,
						crustal_scalar_xml_attrs,
						initial_crustal_scalar_values.begin(),
						initial_crustal_scalar_values.end());
		range_property->tuple_list_push_back(crustal_scalar_range);

		// The domain/range property names.
		const std::pair<GPlatesModel::PropertyName/*domain*/, GPlatesModel::PropertyName/*range*/> domain_range_property_names =
				GPlatesAppLogic::ScalarCoverageFeatureProperties::get_default_domain_range_property_names();

		// Add the domain/range properties.
		//
		// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
		// necessary time-dependent wrapper is added.
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				domain_range_property_names.first/*domain_property_name*/,
				domain_property);
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				domain_range_property_names.second/*range_property_name*/,
				range_property);

		// Add the geometry import time as the current reconstruction time.
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				GPlatesModel::PropertyName::create_gpml("geometryImportTime"),
				GPlatesModel::ModelUtils::create_gml_time_instant(
						GPlatesPropertyValues::GeoTimeInstant(reconstruction_time)));

		// Add the reconstruction plate ID property.
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				d_plate_id_widget->create_property_value_from_widget());

		// Add a gml:validTime Property.
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				GPlatesModel::PropertyName::create_gml("validTime"),
				d_time_period_widget->create_property_value_from_widget());

		// Add a gml:name Property.
		GPlatesModel::ModelUtils::add_property(
				feature_ref,
				GPlatesModel::PropertyName::create_gml("name"),
				d_name_widget->create_property_value_from_widget());

		// Add the feature to the collection.
		collection->add(feature);
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before we emit signals because we don't
		// know whose listening on those signals and they may be expecting model observers to
		// be up-to-date with the modified model.
		// Also this should be done before getting the application state reconstructs which
		// happens when the guard is released (because we modified the model).
		model_notification_guard.release_guard();

		Q_EMIT feature_created(feature->reference());
	
		accept();
	}
	catch (const ChooseFeatureCollectionWidget::NoFeatureCollectionSelectedException &)
	{
		QMessageBox::critical(this,
				tr("No feature collection selected"),
				tr("Please select a feature collection to add the new feature to."));
		return;
	}
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_cancel()
{
	reject();
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::setup_pages()
{
	// Limit values - if too large then generates too high a point density making GPlates very sluggish.
	point_density_spin_box->setMinimum(1);
	point_density_spin_box->setMaximum(10);
	point_density_spin_box->setValue(6); // default value
	QObject::connect(
			point_density_spin_box, SIGNAL(valueChanged(int)),
			this, SLOT(handle_point_density_spin_box_value_changed(int)));
	display_point_density_spacing();

	// Random offset is a percentage.
	random_offset_spin_box->setMinimum(0.0);
	random_offset_spin_box->setMaximum(100.0);
	random_offset_spin_box->setValue(0.0); // default value

	// Populate the crustal scalar type combobox.
	crustal_scalar_type_combobox->clear();
	crustal_scalar_type_combobox->addItem(convert_qualified_xml_name_to_qstring(GPML_CRUSTAL_THINNING_FACTOR));
	crustal_scalar_type_combobox->addItem(convert_qualified_xml_name_to_qstring(GPML_CRUSTAL_THICKNESS));
	if (d_crustal_scalar_type == GPML_CRUSTAL_THINNING_FACTOR)
	{
		crustal_scalar_type_combobox->setCurrentIndex(0);
		crustal_thinning_factor_widget->show();
		crustal_thickness_widget->hide();
	}
	if (d_crustal_scalar_type == GPML_CRUSTAL_THICKNESS)
	{
		crustal_scalar_type_combobox->setCurrentIndex(1);
		crustal_thickness_widget->show();
		crustal_thinning_factor_widget->hide();
	}

	// Crustal thinning factor spinboxes.
	crustal_thinning_factor_spin_box->setMinimum(0.001);
	crustal_thinning_factor_spin_box->setMaximum(1000.0);
	crustal_thinning_factor_spin_box->setValue(1.0); // default value
	// Crustal thickness spinboxes.
	crustal_thickness_spin_box->setMinimum(0.01);
	crustal_thickness_spin_box->setMaximum(1000.0);
	crustal_thickness_spin_box->setValue(40.0); // default value (kms)

	// The crustal scalar type combobox.
	QObject::connect(
			crustal_scalar_type_combobox, SIGNAL(activated(const QString &)),
			this, SLOT(handle_crustal_scalar_type_combobox_activated(const QString &)));

	// The various Edit widgets need pass focus along the chain if Enter is pressed.
	QObject::connect(
			d_plate_id_widget, SIGNAL(enter_pressed()),
			d_time_period_widget, SLOT(setFocus()));
	QObject::connect(
			d_time_period_widget, SIGNAL(enter_pressed()),
			d_name_widget, SLOT(setFocus()));
	QObject::connect(d_name_widget, SIGNAL(enter_pressed()),
			button_next, SLOT(setFocus()));
	
	// Reconfigure some accelerator keys that conflict.
	d_plate_id_widget->label()->setText(tr("Plate &ID:"));
	// And set the EditStringWidget's label to something suitable for a gml:name property.
	d_name_widget->label()->setText(tr("&Name:"));
	d_name_widget->label()->setHidden(false);

	QHBoxLayout *plate_id_layout;
	plate_id_layout = new QHBoxLayout;
	plate_id_layout->setSpacing(2);
	plate_id_layout->setMargin(0);
	plate_id_layout->addWidget(d_plate_id_widget);

	QVBoxLayout *edit_layout;
	edit_layout = new QVBoxLayout;
	edit_layout->addItem(plate_id_layout);
	edit_layout->addWidget(d_time_period_widget);
	edit_layout->addWidget(d_name_widget);
	edit_layout->insertStretch(-1);
	common_feature_properties_group_box->setLayout(edit_layout);

	QObject::connect(
			button_create, SIGNAL(clicked()),
			this, SLOT(handle_create()));
	QObject::connect(
			button_cancel, SIGNAL(clicked()),
			this, SLOT(handle_cancel()));

	QObject::connect(
			button_previous, SIGNAL(clicked()),
			this, SLOT(handle_previous()));
	QObject::connect(
			button_next, SIGNAL(clicked()),
			this, SLOT(handle_next()));

	// Pushing Enter or double-clicking should cause the create button to focus.
	QObject::connect(
			d_choose_feature_collection_widget, SIGNAL(item_activated()),
			button_create, SLOT(setFocus()));

	QObject::connect(
			push_button_help_scalar_type, SIGNAL(clicked()),
			d_help_scalar_type_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_point_distribution, SIGNAL(clicked()),
			d_help_point_distribution_dialog, SLOT(show()));
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_previous()
{
	if (stacked_widget->currentIndex() == COLLECTION_PAGE)
	{
		make_generate_points_page_current();
	}
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_next()
{
	if (stacked_widget->currentIndex() == GENERATE_POINTS_PAGE)
	{
		make_feature_collection_page_current();
	}
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_crustal_scalar_type_combobox_activated(
		const QString &text)
{
	boost::optional<GPlatesPropertyValues::ValueObjectType> crustal_scalar_type =
			GPlatesModel::convert_qstring_to_qualified_xml_name<
					GPlatesPropertyValues::ValueObjectType>(text);
	if (crustal_scalar_type)
	{
		d_crustal_scalar_type = crustal_scalar_type.get();

		// Show/hide options for thickness / thinning factor.
		if (d_crustal_scalar_type == GPML_CRUSTAL_THINNING_FACTOR)
		{
			crustal_thinning_factor_widget->show();
			crustal_thickness_widget->hide();
		}
		if (d_crustal_scalar_type == GPML_CRUSTAL_THICKNESS)
		{
			crustal_thickness_widget->show();
			crustal_thinning_factor_widget->hide();
		}
	}
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::handle_point_density_spin_box_value_changed(
		int)
{
	display_point_density_spacing();
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::make_generate_points_page_current()
{
	button_previous->setEnabled(false);
	button_next->setEnabled(true);
	button_create->setEnabled(false);
	stacked_widget->setCurrentIndex(GENERATE_POINTS_PAGE);
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::make_feature_collection_page_current()
{
	button_previous->setEnabled(true);
	button_next->setEnabled(false);
	button_create->setEnabled(true);
	stacked_widget->setCurrentIndex(COLLECTION_PAGE);

	d_choose_feature_collection_widget->initialise();
	d_choose_feature_collection_widget->setFocus();
}


void
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::display_point_density_spacing()
{
	const unsigned int point_density_level = point_density_spin_box->value();

	// The side of a level 0 quad face of a Rhombic Triacontahedron is about 40 degrees.
	// And each subdivision level reduces that by about a half.
	const double point_density_spacing_degrees = 40.0 / (1 << point_density_level);

	point_density_spacing_line_edit->setText(QString("%1").arg(point_density_spacing_degrees));
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesQtWidgets::GenerateCrustalThicknessPointsDialog::reverse_reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geom,
		const double &reconstruction_time,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
			d_plate_id_widget->create_integer_plate_id_from_widget();

	// We need to convert geometry to present day coordinates. This is because the geometry is
	// currently reconstructed geometry at the current reconstruction time.

	// Get the reconstruct layers (if any) that reconstruct the feature collection that our
	// feature will be added to.
	std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_outputs;
	GPlatesAppLogic::LayerProxyUtils::find_reconstruct_layer_outputs_of_feature_collection(
			reconstruct_layer_outputs,
			feature_collection_ref,
			d_application_state.get_reconstruct_graph());

	// If there's no reconstruct layers then use the default reconstruction tree creator.
	// This probably shouldn't happen though.
	// Currently we just use the default reconstruction tree layer.
	const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
			!reconstruct_layer_outputs.empty()
			// FIXME: We arbitrarily choose first layer if feature is reconstructed by multiple layers
			// (for example if the user is reconstructing the same feature using two different reconstruction trees)...
			? reconstruct_layer_outputs.front()->get_reconstruct_method_context().reconstruction_tree_creator
			: d_application_state.get_current_reconstruction().get_default_reconstruction_layer_output()->get_reconstruction_tree_creator();

	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	// Reverse reconstruct by plate ID.
	return GPlatesAppLogic::ReconstructUtils::reconstruct_by_plate_id(
				geom,
				reconstruction_plate_id,
				*reconstruction_tree,
				true/*reverse_reconstruct*/);
}
