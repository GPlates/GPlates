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
 
#ifndef GPLATES_QTWIDGETS_DRAWSTYLEDIALOG_H
#define GPLATES_QTWIDGETS_DRAWSTYLEDIALOG_H

#include <boost/optional.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/weak_ptr.hpp>

#include <QMutex>
#include <QMutexLocker>

#include "DrawStyleDialogUi.h"
#include "GPlatesDialog.h"
#include "PythonArgumentWidget.h"
#include "VisualLayersComboBox.h"

#include "gui/PythonConfiguration.h"

#include "presentation/Application.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class ColourSchemeContainer;
	class DrawStyleManager;
	class StyleCategory;
	class StyleAdapter;
	class Configuration;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class ReadErrorAccumulationDialog;

	class LayerGroupComboBox :
			public VisualLayersComboBox
	{
	public:
		LayerGroupComboBox(
				GPlatesPresentation::VisualLayers &visual_layers,
				GPlatesPresentation::VisualLayerRegistry &visual_layer_registry,
				const predicate_type &predicate,
				QWidget *parent_ = NULL) :
			VisualLayersComboBox(visual_layers, visual_layer_registry, predicate, parent_)
		{
			insert_all();
			setCurrentIndex(0); // Set index to the "All" entry we just inserted at beginning.
		}

		/**
		 * Override base class since need to handle 'all' layers (represented by an invalid weak_ptr).
		 */
		virtual
		void
		set_selected_visual_layer(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	protected:

		virtual
		void
		populate();

		void
		insert_all();
	};
	
	class DrawStyleDialog  : 
			public GPlatesDialog, 
			protected Ui_DrawStyleDialog
	{
		Q_OBJECT

	public:
		DrawStyleDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget* parent_ = NULL);

		~DrawStyleDialog();

		/**
		 * Similar to the other overload of @a reset, but uses the layer (or none) currently set by
		 * the most recent call to that @a reset.
		 *
		 * This is used after this dialog is popped up, but not from within a specific layer.
		 */
		void
		reset();

		/**
		 * Set up this dialog (after it has been popped up) for a specific layer, or 'all' layers
		 * if @a layer is invalid.
		 */
		void
		reset(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> layer,
				//
				// FIXME: DrawStyleDialog should update its GUI when the draw style changes in visual layer params.
				//
				// Currently DrawStyleDialog clobbers the draw style in the visual layer params.
				// DrawStyleDialog should just be one observer of visual layer params
				// (ie, it is not the only one who can change its state).
				//
				// As a temporary hack to get around this we allow another observer to set the draw style via
				// DrawStyleDialog using the following style parameter (this also sets it in the layer's visual params).
				// This means that when DrawStyleDialog is popped up by the user it will reset the draw style
				// (to the state that is stored in its GUI) but that state will be up-to-date (ie, not old state).
				//
				boost::optional<const GPlatesGui::StyleAdapter *> style_ = boost::none);

	protected:

		virtual
		void
		showEvent(
				QShowEvent *show_event);
		
		void
		init_category_table();

		void
		init_dlg();

		void
		make_signal_slot_connections();

		void
		set_style();

		void
		set_style(
				GPlatesGui::StyleAdapter* style);

		void
		load_category(
				const GPlatesGui::StyleCategory& );

		void
		show_preview_icons();

		GPlatesGui::StyleCategory*
		get_catagory(
				QTableWidgetItem& item)
		{
			QVariant qv = item.data(Qt::UserRole);
			return static_cast<GPlatesGui::StyleCategory*>(qv.value<void*>());
		}

		GPlatesGui::StyleAdapter*
		get_style(
				QListWidgetItem* item)
		{
			QVariant qv = item->data(Qt::UserRole);
			return static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
		}

		void
		refresh_current_icon();

#if !defined(GPLATES_NO_PYTHON)			
		QWidget *
		create_cfg_widget(
				GPlatesGui::PythonCfgItem* item)
		{
			//this function is temporary.
			if(dynamic_cast<GPlatesGui::PythonCfgColor*>(item) != 0)
			{
				return new PythonArgColorWidget(item,this);
			}

			if(dynamic_cast<GPlatesGui::PythonCfgPalette*>(item) != 0)
			{
				return new PythonArgPaletteWidget(item,this);
			}
		
			return new PythonArgDefaultWidget(item,this);
		}
#endif

		void
		build_config_panel(const GPlatesGui::Configuration& cfg);


		void
		enable_config_panel(bool flag)
		{
			scrollArea->setEnabled(flag);
		}


		GPlatesGui::StyleAdapter*
		get_current_style()
		{
			QListWidgetItem * item = style_list->currentItem();
			if(item)
				return get_style(item);
			else
				return NULL;
		}


		bool
		is_style_name_valid(
				const GPlatesGui::StyleCategory&,
				const QString&);


		const QString
		generate_new_valid_style_name(
				const GPlatesGui::StyleCategory&,
				const QString&);

		void
		focus_style(
				const GPlatesGui::StyleAdapter*);

		void
		apply_style_to_all_layers();


	private Q_SLOTS:
		void
		handle_close_button_clicked();

		void
		handle_remove_button_clicked();
		
		void
		handle_categories_table_cell_changed(
				int current_row,
				int current_column,
				int previous_row,
				int previous_column);

		void
		handle_style_selection_changed(
				QListWidgetItem* current,
				QListWidgetItem* previous);

		void
		handle_main_repaint(
				bool);

		void
		handle_show_thumbnails_changed(
				int state)
		{
			d_show_thumbnails = (state == Qt::Checked);
			QTableWidgetItem* item = categories_table->currentItem();
			if(item)
			{
				GPlatesGui::StyleCategory* cata = get_catagory(*item);
				if(cata)
				{
					load_category(*cata);
				}
			}
		}

		
		void
		handle_cfg_name_changed(
				const QString& new_cfg_name);

		void
		handle_add_button_clicked(
				bool);

		void
		handle_configuration_changed();

		
		void
		handle_layer_changed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer>);
	
	private:

		class PreviewGuard
		{
		public:
			PreviewGuard(
					DrawStyleDialog &draw_style_dialog);

			~PreviewGuard();

		private:
			DrawStyleDialog &d_draw_style_dialog;
			int d_current_idx;
		};


		static const int ICON_SIZE = 145;


		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;
		QIcon d_blank_icon;
		GPlatesGui::DrawStyleManager* d_style_mgr;
		bool d_show_thumbnails;
		bool d_ignore_next_main_repaint;
		GlobeAndMapWidget *d_globe_and_map_widget_ptr;
		QString d_last_open_directory;
		std::vector<QWidget*> d_cfg_widgets;
		GPlatesPresentation::ViewState& d_view_state;
		LayerGroupComboBox* d_combo_box;
		const GPlatesGui::StyleAdapter* d_style_of_all;
	};
}


#endif  // GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H

