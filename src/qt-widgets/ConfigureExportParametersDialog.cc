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

#include <map>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QColorGroup>
#include <QMessageBox>
#include <QHeaderView>

#include "ConfigureExportParametersDialog.h"
#include "ExportAnimationDialog.h"
#include "ExportOptionsWidget.h"

#include "gui/ExportAnimationRegistry.h"
#include "gui/ExportAnimationType.h"

#include "presentation/ViewState.h"


namespace
{
	void
	set_fixed_size_for_item_view(
			QAbstractItemView *view)
	{
		int num_rows = view->model()->rowCount();
		if (num_rows > 0)
		{
			view->setFixedHeight(view->sizeHintForRow(0) * (num_rows + 1));
		}
	}
}


GPlatesQtWidgets::ConfigureExportParametersDialog::ConfigureExportParametersDialog(
		GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
		QWidget *parent_):
	QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint),
	d_export_animation_context_ptr(
			export_animation_context_ptr),
	d_is_single_frame(false),
	d_export_options_widget_layout(NULL)
{
	setupUi(this);
	set_fixed_size_for_item_view(treeWidget_template);
	treeWidget_template->setHeaderHidden(true);
	treeWidget_template->header()->setResizeMode(0, QHeaderView::ResizeToContents);

	// Give the export options widget a layout.
	d_export_options_widget_layout = new QVBoxLayout(widget_export_options);
	d_export_options_widget_layout->setContentsMargins(0, 0, 0, 0);

	initialize_export_type_list_widget();

	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(false);

	QObject::connect(
			listWidget_export_items,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(react_export_type_selection_changed()));
	QObject::connect(
			listWidget_export_items,
			SIGNAL(itemClicked(QListWidgetItem *)),
			this,
			SLOT(react_export_type_selection_changed()));
	QObject::connect(
			listWidget_format,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(react_export_format_selection_changed()));
	QObject::connect(
			lineEdit_filename,
			SIGNAL(cursorPositionChanged(int, int)),
			this,
			SLOT(react_filename_template_changing()));
	QObject::connect(
			lineEdit_filename,
			SIGNAL(editingFinished()),
			this,
			SLOT(react_filename_template_changed()));

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(react_add_item_clicked()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	// Help the user move focus around the dialog.
	QObject::connect(
			listWidget_export_items,
			SIGNAL(itemPressed(QListWidgetItem *)),
			this,
			SLOT(focus_on_listwidget_format()));
	QObject::connect(
			listWidget_format,
			SIGNAL(itemPressed(QListWidgetItem *)),
			this,
			SLOT(focus_on_lineedit_filename()));
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_export_type_list_widget()
{
	listWidget_export_items->clear();
	listWidget_format->clear();
	clear_export_options_widget();

	//
	// Add a widget item for each export type that has not had all its export formats added already.
	//

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Get a list of all the currently supported exporters.
	const std::vector<GPlatesGui::ExportAnimationType::ExportID> supported_exporters =
			export_animation_registry.get_registered_exporters();

	// Get the supported export types.
	const std::vector<GPlatesGui::ExportAnimationType::Type> supported_export_types =
			GPlatesGui::ExportAnimationType::get_export_types(supported_exporters);

	// Iterate over the export types.
	std::vector<GPlatesGui::ExportAnimationType::Type>::const_iterator export_type_iter;
	for (export_type_iter = supported_export_types.begin();
		export_type_iter != supported_export_types.end();
		++export_type_iter)
	{
		const GPlatesGui::ExportAnimationType::Type supported_export_type = *export_type_iter;

		// Get the supported export formats for the current export type.
		const std::vector<GPlatesGui::ExportAnimationType::Format> supported_export_formats =
				get_export_formats(supported_exporters, supported_export_type);

		bool all_export_formats_already_added = true;

		// Iterate over the export formats of the current export type.
		std::vector<GPlatesGui::ExportAnimationType::Format>::const_iterator export_format_iter;
		for (export_format_iter = supported_export_formats.begin();
			export_format_iter != supported_export_formats.end();
			++export_format_iter)
		{
			const GPlatesGui::ExportAnimationType::Format supported_export_format = *export_format_iter;

			const GPlatesGui::ExportAnimationType::ExportID export_id =
					get_export_id(supported_export_type, supported_export_format);

			// See if the current export type and format have already been added by the user.
			if (d_exporters_added.find(export_id) == d_exporters_added.end())
			{
				// We didn't find the current export format in the list of added exporters.
				all_export_formats_already_added = false;
				break;
			}
		}

		// If not all export formats (for the current export type) have already been added
		// then add a widget item for the current export type.
		if (!all_export_formats_already_added)
		{
			QListWidgetItem *widget_item = new ExportTypeWidgetItem<QListWidgetItem>(supported_export_type);
			listWidget_export_items->addItem(widget_item);
			widget_item->setText(get_export_type_name(supported_export_type));
		}
	}
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_type_selection_changed()
{
	if(!listWidget_export_items->currentItem())
	{
		return;
	}

	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(false);

	lineEdit_filename->clear();
	label_file_extension->clear();
	listWidget_format->clear();
	clear_export_options_widget();

	const GPlatesGui::ExportAnimationType::Type selected_export_type =
			get_export_type(listWidget_export_items->currentItem());

	label_export_description->setText(get_export_type_description(selected_export_type));

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Get a list of all the currently supported exporters.
	const std::vector<GPlatesGui::ExportAnimationType::ExportID> supported_exporters =
			export_animation_registry.get_registered_exporters();

	// Of those, narrow down to exporters having the specified export type and
	// return a list of their export formats.
	const std::vector<GPlatesGui::ExportAnimationType::Format> supported_export_formats =
			get_export_formats(supported_exporters, selected_export_type);

	// Iterate through the supported export formats.
	std::vector<GPlatesGui::ExportAnimationType::Format>::const_iterator export_format_iter;
	for (export_format_iter = supported_export_formats.begin();
		export_format_iter != supported_export_formats.end();
		++export_format_iter)
	{
		const GPlatesGui::ExportAnimationType::Format export_format = *export_format_iter;

		const GPlatesGui::ExportAnimationType::ExportID export_id =
				get_export_id(selected_export_type, export_format);

		// If we've already added the exporter then continue to the next export format.
		if (d_exporters_added.find(export_id) != d_exporters_added.end())
		{
			continue;
		}

		QListWidgetItem *item = new ExportFormatWidgetItem<QListWidgetItem>(export_format);
		listWidget_format->addItem(item);
		item->setText(get_export_format_description(export_format));
	}
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_format_selection_changed()
{
	if (!listWidget_export_items->currentItem() ||
		!listWidget_format->currentItem())
	{
		return;
	}

	const GPlatesGui::ExportAnimationType::Type selected_export_type =
			get_export_type(listWidget_export_items->currentItem());
	const GPlatesGui::ExportAnimationType::Format selected_export_format =
			get_export_format(listWidget_format->currentItem());
	
	if (selected_export_type == GPlatesGui::ExportAnimationType::INVALID_TYPE ||
		selected_export_format == GPlatesGui::ExportAnimationType::INVALID_FORMAT)
	{	
		qWarning()<<"invalid export type or format!";
		return;
	}

	const GPlatesGui::ExportAnimationType::ExportID selected_export_id =
			get_export_id(selected_export_type, selected_export_format);

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	//
	// Display the filename template.
	//

	const QString &default_filename_template =
			export_animation_registry.get_default_filename_template(selected_export_id);
	
	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(true);

	lineEdit_filename->setText(
			default_filename_template.toStdString().substr(
					0, default_filename_template.toStdString().find_last_of(".")).c_str());
	
	label_file_extension->setText(
			default_filename_template.toStdString().substr(
					default_filename_template.toStdString().find_last_of(".")).c_str());

	//
	// Display any export options for the selected format (if there are any).
	//

	set_export_options_widget(selected_export_id);

#if 0
	const QString &filename_template_description =
			export_animation_registry.get_filename_template_description(selected_export_id);

	label_filename_desc->setText(filename_template_description);
	QPalette pal=label_filename_desc->palette();
	pal.setColor(QPalette::WindowText, QColor("black")); 
	label_filename_desc->setPalette(pal);
#endif
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_add_item_clicked()
{
	if(!listWidget_export_items->currentItem() ||
		!listWidget_format->currentItem())
	{
		return;
	}
	
	QString filename_template = lineEdit_filename->text() + label_file_extension->text();		

	// Get the currently selected export type and format.
	const GPlatesGui::ExportAnimationType::Type selected_export_type = get_export_type(
			listWidget_export_items->currentItem());
	const GPlatesGui::ExportAnimationType::Format selected_export_format = get_export_format(
			listWidget_format->currentItem());

	// Determine the corresponding export ID.
	const GPlatesGui::ExportAnimationType::ExportID selected_export_id =
			get_export_id(selected_export_type, selected_export_format);

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Validate the filename template against the selected exporter.
	QString filename_template_validation_message;
	if (!export_animation_registry.validate_filename_template(
			selected_export_id, filename_template, filename_template_validation_message))
	{
		QMessageBox error_popup;
		error_popup.setWindowTitle(QString("Cannot Add Data to Export"));
		error_popup.setText(QString("The filename template contains an invalid format string."));
		error_popup.setInformativeText(filename_template_validation_message);
		error_popup.setIcon(QMessageBox::Warning);
		error_popup.exec();
#if 0
		label_filename_desc->setText(filename_template_validation_message);
		QPalette pal=label_filename_desc->palette();
		pal.setColor(QPalette::WindowText, QColor("red")); 
		label_filename_desc->setPalette(pal);
#endif
		main_buttonbox->setEnabled(false);
		return;
	}

	// If we have an export options widget then get it to create the export animation configuration.
	// Otherwise just create the default configuration.
	GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr export_cfg;
	if (d_current_export_options_widget)
	{
		export_cfg = d_current_export_options_widget.get()->
					create_export_animation_strategy_configuration(filename_template);
	}
	else
	{
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr const_default_export_cfg =
				export_animation_registry.get_default_export_configuration(selected_export_id);
		if (!const_default_export_cfg)
		{
			// Something is not right - shouldn't be able to get here.
			// Should probably assert so programmer can fix bug.
			// But will just return without adding exporter.
			qWarning() << "Encountered NULL export configuration - ignoring selected exporter.";
			return;
		}

		const GPlatesGui::ExportAnimationStrategy::configuration_base_ptr default_export_cfg =
				const_default_export_cfg->clone();
		default_export_cfg->set_filename_template(filename_template);
		export_cfg = default_export_cfg;
	}

	delete listWidget_format->takeItem(listWidget_format->currentRow());

	if (listWidget_format->count() == 0)
	{
		delete listWidget_export_items->takeItem(listWidget_export_items->currentRow());
	}

	clear_export_options_widget();

	// Add to the list of exporters we've added so far.
	d_exporters_added.insert(selected_export_id);

	// Add the selected exporter to the export animation dialog.
	d_export_animation_context_ptr->get_export_dialog()->insert_item(
			selected_export_type,
			selected_export_format,
			export_cfg);

	accept();
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::initialise(
		QTableWidget* table)
{
	d_exporters_added.clear();
	
	for(int i=0; i<table->rowCount();i++)
	{
		const GPlatesGui::ExportAnimationType::Type selected_export_type =
				get_export_type(table->item(i,0));
		const GPlatesGui::ExportAnimationType::Format selected_export_format =
				get_export_format(table->item(i,1));

		const GPlatesGui::ExportAnimationType::ExportID selected_export_id =
				get_export_id(selected_export_type, selected_export_format);

		// Mark the exporter as having been added.
		d_exporters_added.insert(selected_export_id);
	}

	initialize_export_type_list_widget();

	lineEdit_filename->clear();
	label_file_extension->clear();
	label_export_description->clear();
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_filename_template_changed()
{
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_filename_template_changing()
{
	if (!listWidget_export_items->currentItem() ||
		!listWidget_format->currentItem())
	{
		return;
	}

	const GPlatesGui::ExportAnimationType::Type selected_export_type = get_export_type(
			listWidget_export_items->currentItem());
	const GPlatesGui::ExportAnimationType::Format selected_export_format = get_export_format(
			listWidget_format->currentItem());

	if (selected_export_type == GPlatesGui::ExportAnimationType::INVALID_TYPE ||
		selected_export_format == GPlatesGui::ExportAnimationType::INVALID_FORMAT)
	{
		return;
	}
	
	const GPlatesGui::ExportAnimationType::ExportID selected_export_id =
			get_export_id(selected_export_type, selected_export_format);

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Get a list of all the currently supported exporters.
	const std::vector<GPlatesGui::ExportAnimationType::ExportID> supported_exporters =
			export_animation_registry.get_registered_exporters();

	// If the selected export type and format are not in the list of supported exporters
	// then something is not right.
	if (std::find(supported_exporters.begin(), supported_exporters.end(), selected_export_id) ==
		supported_exporters.end())
	{
		qWarning()<<"invalid selected items!";
		return;
	}

#if 0
	label_filename_desc->setText(...);
	QPalette pal=label_filename_desc->palette();
	pal.setColor(QPalette::WindowText, QColor("black")); 
	label_filename_desc->setPalette(pal);
#endif
	main_buttonbox->setEnabled(true);
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::focus_on_listwidget_format()
{
	listWidget_format->setFocus();
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::focus_on_lineedit_filename()
{
	lineEdit_filename->setFocus();
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::clear_export_options_widget()
{
	if (d_current_export_options_widget)
	{
		d_export_options_widget_layout->removeWidget(d_current_export_options_widget.get());
		delete d_current_export_options_widget.get();

		d_current_export_options_widget = boost::none;
	}

	widget_export_options->setEnabled(false);
	widget_export_options->setVisible(false);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::set_export_options_widget(
		GPlatesGui::ExportAnimationType::ExportID export_id)
{
	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	if (d_current_export_options_widget)
	{
		d_export_options_widget_layout->removeWidget(d_current_export_options_widget.get());
		delete d_current_export_options_widget.get();
	}

	d_current_export_options_widget = export_animation_registry.create_export_options_widget(
			export_id, this);

	if (d_current_export_options_widget)
	{
		d_current_export_options_widget.get()->layout()->setContentsMargins(0, 0, 0, 0);
		d_export_options_widget_layout->addWidget(d_current_export_options_widget.get());

		widget_export_options->setEnabled(true);
		widget_export_options->setVisible(true);
	}
	else
	{
		widget_export_options->setEnabled(false);
		widget_export_options->setVisible(false);
	}
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::add_all_remaining_exports()
{
	//
	// Insert an exporter into the export animation dialog for each supported
	// export type and format not yet added.
	//

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Get a list of all the currently supported exporters.
	const std::vector<GPlatesGui::ExportAnimationType::ExportID> supported_export_ids =
			export_animation_registry.get_registered_exporters();

	// Iterate over the export ids.
	std::vector<GPlatesGui::ExportAnimationType::ExportID>::const_iterator export_id_iter;
	for (export_id_iter = supported_export_ids.begin();
		export_id_iter != supported_export_ids.end();
		++export_id_iter)
	{
		const GPlatesGui::ExportAnimationType::ExportID supported_export_id = *export_id_iter;

		// See if the current export id has already been added by the user.
		if (d_exporters_added.find(supported_export_id) != d_exporters_added.end())
		{
			continue;
		}

		// We didn't find the current export id so add it.

		const GPlatesGui::ExportAnimationType::Type supported_export_type =
				GPlatesGui::ExportAnimationType::get_export_type(supported_export_id);
		const GPlatesGui::ExportAnimationType::Format supported_export_format =
				GPlatesGui::ExportAnimationType::get_export_format(supported_export_id);

		// Create the default export configuration for the current exporter.
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr export_configuration =
				export_animation_registry.get_default_export_configuration(
						supported_export_id);

		if (!export_configuration)
		{
			// Something is not right - shouldn't be able to get here.
			// Should probably assert so programmer can fix bug.
			// But will just continue without adding exporter.
			qWarning() << "Encountered NULL export configuration - ignoring exporter.";
			continue;
		}

		// Insert a new export item in the export animation dialog.
		d_export_animation_context_ptr->get_export_dialog()->insert_item(
				supported_export_type,
				supported_export_format,
				export_configuration);

		// Mark the export as having been added.
		d_exporters_added.insert(supported_export_id);
	}
}
