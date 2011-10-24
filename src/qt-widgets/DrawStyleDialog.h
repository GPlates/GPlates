/* $Id: ColouringDialog.h 10521 2010-12-11 06:33:37Z elau $ */

/**
 * \file 
 * $Revision: 10521 $
 * $Date: 2010-12-11 17:33:37 +1100 (Sat, 11 Dec 2010) $ 
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
#include <boost/thread/mutex.hpp>
#include <boost/weak_ptr.hpp>

#include "presentation/Application.h"
#include "DrawStyleDialogUi.h"
#include "gui/PythonConfiguration.h"
#include "PythonArgumentWidget.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class ColourSchemeContainer;
	class DrawStyleManager;
	class StyleCatagory;
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

	class DrawStyleDialog  : 
			public QDialog, 
			protected Ui_DrawStyleDialog
	{
		Q_OBJECT

	public:
		DrawStyleDialog(
			GPlatesPresentation::ViewState &view_state,
			boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
			QWidget* parent_ = NULL);

		~DrawStyleDialog();
		
		void
		init_catagory_table();

		void
		init_dlg();

	protected:
		void
		make_signal_slot_connections();

		void
		set_style();

		void
		set_style(GPlatesGui::StyleAdapter* style);

		void
		load_category(const GPlatesGui::StyleCatagory& );

		void
		show_preview_icon();

		GPlatesGui::StyleCatagory*
		get_catagory(QTableWidgetItem& item)
		{
			QVariant qv = item.data(Qt::UserRole);
			return static_cast<GPlatesGui::StyleCatagory*>(qv.value<void*>());
		}

		GPlatesGui::StyleAdapter*
		get_style(QListWidgetItem* item)
		{
			QVariant qv = item->data(Qt::UserRole);
			return static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());
		}

		void
		refresh_current_icon();

#if !defined(GPLATES_NO_PYTHON)			
		QWidget *
		create_cfg_widget(GPlatesGui::PythonCfgItem* item)
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
				const GPlatesGui::StyleCatagory&,
				const QString&);


		const QString
		generate_new_valid_style_name(
				const GPlatesGui::StyleCatagory&,
				const QString&);

	private slots:
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
		handle_repaint(bool);


		void
		handle_show_thumbnails_changed(int state)
		{
			d_show_thumbnails = (state == Qt::Checked);
			QTableWidgetItem* item = categories_table->currentItem();
			if(item)
			{
				GPlatesGui::StyleCatagory* cata = get_catagory(*item);
				if(cata)
					load_category(*cata);
			}
		}

		
		void
		handle_cfg_name_changed(const QString& new_cfg_name);

		void
		handle_add_button_clicked(bool);

		void
		handle_configuration_changed()
		{
			refresh_current_icon();
		}

	private:
		static const int ICON_SIZE = 145;
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;
		QIcon d_blank_icon;
		GPlatesGui::DrawStyleManager* d_style_mgr;
		bool d_show_thumbnails;
		GlobeAndMapWidget *d_globe_and_map_widget_ptr;
		bool d_repaint_flag;
		QImage d_image;
		bool d_disable_style_item_change;
		QString d_last_open_directory;
		std::vector<QWidget*> d_cfg_widgets;
	};
}

#endif  // GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H
