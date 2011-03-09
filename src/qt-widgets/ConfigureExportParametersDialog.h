/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#include <set>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ConfigureExportParametersDialogUi.h"

#include "gui/ExportAnimationContext.h"
#include "gui/ExportAnimationType.h"


namespace GPlatesQtWidgets
{
	class ExportOptionsWidget;

	class ConfigureExportParametersDialog : 
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
	
		void
		initialise(
				QTableWidget*);

		void
		set_single_frame(
				bool is_single_frame)
		{
			d_is_single_frame = is_single_frame;
		}

		void
		add_all_remaining_exports();


		/**
		 * A widget item to store the state of an ExportAnimationType::Type.
		 */
		template <class WidgetItemType>
		class ExportTypeWidgetItem :
			public WidgetItemType
		{
		public:
			explicit
			ExportTypeWidgetItem(
					GPlatesGui::ExportAnimationType::Type type):
			d_type(type)
			{  }

			GPlatesGui::ExportAnimationType::Type
			type() const
			{
				return d_type;
			}

		private:
			GPlatesGui::ExportAnimationType::Type d_type;
		};

		template <class WidgetItemType>
		static
		GPlatesGui::ExportAnimationType::Type
		get_export_type(
				WidgetItemType* widget_item)
		{
			if (ExportTypeWidgetItem<WidgetItemType>* item =
				dynamic_cast<ExportTypeWidgetItem<WidgetItemType>*> (widget_item))
			{
				return item->type();
			}
			else
			{
				//This is very unlikely to happen. If it did happen, it is not necessary to abort the application since this is not a fatal error.
				//Record the error here. The further improvement could be throwing exception and make sure the exception is handled properly.
				qWarning()
					<<"Unexpected pointer type found in ConfigureExportParametersDialog::get_export_type()";
				return GPlatesGui::ExportAnimationType::INVALID_TYPE;
			}
		}


		/**
		 * A widget item to store the state of an ExportAnimationType::Format.
		 */
		template <class WidgetItemType>
		class ExportFormatWidgetItem :
			public WidgetItemType
		{
		public:
			explicit
			ExportFormatWidgetItem(
					GPlatesGui::ExportAnimationType::Format format):
			d_format(format)
			{  }

			GPlatesGui::ExportAnimationType::Format
			format() const
			{
				return d_format;
			}

		private:
			GPlatesGui::ExportAnimationType::Format d_format;
		};

		template <class WidgetItemType>
		static
		GPlatesGui::ExportAnimationType::Format
		get_export_format(
				WidgetItemType* widget_item)
		{
			if (ExportFormatWidgetItem<WidgetItemType>* item =
				dynamic_cast<ExportFormatWidgetItem<WidgetItemType>*> (widget_item))
			{
				return item->format();
			}
			else
			{
				//This is very unlikely to happen. If it did happen, it is not necessary to abort the application since this is not a fatal error.
				//Record the error here. The further improvement could be throwing exception and make sure the exception is handled properly.
				qWarning()
					<<"Unexpected pointer type found in ConfigureExportParametersDialog::get_export_format()";
				return GPlatesGui::ExportAnimationType::INVALID_FORMAT;
			}
		}


		/**
		 * A widget item to store the state of an ExportAnimationStrategy::const_configuration_base_ptr.
		 */
		template <class WidgetItemType>
		class ExportConfigurationWidgetItem :
			public WidgetItemType
		{
		public:
			explicit
			ExportConfigurationWidgetItem(
					const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &configuration) :
				d_configuration(configuration)
			{  }

			const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &
			configuration()
			{
				return d_configuration;
			}

		private:
			GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr d_configuration;
		};

		template <class WidgetItemType>
		static
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		get_export_configuration(
				WidgetItemType* widget_item)
		{
			if (ExportConfigurationWidgetItem<WidgetItemType>* item =
				dynamic_cast<ExportConfigurationWidgetItem<WidgetItemType>*> (widget_item))
			{
				return item->configuration();
			}
			else
			{
				//This is very unlikely to happen. If it did happen, it is not necessary to abort the application since this is not a fatal error.
				//Record the error here. The further improvement could be throwing exception and make sure the exception is handled properly.
				qWarning()
					<<"Unexpected pointer type found in ConfigureExportParametersDialog::get_export_configuration()";
				return GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr();
			}
		}

	private slots:
		void
		react_add_item_clicked();

		void
		react_export_type_selection_changed();

		void
		react_export_format_selection_changed();

		void
		react_filename_template_changed();
		
		void
		react_filename_template_changing();

		void
		focus_on_listwidget_format();

		void
		focus_on_lineedit_filename();
	
	private:
		//! Typedef for a set of exporters - identified by their export IDs.
		typedef std::set<GPlatesGui::ExportAnimationType::ExportID> exporter_set_type;


		/**
		 * The ExportAnimationContext is the Context role of the Strategy pattern
		 * in Gamma et al p315.
		 *
		 * It keeps all the actual export parameters.
		 */
		GPlatesGui::ExportAnimationContext::non_null_ptr_type d_export_animation_context_ptr;

		//! The currently added exporters - added by the user.
		exporter_set_type d_exporters_added;

		bool d_is_single_frame;

		/**
		 * The current widget, if any, used to select export options.
		 *
		 * This is created after the export type and format have been selected.
		 */
		boost::optional<ExportOptionsWidget *> d_current_export_options_widget;

		/**
		 * The layout for the export options widget.
		 */
		QVBoxLayout *d_export_options_widget_layout;


		void
		initialize_export_type_list_widget();

		void
		clear_export_options_widget();

		void
		set_export_options_widget(
				GPlatesGui::ExportAnimationType::ExportID export_id);
	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGUREEXPORTPARAMETERSDIALOG_H
