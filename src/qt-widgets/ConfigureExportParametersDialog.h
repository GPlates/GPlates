/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H
#define GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QTableWidget>
#include <QString>
#include "ConfigureExportParametersDialogUi.h"

#include "gui/ExportAnimationContext.h"
#include "utils/ExportFileNameTemplateValidator.h"
#include "utils/ExportAnimationStrategyExporterID.h"

namespace GPlatesQtWidgets
{
	class ConfigureExportParametersDialog: 
			public QDialog,
			protected Ui_ConfigureExportParametersDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		ConfigureExportParametersDialog(
				GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
				QWidget *parent_ = NULL);

		virtual
		~ConfigureExportParametersDialog()
		{ }
	
		friend class ExportAnimationDialog;

		typedef struct {
			bool has_been_added;
			GPlatesUtils::Exporter_ID class_id;
		} export_item_info;

		enum ExportItemName
		{
			RECONSTRUCTED_GEOMETRIES,
			PROJECTED_GEOMETRIES,
			MESH_VELOCITIES,
			RESOLVED_TOPOLOGIES,
			RELATIVE_ROTATION,
			EQUIVALENT_ROTATION,
			ROTATION_PARAMS,
			RASTER,
			INVALID_NAME=999
		};

		enum ExportItemType
		{
			GMT,
			SHAPEFILE,
			SVG,
			GPML,
			CSV_COMMA,
			CSV_SEMICOLON,
			CSV_TAB,
			BMP,
			JPG,
			JPEG,
			PNG,
			PPM,
			TIFF,
			XBM,
			XPM,
			INVALID_TYPE=999
		};

		typedef std::map<ExportItemType, export_item_info> export_type_map_type;
		typedef std::map<ExportItemName, export_type_map_type > export_item_map_type;

		void
		init(
				ExportAnimationDialog*,
				QTableWidget*);

	private slots:
		void
		react_add_item_clicked();

		void
		react_export_items_selection_changed();

		void
		react_format_selection_changed();

		void
		react_filename_template_changed();
		
		void
		react_filename_template_changing();
	
	private:
		void
		initialize_export_item_map();

		void
		initialize_export_item_list_widget();
		
		static 
		bool
		initialize_item_name_and_type_map();

		static 
		void
		initialize_item_desc_map();

		bool
		all_types_has_been_added(
				export_type_map_type type_map);
		
		inline
		ExportItemName 
		get_export_item_name(
				QListWidgetItem* widget_item)
		{
			if(ExportItem* item = dynamic_cast<ExportItem*> (widget_item))
			{
				return item->itemname_id();
			}
			else
			{
				qWarning()
					<<"Unexpected pointer type found in GPlatesQtWidgets::"
					"ConfigureExportParametersDialog::react_format_selection_changed()";
				return INVALID_NAME;
			}
		};

		inline
		ExportItemType
		get_export_item_type(
				QListWidgetItem* widget_item)
		{
			if(ExportTypeItem* item = dynamic_cast<ExportTypeItem*> (widget_item))
			{
				return item->itemtype_id();
			}
			else
			{
				qWarning()
					<<"Unexpected pointer type found in GPlatesQtWidgets::"
					"ConfigureExportParametersDialog::react_format_selection_changed()";
				return INVALID_TYPE;
			}
		}

		/**
		 * The ExportAnimationContext is the Context role of the Strategy pattern
		 * in Gamma et al p315.
		 *
		 * It keeps all the actual export parameters.
		 */
		GPlatesGui::ExportAnimationContext::non_null_ptr_type d_export_animation_context_ptr;

		static std::map<ExportItemName, QString> d_name_map;
		static std::map<ExportItemType, QString> d_type_map;
		static std::map<ExportItemName, QString> d_desc_map;

		class ExportItem :
			public QListWidgetItem
		{
		public:
			ExportItem(
					ExportItemName item_id):
				d_id(item_id)
			{ }

			ExportItemName
					itemname_id()
			{
				return d_id;
			}

		private:
			ExportItemName d_id;
		};

		class ExportTypeItem :
			public QListWidgetItem
		{
		public:
			ExportTypeItem(
					ExportItemType type_id):
				d_id(type_id)
			{ }

			ExportItemType
					itemtype_id()
			{
				return d_id;
			}

		private:
			ExportItemType d_id;
		};

		export_item_map_type d_export_item_map;
		QString d_filename_template;

		bool d_is_single_frame;
		static bool dummy;


	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H
