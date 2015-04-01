/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute file_iter and/or modify file_iter under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that file_iter will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <memory>
#include <boost/foreach.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QMessageBox>

#include "CoRegistrationLayerConfigurationDialog.h"

#include "GlobeAndMapWidget.h"
#include "ReconstructionViewWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/CoRegistrationLayerTask.h"
#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructLayerProxy.h"

#include "data-mining/PopulateShapeFileAttributesVisitor.h"
#include "data-mining/CoRegConfigurationTable.h"
#include "data-mining/RegionOfInterestFilter.h"
#include "data-mining/SeedSelfFilter.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLContext.h"
#include "opengl/GLRasterCoRegistration.h"
#include "opengl/GLRenderer.h"

#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"

#include "property-values/RawRasterUtils.h"

#include "utils/UnicodeStringUtils.h"

namespace
{
	const char* DISTANCE = QT_TR_NOOP("Distance");
	const char* PRESENCE = QT_TR_NOOP("Presence");
	const char* NUM_ROI  = QT_TR_NOOP("Number in Region");
	const char* HIGHEST = QT_TR_NOOP("Highest"); // Used for raster level-of-detail.
}


GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::CoRegistrationLayerConfigurationDialog(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		boost::weak_ptr<GPlatesPresentation::VisualLayer> layer) :
	d_application_state(view_state.get_application_state()),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_visual_layers(view_state.get_visual_layers()),
	d_visual_layer(layer),
	d_raster_co_registration_supported(false)
{
	setupUi(this);

	QObject::connect(target_layers_list_widget, SIGNAL(itemSelectionChanged()),
		this, SLOT(react_target_layer_selection_changed()));

	QObject::connect(target_layers_list_widget, SIGNAL(itemClicked(QListWidgetItem *)),
		this, SLOT(react_target_layer_selection_changed()));

	QObject::connect(add_push_button, SIGNAL(clicked()),
		this, SLOT(react_add_configuration_row()));

	QObject::connect(button_box, SIGNAL(clicked(QAbstractButton*)), 
		this, SLOT(apply(QAbstractButton*)));

	QObject::connect(relational_radio_button, SIGNAL(clicked()), 
		this, SLOT(populate_relational_attributes()));

	QObject::connect(co_reg_radio_buttton, SIGNAL(clicked()), 
		this, SLOT(populate_coregistration_attributes()));

	QObject::connect(remove_push_button, SIGNAL(clicked()), 
		this, SLOT(remove()));

	QObject::connect(remove_all_push_button, SIGNAL(clicked()), 
		this, SLOT(remove_all()));

	QObject::connect(co_reg_cfg_table_widget, SIGNAL(cellChanged(int, int)), 
		this, SLOT(cfg_table_cell_changed(int,int)));

	QObject::connect(
		&d_application_state.get_reconstruct_graph(),
		SIGNAL(layer_added_input_connection(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer,
				GPlatesAppLogic::Layer::InputConnection)),
		this,
		SLOT(handle_co_registration_input_layer_list_changed(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer )));

	QObject::connect(
		&d_application_state.get_reconstruct_graph(),
		SIGNAL(layer_removed_input_connection(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer)),
		this,
		SLOT(handle_co_registration_input_layer_list_changed(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer )));

	QObject::connect(
		&d_application_state.get_reconstruct_graph(),
		SIGNAL(layer_activation_changed(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer,
				bool)),
		this,
		SLOT(handle_co_registration_input_layer_list_changed(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer )));

	// Register a model callback so we can update our GUI whenever the feature store is modified.
	// This is because the list of available attributes might have changed.
	//
	// We could keep a list of input feature collections (to the target input layers)
	// and only detect when those feature collections have changed.
	// But it's easier to simply detect if any feature collection has been modified and update
	// our GUI even if we're not (indirectly) referencing that feature collection.
	// Besides the cost of updating the GUI should be relatively small and it'll only happen
	// when the GUI is visible.
	d_callback_feature_store.attach_callback(new UpdateWhenFeatureStoreIsModified(*this));

	co_reg_cfg_table_widget->resizeColumnsToContents();
	attributes_list_widget->setSelectionMode(QAbstractItemView::MultiSelection);
}

			
void 
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::pop_up()
{
	// Start off listing the co-registration attributes (rasters currently only have co-registration
	// attributes so this makes it easier for the user).
	co_reg_radio_buttton->setChecked(true);

	// Note: We don't test for raster co-registration in constructor since we want the GUI system
	// to be stabile/initialised first since we use the active OpenGL context which is associated
	// with the globe/map window.
	d_raster_co_registration_supported = is_raster_co_registration_supported();

	QtWidgetUtils::pop_up_dialog(this);

	// Note: We update *after* popping up the dialog because it only updates when dialog is *visible*.
	update();
}


bool
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::is_raster_co_registration_supported() const
{
	// We need an OpenGL renderer before we can query support.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = create_gl_renderer();

	// Start a begin_render/end_render scope.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

	return GPlatesOpenGL::GLRasterCoRegistration::is_supported(*renderer);
}


std::vector<GPlatesAppLogic::Layer>
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::get_input_seed_layers() const
{
	return get_input_layers(
			GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES,
			false/*target_layers*/);
}


std::vector<GPlatesAppLogic::Layer>
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::get_input_target_layers() const
{
	return get_input_layers(
			GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES,
			true/*target_layers*/);
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

std::vector<GPlatesAppLogic::Layer>
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::get_input_layers(
		GPlatesAppLogic::LayerInputChannelName::Type channel_name,
		bool target_layers) const
{
	std::vector<GPlatesAppLogic::Layer> input_layers;

	boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock();
	if (locked_visual_layer)
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

		std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections =
				layer.get_channel_inputs(channel_name);

		BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& connection, input_connections)
		{
			// The inputs of co-registration layer are the output of other layers.
			// We only look for inputs that are layers (not files - shouldn't be any file inputs anyway).
			boost::optional<GPlatesAppLogic::Layer> input_layer = connection.get_input_layer();
			if (!input_layer)
			{
				continue;
			}

			// We don't include inactive/disabled layers since they cannot do anything.
			if (!input_layer->is_active())
			{
				continue;
			}

			// We're expecting only layers of type RECONSTRUCT and RASTER for target layers and
			// RECONSTRUCT for seed layers.
			const GPlatesAppLogic::LayerTaskType::Type input_layer_type = input_layer->get_type();
			if (target_layers)
			{
				if (input_layer_type != GPlatesAppLogic::LayerTaskType::RECONSTRUCT &&
					input_layer_type != GPlatesAppLogic::LayerTaskType::RASTER)
				{
					continue;
				}
			}
			else
			{
				if (input_layer_type != GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
				{
					continue;
				}
			}

			// For raster layers we only list those rasters containing numerical data (ie, not
			// containing RGBA colour data) because we're doing data analysis not visualisation.
			if (target_layers &&
				input_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
			{
				// NOTE: We list raster layers even if raster co-registration is not supported
				// (because the necessary OpenGL extensions not available).
				// When the user subsequently selects a raster layer a message box pops up
				// informing them that raster co-registration is not supported on their system.

				// See if any raster bands contain numerical data - we need at least one.
				if (!does_raster_layer_contain_numerical_data(input_layer.get()))
				{
					continue;
				}
			}

			input_layers.push_back(input_layer.get());
		}
	}

	return input_layers;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


bool
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::does_raster_layer_contain_numerical_data(
		const GPlatesAppLogic::Layer &raster_layer) const
{
	// Get the raster layer proxy.
	boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> raster_layer_proxy =
			raster_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			raster_layer_proxy,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the raster bands.
	const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names =
			raster_layer_proxy.get()->get_raster_band_names();
	BOOST_FOREACH(
			const GPlatesPropertyValues::GpmlRasterBandNames::BandName &raster_band_name,
			raster_band_names)
	{
		// If the raster band contains numerical data (ie, it's not colour data) then we can
		// use it for co-registration.
		if (raster_layer_proxy.get()->does_raster_band_contain_numerical_data(raster_band_name.get_name()->get_value()))
		{
			return true;
		}
	}

	return false;
}


GPlatesOpenGL::GLRenderer::non_null_ptr_type
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::create_gl_renderer() const
{
	// Get an OpenGL context since the (raster) co-registration is accelerated with OpenGL.
	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
			d_viewport_window->reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	return gl_context->create_renderer();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_target_layers_list() const
{
	// Clear the existing entries.
	target_layers_list_widget->clear();

	std::vector<GPlatesAppLogic::Layer> target_layers = get_input_target_layers();

	BOOST_FOREACH(const GPlatesAppLogic::Layer& target_layer, target_layers)
	{
		// Get the visual layer associated with the (application-logic) layer.
		boost::weak_ptr<const GPlatesPresentation::VisualLayer> target_visual_layer =
				d_visual_layers.get_visual_layer(target_layer);
		if (boost::shared_ptr<const GPlatesPresentation::VisualLayer> locked_target_visual_layer = target_visual_layer.lock())
		{
			const QString target_layer_name = locked_target_visual_layer->get_name();

			target_layers_list_widget->addItem(
					new LayerItem(target_layer, target_layer_name));
		}
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::react_target_layer_selection_changed()
{
	// Get the currently selected target layer.
	LayerItem* current_target_layer_item = dynamic_cast< LayerItem* > (target_layers_list_widget->currentItem());
	if (!current_target_layer_item)
	{
		//qDebug() << "The current target layer item is null.";
		return;
	}

	// If the layer is a raster layer and raster co-registration is not supported on the run-time
	// system then pop-up a message box to the user.
	if (current_target_layer_item->layer.get_type() == GPlatesAppLogic::LayerTaskType::RASTER &&
		!d_raster_co_registration_supported)
	{
		// Clear the list of attributes (from the previous layer selection) before popping up the message.
		attributes_list_widget->clear();

		const QString message = tr(
				"Raster co-registration requires roughly OpenGL 2.0/3.0 compliant graphics hardware "
				"(specifically floating-point textures and frame buffer objects).\n\n"
				"Please select a non-raster layer instead.");
		QMessageBox::warning(
				this,
				tr("Raster co-registration not supported on this graphics hardware"),
				message,
				QMessageBox::Ok,
				QMessageBox::Ok);

		return;
	}

	populate_attributes();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_attributes()
{
	attributes_list_widget->clear();
	
	LayerItem* current_target_layer_item = dynamic_cast< LayerItem* > (target_layers_list_widget->currentItem());
	if (!current_target_layer_item)
	{
		//qDebug() << "The current target layer item is null.";
		return;
	}

	if (relational_radio_button->isChecked()) 
	{
		populate_relational_attributes();
	}
	else 
	{
		populate_coregistration_attributes();
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_relational_attributes()
{
	attributes_list_widget->clear();
	LayerItem* current_target_layer_item = 
		dynamic_cast< LayerItem* > (target_layers_list_widget->currentItem());
	if(!current_target_layer_item)
	{
		return;
	}

	const GPlatesAppLogic::Layer target_layer = current_target_layer_item->layer;

	// We're expecting only layers of type RECONSTRUCT and RASTER.
	const GPlatesAppLogic::LayerTaskType::Type target_layer_type = target_layer.get_type();

	if (target_layer_type == GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
	{
		QListWidgetItem *item = 
			new AttributeListItem(
					QApplication::tr(DISTANCE), 
					GPlatesDataMining::DISTANCE_ATTRIBUTE);
		attributes_list_widget->addItem(item);

		item = 
			new AttributeListItem(
					QApplication::tr(PRESENCE), 
					GPlatesDataMining::PRESENCE_ATTRIBUTE);
		attributes_list_widget->addItem(item);

		item = 
			new AttributeListItem(
					QApplication::tr(NUM_ROI), 
					GPlatesDataMining::NUMBER_OF_PRESENCE_ATTRIBUTE);
		attributes_list_widget->addItem(item);
	}
	else if (target_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
	{
		// No relational attributes for rasters yet.
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_coregistration_attributes()
{
	attributes_list_widget->clear();
	LayerItem* current_target_layer_item = 
		dynamic_cast< LayerItem* > (target_layers_list_widget->currentItem());
	if(!current_target_layer_item)
	{
		return;
	}
	
	const GPlatesAppLogic::Layer target_layer = current_target_layer_item->layer;

	// We're expecting only layers of type RECONSTRUCT and RASTER.
	const GPlatesAppLogic::LayerTaskType::Type target_layer_type = target_layer.get_type();

	if (target_layer_type == GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
	{
		populate_reconstructed_geometries_coregistration_attributes(target_layer);
	}
	else if (target_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
	{
		populate_raster_coregistration_attributes(target_layer);
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_reconstructed_geometries_coregistration_attributes(
		const GPlatesAppLogic::Layer &target_layer)
{
	// We only get here for 'reconstruct geometries' target layers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			target_layer.get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCT,
			GPLATES_ASSERTION_SOURCE);

	std::set<GPlatesModel::PropertyName> property_names;
	std::set< QString > shapefile_attr_names;
	get_unique_attribute_names(target_layer, property_names, shapefile_attr_names);
	
	//hack for Jo
	property_names.insert(GPlatesModel::PropertyName::create_gpml("gpml feature type"));

	// Attributes for gpml properties.
	std::set< GPlatesModel::PropertyName >::const_iterator property_names_iter = property_names.begin();
	std::set< GPlatesModel::PropertyName >::const_iterator property_names_end = property_names.end();
	for ( ; property_names_iter != property_names_end; property_names_iter++)
	{
		const GPlatesModel::PropertyName &property_name = *property_names_iter;

		QListWidgetItem *attr_item = 
			new AttributeListItem(
					GPlatesUtils::make_qstring_from_icu_string(property_name.get_name()),
					GPlatesDataMining::CO_REGISTRATION_GPML_ATTRIBUTE);

		attributes_list_widget->addItem(attr_item);
	}

	// Attributes for shapefile attributes.
	std::set<QString>::const_iterator shapefile_attr_names_iter = shapefile_attr_names.begin();
	std::set<QString>::const_iterator shapefile_attr_names_end = shapefile_attr_names.end();
	for ( ; shapefile_attr_names_iter != shapefile_attr_names_end; shapefile_attr_names_iter++)
	{
		const QString &shapefile_attr_name = *shapefile_attr_names_iter;

		QListWidgetItem *attr_item = 
				new AttributeListItem(
						shapefile_attr_name,
						GPlatesDataMining::CO_REGISTRATION_SHAPEFILE_ATTRIBUTE);

		attributes_list_widget->addItem(attr_item);
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::get_unique_attribute_names(
		const GPlatesAppLogic::Layer &target_layer,
		std::set< GPlatesModel::PropertyName > &property_names,
		std::set< QString > &shapefile_attr_names)
{
	// We only get here for 'reconstruct geometries' target layers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			target_layer.get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCT,
			GPLATES_ASSERTION_SOURCE);

	// The 'reconstruct geometries' layer has input feature collections on its main input channel.
	const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
			target_layer.get_main_input_feature_collection_channel();
	const std::vector<GPlatesAppLogic::Layer::InputConnection> main_inputs =
			target_layer.get_channel_inputs(main_input_channel);

	// Loop over all input connections to get the files (feature collections) for the current target layer.
	BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& main_input_connection, main_inputs)
	{
		boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = 
				main_input_connection.get_input_file();
		// If it's not a file (ie, it's a layer) then continue to the next file.
		if(!input_file)
		{
			continue;
		}

		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				input_file->get_file().get_file().get_feature_collection();

		GPlatesModel::FeatureCollectionHandle::const_iterator features_iter = 
			feature_collection_ref->begin();
		GPlatesModel::FeatureCollectionHandle::const_iterator features_end = 
			feature_collection_ref->end();

		for( ; features_iter != features_end; features_iter++)
		{
			GPlatesModel::FeatureHandle::const_iterator properties_iter = (*features_iter)->begin();
			GPlatesModel::FeatureHandle::const_iterator properties_end = (*features_iter)->end();

			for( ; properties_iter != properties_end; properties_iter++)
			{
				const GPlatesModel::PropertyName &property_name = (*properties_iter)->get_property_name();

				GPlatesDataMining::CheckAttrTypeVisitor visitor;
				(*properties_iter)->accept_visitor(visitor);

				//hacking code for shape file.
				if (GPlatesUtils::make_qstring_from_icu_string(property_name.get_name()) == "shapefileAttributes")
				{
					// Add the shapefile attribute names.
					typedef std::multimap< QString, GPlatesDataMining::AttributeTypeEnum > shape_attr_map_type;
					const shape_attr_map_type &shape_attr_map = visitor.shape_map();
					BOOST_FOREACH(const shape_attr_map_type::value_type &shape_attr_map_entry, shape_attr_map)
					{
						shapefile_attr_names.insert(shape_attr_map_entry.first);
					}

					// Add to the attribute type mapping.
					d_attr_name_type_map.insert(
							visitor.shape_map().begin(),
							visitor.shape_map().end());
				}
				else
				{
					// Add the gpml property names.
					property_names.insert( property_name );

					// Add to the attribute type mapping.
					d_attr_name_type_map.insert(
							std::pair< QString, GPlatesDataMining::AttributeTypeEnum >(
									GPlatesUtils::make_qstring_from_icu_string(property_name.get_name()),
									visitor.type() ) );
				}
			}
		}
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::populate_raster_coregistration_attributes(
		const GPlatesAppLogic::Layer &target_layer)
{
	boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> raster_layer_proxy =
			target_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();

	// We only get here for 'raster' target layers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			raster_layer_proxy,
			GPLATES_ASSERTION_SOURCE);

	// We'll treat the raster band names as if they were attributes.
	// They are not really feature property values but they are attributes in a sense because
	// each raster band can be thought of as a coverage of geometry points where the attribute
	// varies over the geometry (instead of being constant) and each band is a different parameter
	// (eg, topography or gravity).
	const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names =
			raster_layer_proxy.get()->get_raster_band_names();
	BOOST_FOREACH(
			const GPlatesPropertyValues::GpmlRasterBandNames::BandName &raster_band_name,
			raster_band_names)
	{
		// If the raster band does not contain numerical data (ie, it's colour data) then we don't
		// use it for co-registration.
		if (!raster_layer_proxy.get()->does_raster_band_contain_numerical_data(raster_band_name.get_name()->get_value()))
		{
			continue;
		}

		const QString raster_attr_name =
				GPlatesUtils::make_qstring_from_icu_string(raster_band_name.get_name()->get_value().get());

		QListWidgetItem *attr_item = 
				new AttributeListItem(
						raster_attr_name,
						GPlatesDataMining::CO_REGISTRATION_RASTER_ATTRIBUTE);

		attributes_list_widget->addItem(attr_item);
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::react_add_configuration_row()
{
	QList<QListWidgetItem*> attr_items = attributes_list_widget->selectedItems();
	// Make sure a target layer and at least one attribute has been selected.
	if( !target_layers_list_widget->currentItem() || 
		attr_items.size() == 0)
	{
		return;
	}

	// Get the target layer type - it determines what reducer options are available for example.
	LayerItem* current_target_layer_item = dynamic_cast<LayerItem *>(target_layers_list_widget->currentItem());
	if (!current_target_layer_item)
	{
		return;
	}
	const GPlatesAppLogic::Layer target_layer = current_target_layer_item->layer;
	const GPlatesAppLogic::LayerTaskType::Type target_layer_type = target_layer.get_type();

	// Iterate over the selected attributes.
	BOOST_FOREACH(QListWidgetItem* item, attr_items)
	{
		AttributeListItem *attr_item = dynamic_cast< AttributeListItem* >(item);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attr_item,
				GPLATES_ASSERTION_SOURCE);

		int row_num=co_reg_cfg_table_widget->rowCount();
		co_reg_cfg_table_widget->insertRow(row_num);

		//Attribute Name column 
		AttributeTableItem * attr_name_item = 
			new AttributeTableItem(
					attr_item->text(),
					attr_item->attr_type);
		attr_name_item->setFlags(attr_name_item->flags() & ~Qt::ItemIsEditable);
		co_reg_cfg_table_widget->setItem(
				row_num, 
				ATTRIBUTE_NAME, 
				attr_name_item);

		//association name column
		co_reg_cfg_table_widget->setItem(
				row_num, 
				ASSOCIATION_NAME, 
				new QTableWidgetItem(QString("Assoc_")+ QString().setNum(row_num)));

		//Data Operator column
		QComboBox* combo = new QComboBox();       
		QObject::connect(combo, SIGNAL(currentIndexChanged(int)), 
			this, SLOT(update_cfg_table()));
		co_reg_cfg_table_widget->setCellWidget(
				row_num, 
				REDUCER, 
				combo);

		setup_reducer_combobox(
				attr_item->text(),
				combo,
				target_layer_type);
		
		// Layer Name column
		LayerTableItem* layer_name_item = 
			new LayerTableItem(
					current_target_layer_item->layer,
					current_target_layer_item->label);
		layer_name_item->setFlags(layer_name_item->flags() & ~Qt::ItemIsEditable);
		co_reg_cfg_table_widget->setItem(
				row_num,
				LAYER_NAME,
				layer_name_item);

		//Association Type column
		//TODO: To be finished...
		QComboBox* association_combo = new QComboBox();       
		QObject::connect(association_combo, SIGNAL(currentIndexChanged(int)), 
			this, SLOT(update_cfg_table()));
		co_reg_cfg_table_widget->setCellWidget(
				row_num, 
				FILTER_TYPE, 
				association_combo);
		
		setup_association_type_combobox(association_combo);

		//Range column
		QDoubleSpinBox* ROI_range_spinbox = new QDoubleSpinBox(); 
		QObject::connect(ROI_range_spinbox, SIGNAL(valueChanged(double)), 
			this, SLOT(update_cfg_table()));
		ROI_range_spinbox->setRange(0,25000);
		ROI_range_spinbox->setValue(0);
		co_reg_cfg_table_widget->setCellWidget(
				row_num, 
				RANGE, 
				ROI_range_spinbox);

		// If it's a raster target layer then it uses extra raster-only columns.
		if (target_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
		{
			// Raster level-of-detail column.
			std::auto_ptr<QComboBox> raster_level_of_detail_combo_box(new QComboBox());

			// Only add the combo box if we were able to determine the number of raster levels of detail.
			if (setup_raster_level_of_detail_combo_box(
				raster_level_of_detail_combo_box.get(),
				target_layer,
				attr_item->text()))
			{
				co_reg_cfg_table_widget->setCellWidget(
						row_num, 
						RASTER_LEVEL_OF_DETAIL, 
						raster_level_of_detail_combo_box.release());
			}

			// Raster fill polygons column.
			QCheckBox *raster_fill_polygons_check_box = new QCheckBox();
			co_reg_cfg_table_widget->setCellWidget(
					row_num, 
					RASTER_FILL_POLYGONS, 
					raster_fill_polygons_check_box);

			setup_raster_fill_polygons_check_box(raster_fill_polygons_check_box);
		}
	}
	co_reg_cfg_table_widget->resizeColumnsToContents();
	update_cfg_table();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::setup_reducer_combobox(
		const QString& attribute_name,	
		QComboBox* combo,
		const GPlatesAppLogic::LayerTaskType::Type target_layer_type)
{
	if (relational_radio_button->isChecked()) 
	{
		// There are no relational attributes for rasters.
		if (target_layer_type != GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
		{
			return;
		}

		if(attribute_name == DISTANCE)
		{
			combo->addItem(
					QApplication::tr("Min"),
					GPlatesDataMining::REDUCER_MIN);
			combo->addItem(
					QApplication::tr("Max"),
					GPlatesDataMining::REDUCER_MAX);
			combo->addItem(
					QApplication::tr("Mean"),
					GPlatesDataMining::REDUCER_MEAN);
			combo->addItem(
					QApplication::tr("Median"),
					GPlatesDataMining::REDUCER_MEDIAN);
			return;
		}

		if(attribute_name == PRESENCE || attribute_name == NUM_ROI)
		{
			combo->addItem(
					QApplication::tr("Lookup"), 
					GPlatesDataMining::REDUCER_LOOKUP);
			return;
		}

		return;
	}

	// Rasters have a fixed set of reducer options that is independent of the attribute type.
	// Mainly because rasters only contain numerical data and hence the attribute type is
	// effectively always a number type (ie, not a string type).
	if (target_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
	{
		combo->addItem(
				QApplication::tr("Min"),
				GPlatesDataMining::REDUCER_MIN);
		combo->addItem(
				QApplication::tr("Max"),
				GPlatesDataMining::REDUCER_MAX);
		combo->addItem(
				QApplication::tr("Mean"),
				GPlatesDataMining::REDUCER_MEAN);
		combo->addItem(
				QApplication::tr("Std Dev"),
				GPlatesDataMining::REDUCER_STANDARD_DEVIATION);

		return;
	}

	// The attribute name map is only available if the co-registration radio button is checked.
	GPlatesDataMining::AttributeTypeEnum a_type = GPlatesDataMining::Unknown_Type;
	if(d_attr_name_type_map.find(attribute_name) != d_attr_name_type_map.end())
	{
		a_type = d_attr_name_type_map.find(attribute_name)->second;
	}

	if(GPlatesDataMining::String_Attribute == a_type )
	{
		combo->addItem(
				QApplication::tr("Lookup"), 
				GPlatesDataMining::REDUCER_LOOKUP);
		combo->addItem(
				QApplication::tr("Vote"),
				GPlatesDataMining::REDUCER_VOTE);
		return;
	}

	if(GPlatesDataMining::Number_Attribute == a_type || GPlatesDataMining::Unknown_Type == a_type)
	{
		combo->addItem(
				QApplication::tr("Lookup"), 
				GPlatesDataMining::REDUCER_LOOKUP);
		combo->addItem(
				QApplication::tr("Vote"),
				GPlatesDataMining::REDUCER_VOTE);
		combo->addItem(
				QApplication::tr("Min"),
				GPlatesDataMining::REDUCER_MIN);
		combo->addItem(
				QApplication::tr("Max"),
				GPlatesDataMining::REDUCER_MAX);
		combo->addItem(
				QApplication::tr("Mean"),
				GPlatesDataMining::REDUCER_MEAN);
		combo->addItem(
				QApplication::tr("Median"),
				GPlatesDataMining::REDUCER_MEDIAN);
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::setup_association_type_combobox(
			QComboBox* combo)
{
	combo->addItem(QApplication::tr("Region of Interest"));
}


bool
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::setup_raster_level_of_detail_combo_box(
		QComboBox* combo_box,
		const GPlatesAppLogic::Layer &raster_target_layer,
		const QString &raster_band_name)
{
	// Get the raster layer proxy.
	boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> raster_layer_proxy =
			raster_target_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();
	if (!raster_layer_proxy)
	{
		// We won't assert just in case raster layer has been deactivated and we haven't handled that.
		return false;
	}

	// We need an OpenGL renderer before we can query multi-resolution rasters.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = create_gl_renderer();

	// Start a begin_render/end_render scope.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

	// Get the multi-resolution raster from the layer proxy.
	// The number of levels of detail should be independent of time since a time-dependent raster
	// should have the same image dimensions for each raster in the time sequence - so we'll just
	// get the multi-resolution raster for the current reconstruction time.
	boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type> multi_resolution_raster =
			raster_layer_proxy.get()->get_multi_resolution_data_raster(
			*renderer,
			GPlatesUtils::make_icu_string_from_qstring(raster_band_name));
	if (!multi_resolution_raster)
	{
		// We shouldn't get here because the raster doesn't contain numerical data or because
		// floating-point textures are not supported (those should already have been checked).
		// So there's a lower-level error and the co-registration will end up skipping this raster.
		return false;
	}

	const unsigned int num_levels_of_detail = multi_resolution_raster.get()->get_num_levels_of_detail();
	for (unsigned int lod = 0; lod < num_levels_of_detail; ++lod)
	{
		QString label;
		if (lod == 0)
		{
			// Write "Highest(0)".
			label = QString("%1(0)").arg(QApplication::tr(HIGHEST));
		}
		else
		{
			label = QString("%1").arg(lod);
		}

		combo_box->addItem(label, lod);
	}

	return true;
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::setup_raster_fill_polygons_check_box(
		QCheckBox* check_box)
{
	// Turn fill polygons on by default.
	check_box->setCheckState(Qt::Checked);
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::update(
		bool update_only_when_visible)
{
	// If this dialog is not visible then we don't need to update it - it'll get updated when it pops up.
	if (update_only_when_visible && !isVisible())
	{
		return;
	}

	// Re-populate the list of target layers.
	populate_target_layers_list();

	// Re-populate the list of attributes.
	populate_attributes();

	// Remove any configuration rows that reference target layers not existing anymore.
	remove_config_rows_referencing_nonexistent_target_layer();

	// Generate a new co-registration configuration table.
	GPlatesDataMining::CoRegConfigurationTable cfg_table;
	create_configuration_table(cfg_table);

	// If the configuration has changed then let the co-registration layer know.
	if (d_cfg_table != cfg_table)
	{
		d_cfg_table = cfg_table;

		set_configuration_table_on_layer(cfg_table);

		// Force a reconstruction so that the co-registration layer uses the updated configuration.
		d_application_state.reconstruct();
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::create_configuration_table(
		GPlatesDataMining::CoRegConfigurationTable &cfg_table)
{
	cfg_table.clear(); //Clean up the table in case it's not clear already.

	const int num_rows = co_reg_cfg_table_widget->rowCount();

	for(int row = 0; row < num_rows; row++)
	{
		QTableWidgetItem* assoc_name_item = 
			dynamic_cast<QTableWidgetItem*>(
					co_reg_cfg_table_widget->item(row, ASSOCIATION_NAME));
		LayerTableItem* layer_item = 
			dynamic_cast< LayerTableItem* > (
					co_reg_cfg_table_widget->item(row, LAYER_NAME) );
		AttributeTableItem* attr_item = 
			dynamic_cast< AttributeTableItem* > (
					co_reg_cfg_table_widget->item(row, ATTRIBUTE_NAME) );
		QComboBox* reducer_box = 
			dynamic_cast< QComboBox* >(
					co_reg_cfg_table_widget->cellWidget(row, REDUCER) );
		QDoubleSpinBox* spinbox_ROI_range = 
			dynamic_cast< QDoubleSpinBox* >(
					co_reg_cfg_table_widget->cellWidget(row, RANGE) );
		
		if( !(	assoc_name_item			&&
				layer_item				&&
				attr_item				&&
				reducer_box				&&
				spinbox_ROI_range) )
		{
			qDebug() << "CoRegistrationLayerConfigurationDialog: Invalid input table item found! Skip this iteration";
			continue;
		}

		// The target layer and type.
		const GPlatesAppLogic::Layer target_layer = layer_item->layer;
		const GPlatesAppLogic::LayerTaskType::Type target_layer_type = target_layer.get_type();
		
		const GPlatesDataMining::ReducerType reducer_operation = 
			static_cast<GPlatesDataMining::ReducerType>(
					reducer_box->itemData(reducer_box->currentIndex()).toUInt());

		GPlatesDataMining::ConfigurationTableRow config_row;

		config_row.target_layer = target_layer;
 
		//TODO: TO BE IMPLEMENTED
		config_row.assoc_name = assoc_name_item->text();
		config_row.filter_cfg.reset(new GPlatesDataMining::RegionOfInterestFilter::Config(spinbox_ROI_range->value()));
		config_row.layer_name = layer_item->text();
		config_row.attr_name = attr_item->text();
		config_row.attr_type = attr_item->attr_type;
		config_row.reducer_type = reducer_operation;

		// If the current target layer is a seed layer and the region of interest is zero
		// then an optimised filter is used to return seed features.
		if (target_layer_type == GPlatesAppLogic::LayerTaskType::RECONSTRUCT &&
			0 == GPlatesMaths::Real(spinbox_ROI_range->value()))
		{
			const std::vector<GPlatesAppLogic::Layer> input_seed_layers = get_input_seed_layers();
			BOOST_FOREACH(const GPlatesAppLogic::Layer &input_seed_layer, input_seed_layers)
			{
				if (input_seed_layer == target_layer)
				{
					config_row.filter_cfg.reset(new GPlatesDataMining::SeedSelfFilter::Config());
					break;
				}
			}
		}

		// Raster target layers have extra configuration columns.
		if (target_layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
		{
			QComboBox* raster_level_of_detail_combo_box =
					dynamic_cast<QComboBox *>(
							co_reg_cfg_table_widget->cellWidget(row, RASTER_LEVEL_OF_DETAIL));

			QCheckBox *raster_fill_polygons_check_box =
					dynamic_cast<QCheckBox *>(
							co_reg_cfg_table_widget->cellWidget(row, RASTER_FILL_POLYGONS));

			if (raster_level_of_detail_combo_box == NULL ||
				raster_fill_polygons_check_box == NULL)
			{
				qWarning() << "CoRegistrationLayerConfigurationDialog: Invalid raster input table item found! Skip this iteration";
				continue;
			}
		
			//
			// The raster level-of-detail.
			//

			const QString raster_level_of_detail_string = 
					raster_level_of_detail_combo_box->itemText(
							raster_level_of_detail_combo_box->currentIndex());
			if (raster_level_of_detail_string == QString("%1(0)").arg(QApplication::tr(HIGHEST)))
			{
				config_row.raster_level_of_detail = 0;
			}
			else
			{
				config_row.raster_level_of_detail = raster_level_of_detail_string.toUInt();
			}

			//
			// The raster fill polygons.
			//

			config_row.raster_fill_polygons = (raster_fill_polygons_check_box->checkState() == Qt::Checked);
		}

		cfg_table.push_back(config_row);
	}

	// We've finished creating/modifying the config table so optimise it (also makes it read-only).
	// We also do this so that clients can properly compare two configuration tables for equality.
	cfg_table.optimize();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::set_configuration_table_on_layer(
		const GPlatesDataMining::CoRegConfigurationTable &cfg_table)
{
	boost::shared_ptr<GPlatesPresentation::VisualLayer> layer = d_visual_layer.lock();
	GPlatesAppLogic::CoRegistrationLayerTask::Params* params =
		dynamic_cast<GPlatesAppLogic::CoRegistrationLayerTask::Params*>(
				&layer->get_reconstruct_graph_layer().get_layer_task_params());
	if(params)
	{
		params->set_cfg_table(cfg_table);
	}
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::apply(
		QAbstractButton* button)
{
	if(button_box->buttonRole(button) != QDialogButtonBox::ApplyRole)
	{
		return;
	}

	update_cfg_table();

	done(QDialog::Accepted);
}


bool
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::remove_config_rows_referencing_nonexistent_target_layer()
{
	bool removed = true;

	for (int i = 0; i < co_reg_cfg_table_widget->rowCount(); )
	{
		bool is_cfg_row_valid = false;

		LayerTableItem* co_reg_cfg_table_item = 
			dynamic_cast< LayerTableItem* > (
					co_reg_cfg_table_widget->item(i, LAYER_NAME) );

		// Make sure the items in cfg table still reference layers listed in the target layers.
		const int target_layers_list_size = target_layers_list_widget->count();
		for(int j = 0; j < target_layers_list_size; j++ )
		{
			LayerItem* target_layer_item = 
				dynamic_cast< LayerItem* > (target_layers_list_widget->item(j));
			if (target_layer_item->layer == co_reg_cfg_table_item->layer)
			{
				is_cfg_row_valid = true;
				break;
			}
		}

		if (!is_cfg_row_valid)
		{
			qDebug() << "Removing co-registration configuration row - no longer referencing a valid target layer.";
			co_reg_cfg_table_widget->removeRow(i);
			removed = false;
		}
		else
		{
			i++;
		}
	}

	return removed;
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::handle_co_registration_input_layer_list_changed(
		GPlatesAppLogic::ReconstructGraph &graph,
		GPlatesAppLogic::Layer layer)
{
	// See if the layer (whose input layer list changed) is our co-registration layer.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> co_reg_layer_ptr = d_visual_layer.lock())
	{
		if (co_reg_layer_ptr->get_reconstruct_graph_layer() != layer)
		{
			return;
		}
	}

	// Input layers, connected to our co-registration layer, were added or removed or
	// activated/deactivated so we need to refresh/update.
	//
	// NOTE: We need to update even if the dialog is not visible because a layer may have been
	// disconnected that is part of the configuration table last set on the co-registration layer.
	// The co-registration layer would then detect an invalid table and refuse to co-register.
	update(false/*update_only_when_visible*/);
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::remove()
{
	int idx = co_reg_cfg_table_widget->currentRow();
	co_reg_cfg_table_widget->removeRow(idx);
	update_cfg_table();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::remove_all()
{
	co_reg_cfg_table_widget->clearContents();
	co_reg_cfg_table_widget->setRowCount(0);
	update_cfg_table();
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::cfg_table_cell_changed(
		int row,
		int col)
{
	if(static_cast<std::size_t>(row) < d_cfg_table.size())
	{
		update_cfg_table();
	}
	return;
}


void
GPlatesQtWidgets::CoRegistrationLayerConfigurationDialog::update_cfg_table()
{
	GPlatesDataMining::CoRegConfigurationTable cfg_table;
	create_configuration_table(cfg_table);

	// If the configuration has changed then let the co-registration layer know.
	if (d_cfg_table != cfg_table)
	{
		d_cfg_table = cfg_table;

		set_configuration_table_on_layer(cfg_table);
	}

	// Force a reconstruction so that the co-registration layer uses the updated configuration.
	d_application_state.reconstruct();
}


// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the layer because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")


