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

#include <algorithm>
#include <map>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QColorGroup>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollArea>

#include "ConfigureExportParametersDialog.h"
#include "ExportAnimationDialog.h"
#include "ExportFileNameTemplateWidget.h"
#include "ExportOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "gui/ExportAnimationRegistry.h"
#include "gui/ExportAnimationType.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::ConfigureExportParametersDialog::ConfigureExportParametersDialog(
		GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
		QWidget *parent_):
	QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint),
	d_export_animation_context_ptr(export_animation_context_ptr),
	d_is_single_frame(false),
	d_export_format_list_widget(NULL),
	d_export_file_name_template_widget(NULL),
	d_export_options_widget_layout(NULL)
{
	setupUi(this);

	// We use our own list widgets that resize to the contents of the lists.
	// For the export *format* list widget this is needed so that the scroll area, just below it,
	// has all remaining space available to it.

	d_export_format_list_widget = new ExportFormatListWidget(this);
	QtWidgetUtils::add_widget_to_placeholder(d_export_format_list_widget, list_widget_format_placeholder);
    QSizePolicy list_widget_format_size_policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    d_export_format_list_widget->setSizePolicy(list_widget_format_size_policy);
	d_export_format_list_widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	d_export_format_list_widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Make the export options a scroll area since we don't know how many options
	// will be dynamically placed there.
	QWidget *scrollarea_widget = new QWidget(this);

	// Give the export options widget a layout.
	d_export_options_widget_layout = new QVBoxLayout(scrollarea_widget);
	d_export_options_widget_layout->setContentsMargins(0, 0, 0, 0);
	// If there's not enough options to fill the scroll area then take up extra space with a spacer item.
	d_export_options_widget_layout->addStretch();

	// Qt advises setting the widget on the scroll area after its layout has been set.
	widget_export_options->setWidget(scrollarea_widget);

	// Create the filename template widget and add it to the placeholder.
	d_export_file_name_template_widget = new ExportFileNameTemplateWidget(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_export_file_name_template_widget,
			export_filename_template_place_holder);

	// Give more space to the right side of the splitter.
	// That's where the export options are - we don't want to crowd them too much.
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 2);

	initialize_export_type_list_widget();

	// Only enable 'accept' button when an export type and format have been selected.
	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(false);

	QObject::connect(
			export_type_list_widget,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(react_export_type_selection_changed()));
	QObject::connect(
			export_type_list_widget,
			SIGNAL(itemClicked(QListWidgetItem *)),
			this,
			SLOT(react_export_type_selection_changed()));
	QObject::connect(
			d_export_format_list_widget,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(react_export_format_selection_changed()));

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
			export_type_list_widget,
			SIGNAL(itemPressed(QListWidgetItem *)),
			this,
			SLOT(focus_on_listwidget_format()));
	QObject::connect(
			d_export_format_list_widget,
			SIGNAL(itemPressed(QListWidgetItem *)),
			d_export_file_name_template_widget,
			SLOT(focus_on_line_edit_filename()));
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_export_type_list_widget()
{
	export_type_list_widget->clear();
	d_export_format_list_widget->clear();
	// Update the geometry since we override the size hint to match the contents size.
	d_export_format_list_widget->updateGeometry();
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

		// Add a widget item for the current export type.
		QListWidgetItem *widget_item = new ExportTypeWidgetItem<QListWidgetItem>(supported_export_type);
		export_type_list_widget->addItem(widget_item);
		widget_item->setText(get_export_type_name(supported_export_type));
	}
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_type_selection_changed()
{
	if (!export_type_list_widget->currentItem())
	{
		return;
	}

	// Only enable 'accept' button when an export type and format have been selected.
	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(false);

	d_export_file_name_template_widget->clear_file_name_template();
	d_export_format_list_widget->clear();
	// Update the geometry since we override the size hint to match the contents size.
	d_export_format_list_widget->updateGeometry();
	clear_export_options_widget();

	const GPlatesGui::ExportAnimationType::Type selected_export_type =
			get_export_type(export_type_list_widget->currentItem());

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

		QListWidgetItem *item = new ExportFormatWidgetItem<QListWidgetItem>(export_format);
		d_export_format_list_widget->addItem(item);
		// Update the geometry since we override the size hint to match the contents size.
		d_export_format_list_widget->updateGeometry();
		item->setText(get_export_format_description(export_format));
	}
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_format_selection_changed()
{
	if (!export_type_list_widget->currentItem() ||
		!d_export_format_list_widget->currentItem())
	{
		return;
	}

	const GPlatesGui::ExportAnimationType::Type selected_export_type =
			get_export_type(export_type_list_widget->currentItem());
	const GPlatesGui::ExportAnimationType::Format selected_export_format =
			get_export_format(d_export_format_list_widget->currentItem());
	
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
	// Make sure the selected export id is supported.
	//
	// An unsupported export id can happen when react_export_type_selection_changed()
	// is signaled which then clears the format widget which in turn signals
	// react_export_format_selection_changed.
	// In this situation the current export format (leftover from a previous format selection
	// for a different type of export) might not be supported for the current export type.
	// Get a list of all the currently supported exporters.
	//
	const std::vector<GPlatesGui::ExportAnimationType::ExportID> supported_exporters =
			export_animation_registry.get_registered_exporters();
	if (std::find(supported_exporters.begin(), supported_exporters.end(), selected_export_id) ==
		supported_exporters.end())
	{
		// Removing warning since this happens quite often in certain situations.
		//qWarning() << "Unsupported export id selected";
		return;
	}


	//
	// Display the filename template.
	//

	const QString filename_template =
			export_animation_registry.get_default_filename_template(selected_export_id);
	d_export_file_name_template_widget->set_file_name_template(filename_template, selected_export_format);

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

	// Enable 'accept' button now that an export type and format have been selected.
	main_buttonbox->button(QDialogButtonBox::Ok)->setEnabled(true);
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_add_item_clicked()
{
	if (!export_type_list_widget->currentItem() ||
		!d_export_format_list_widget->currentItem())
	{
		return;
	}

	QString filename_template = d_export_file_name_template_widget->get_file_name_template();

	// Get the currently selected export type and format.
	const GPlatesGui::ExportAnimationType::Type selected_export_type = get_export_type(
			export_type_list_widget->currentItem());
	const GPlatesGui::ExportAnimationType::Format selected_export_format = get_export_format(
			d_export_format_list_widget->currentItem());

	// Determine the corresponding export ID.
	const GPlatesGui::ExportAnimationType::ExportID selected_export_id =
			get_export_id(selected_export_type, selected_export_format);

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Validate the filename template against the selected exporter.
	QString filename_template_validation_message;
	if (!export_animation_registry.validate_filename_template(
			selected_export_id,
			filename_template,
			filename_template_validation_message,
			!d_is_single_frame/*check_filename_variation*/))
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

	clear_export_options_widget();

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
	initialize_export_type_list_widget();

	d_export_file_name_template_widget->clear_file_name_template();
	label_export_description->clear();
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::focus_on_listwidget_format()
{
	d_export_format_list_widget->setFocus();
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
			export_id, this, *d_export_animation_context_ptr);

	if (d_current_export_options_widget)
	{
		d_current_export_options_widget.get()->layout()->setContentsMargins(0, 0, 0, 0);
		// We 'insert' rather than 'add' the widget so that the spacer item added in constructor
		// is always last.
		d_export_options_widget_layout->insertWidget(0, d_current_export_options_widget.get());

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
	}
}
