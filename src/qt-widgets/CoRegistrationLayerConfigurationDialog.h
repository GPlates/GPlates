/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QT_WIDGETS_COREGISTRATIONLAYERCONFIGURATIONDIALOG_H
#define GPLATES_QT_WIDGETS_COREGISTRATIONLAYERCONFIGURATIONDIALOG_H

#include <boost/weak_ptr.hpp>

#include <QListView>
#include <QCheckBox>
#include <QComboBox>

#include "CoRegistrationLayerConfigurationDialogUi.h"

#include "OpenDirectoryDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "data-mining/CheckAttrTypeVisitor.h"
#include "data-mining/CoRegConfigurationTable.h"

#include "global/PointerTraits.h"

#include "model/FeatureStoreRootHandle.h"
#include "model/WeakReferenceCallback.h"

#include "presentation/ViewState.h"


#include <boost/foreach.hpp>
namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLViewport;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;

	/**
	 * The configuration dialog for Co-registration layer.
	 */
	class CoRegistrationLayerConfigurationDialog :
			public QDialog, 
			protected Ui_CoRegistrationLayerConfigurationDialog
	{
		Q_OBJECT

	public:

		enum ConfigurationTableColumnType
		{
			LAYER_NAME = 0,
			FILTER_TYPE,
			ATTRIBUTE_NAME,
			RANGE,
			REDUCER,
			RASTER_LEVEL_OF_DETAIL,
			RASTER_FILL_POLYGONS,

			NUM_COLUMNS // This must be last
		};

		/**
		 * A QListWidgetItem derivation so that we can display a list of layers in
		 * the list widget using the layer name as the label, while keeping track of which
		 * list item corresponds to which layer.
		 */
		struct LayerItem :
				public QListWidgetItem
		{
			LayerItem(
					const GPlatesAppLogic::Layer& layer_, 
					const QString &layer_name_):
				QListWidgetItem(layer_name_),
				layer(layer_),
				label(layer_name_)
				{ }

			const GPlatesAppLogic::Layer layer;
			const QString label;
		};

		struct AttributeItem
		{
			explicit
			AttributeItem(
					const GPlatesDataMining::AttributeType attr_type_) :  
				attr_type(attr_type_)
			{ }

			const GPlatesDataMining::AttributeType attr_type;
		};

		struct AttributeListItem :
			public QListWidgetItem,
			public AttributeItem
		{
			AttributeListItem(
					const QString &attr_name_,
					const GPlatesDataMining::AttributeType attr_type_) :  
				QListWidgetItem(attr_name_),
				AttributeItem(attr_type_)
			{ }
		};


		struct LayerTableItem :
				public QTableWidgetItem
		{
			LayerTableItem(
					const GPlatesAppLogic::Layer& layer_, 
					const QString &layer_name_) :
				QTableWidgetItem(layer_name_),
				layer(layer_),
				label(layer_name_)
				{ }

			const GPlatesAppLogic::Layer layer;
			const QString label;
		};

		struct AttributeTableItem :
			public QTableWidgetItem,
			public AttributeItem
		{
			AttributeTableItem(
					const QString &attr_name_,
					const GPlatesDataMining::AttributeType attr_type_) :  
				QTableWidgetItem(attr_name_),
				AttributeItem(attr_type_)
			{ }
		};


		CoRegistrationLayerConfigurationDialog(
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				boost::weak_ptr<GPlatesPresentation::VisualLayer> layer);

		void
		pop_up();

		void
		set_visual_layer(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> layer)
		{
			d_visual_layer = layer;
		}

	public slots:

		void
		reject()
		{
			done(QDialog::Rejected);
		}
		
		void
		apply(
				QAbstractButton*);

		/**
		 * Updates GUI and co-registration configuration.
		 *
		 * This is automatically called internally whenever any layers or connections are modified.
		 */
		void
		update(
				bool update_only_when_visible = true);

	private slots:

		void
		react_target_layer_selection_changed();

		void
		react_add_configuration_row();

		void
		populate_attributes();

		void
		populate_relational_attributes();

		void
		populate_coregistration_attributes();

		/**
		 * Is called whenever an input layer to our co-registration layer has been added or removed.
		 */
		void
		handle_co_registration_input_layer_list_changed(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer);

		void
		remove();

		void
		remove_all();

	private:

		/**
		 * The model callback that notifies us when the feature store is modified so that
		 * we can do a reconstruction.
		 */
		struct UpdateWhenFeatureStoreIsModified :
				public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureStoreRootHandle>
		{
			explicit
			UpdateWhenFeatureStoreIsModified(
					CoRegistrationLayerConfigurationDialog &co_reg_layer_cfg_dialog) :
				d_co_reg_layer_cfg_dialog(&co_reg_layer_cfg_dialog)
			{  }

			void
			publisher_modified(
					const modified_event_type &event)
			{
				// Update the GUI (mainly the attribute list) every time the model
				// (feature store) is modified.
				d_co_reg_layer_cfg_dialog->update();
			}

			CoRegistrationLayerConfigurationDialog *d_co_reg_layer_cfg_dialog;
		};

		bool
		is_raster_co_registration_supported() const;

		void
		populate_target_layers_list() const;

		void
		populate_reconstructed_geometries_coregistration_attributes(
				const GPlatesAppLogic::Layer &target_layer);

		void
		populate_raster_coregistration_attributes(
				const GPlatesAppLogic::Layer &target_layer);

		bool
		does_raster_layer_contain_numerical_data(
				const GPlatesAppLogic::Layer &raster_layer) const;

		/**
		 * Creates an OpenGL renderer so we can query raster-related information.
		 */
		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLRenderer>::non_null_ptr_type
		create_gl_renderer(
				GPlatesOpenGL::GLViewport &viewport) const;

		void
		get_unique_attribute_names(
				const GPlatesAppLogic::Layer &target_layer,
				std::set< GPlatesModel::PropertyName > &property_names,
				std::set< QString > &shapefile_attr_names);
			
		void
		setup_reducer_combobox(
				const QString& attribute_name,	
				QComboBox* combo,
				const GPlatesAppLogic::LayerTaskType::Type target_layer_type);

		void
		setup_association_type_combobox(
				QComboBox* combo);

		bool
		setup_raster_level_of_detail_combo_box(
				QComboBox* combo,
				const GPlatesAppLogic::Layer &raster_target_layer,
				const QString &raster_band_name);

		void
		setup_raster_fill_polygons_check_box(
				QCheckBox* check_box);

		bool
		remove_config_rows_referencing_nonexistent_target_layer();
	
		std::vector<GPlatesAppLogic::Layer>
		get_input_target_layers() const;

		std::vector<GPlatesAppLogic::Layer>
		get_input_seed_layers() const;

		std::vector<GPlatesAppLogic::Layer>
		get_input_layers(
				const QString &channel_name,
				bool target_layers) const;

		void
		create_configuration_table(
				GPlatesDataMining::CoRegConfigurationTable &cfg_table);

		void
		set_configuration_table_on_layer(
				const GPlatesDataMining::CoRegConfigurationTable &cfg_table);


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		std::multimap< QString, GPlatesDataMining::AttributeTypeEnum >  d_attr_name_type_map;

		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;

		//! The current configuration table.
		GPlatesDataMining::CoRegConfigurationTable d_cfg_table;

		/**
		 * Keep a weak reference to the feature store root handle just for our callback.
		 *
		 * Only we have access to this weak ref and we make sure the client doesn't have
		 * access to it. This is because any copies of this weak reference also get
		 * copies of the callback thus allowing it to get called more than once per modification.
		 */
		GPlatesModel::FeatureStoreRootHandle::const_weak_ref d_callback_feature_store;

		/**
		  * Is raster co-registration supported (are the necessary OpenGL extensions available).
		 */
		bool d_raster_co_registration_supported;
	};
}

#endif // GPLATES_QT_WIDGETS_COREGISTRATIONLAYERCONFIGURATIONDIALOG_H
