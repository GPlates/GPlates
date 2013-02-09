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

#include <QCheckBox>
#include <QLabel>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollArea>

#include "EditExportParametersDialog.h"
#include "ExportAnimationDialog.h"
#include "ExportOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/ExportAnimationRegistry.h"
#include "gui/ExportAnimationType.h"

#include "presentation/ViewState.h"


namespace GPlatesQtWidgets
{
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
}


GPlatesQtWidgets::EditExportParametersDialog::EditExportParametersDialog(
		GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
		QWidget *parent_):
	QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint),
	d_export_animation_context_ptr(
			export_animation_context_ptr),
	d_export_options_widget_layout(NULL)
{
	setupUi(this);
	set_fixed_size_for_item_view(treeWidget_template);
	treeWidget_template->setHeaderHidden(true);
	treeWidget_template->header()->setResizeMode(0, QHeaderView::ResizeToContents);

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

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(react_edit_item_accepted()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	clear_export_options_widget();
}


void
GPlatesQtWidgets::EditExportParametersDialog::initialise(
		int export_row_in_animation_dialog,
		GPlatesGui::ExportAnimationType::ExportID export_id,
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration)
{
	lineEdit_filename->clear();
	label_file_extension->clear();

	d_export_row_in_animation_dialog = export_row_in_animation_dialog;
	d_export_id = export_id;

	//
	// Display the filename template.
	//

	// Separate the filename template into base name and extension.
	const QString filename_template = export_configuration->get_filename_template();
	const QString export_format_filename_extension = "." +
			GPlatesGui::ExportAnimationType::get_export_format_filename_extension(
					GPlatesGui::ExportAnimationType::get_export_format(d_export_id.get()));

	// Set the filename template base name and extension.
	if (filename_template.endsWith(export_format_filename_extension))
	{
		lineEdit_filename->setText(
				filename_template.left(filename_template.size() - export_format_filename_extension.size()));
		label_file_extension->setText(
				filename_template.right(export_format_filename_extension.size()));
	}
	else
	{
		// File format has no filename extension.
		lineEdit_filename->setText(filename_template);
		label_file_extension->setText("");
	}

	//
	// Display the export options.
	//

	set_export_options_widget(export_configuration);


	// Focus on the filename template line edit.
	lineEdit_filename->setFocus();
}


void
GPlatesQtWidgets::EditExportParametersDialog::react_edit_item_accepted()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_export_id && d_export_row_in_animation_dialog,
			GPLATES_ASSERTION_SOURCE);

	QString filename_template = lineEdit_filename->text() + label_file_extension->text();		

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	// Validate the filename template against the selected exporter.
	QString filename_template_validation_message;
	if (!export_animation_registry.validate_filename_template(
			d_export_id.get(), filename_template, filename_template_validation_message))
	{
		QMessageBox error_popup;
		error_popup.setWindowTitle(QString("Cannot Commit Edited Data to Export"));
		error_popup.setText(QString("The filename template contains an invalid format string."));
		error_popup.setInformativeText(filename_template_validation_message);
		error_popup.setIcon(QMessageBox::Warning);
		error_popup.exec();

		return;
	}

	// If we have an export options widget then get it to create the export animation configuration.
	// Otherwise just create the default configuration.
	GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr export_cfg;
	if (d_export_options_widget)
	{
		export_cfg = d_export_options_widget.get()->
					create_export_animation_strategy_configuration(filename_template);
	}
	else
	{
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr const_default_export_cfg =
				export_animation_registry.get_default_export_configuration(d_export_id.get());
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

	// Edit the exporter in the export animation dialog.
	d_export_animation_context_ptr->get_export_dialog()->edit_item(
			d_export_row_in_animation_dialog.get(),
			export_cfg);

	accept();
}


void
GPlatesQtWidgets::EditExportParametersDialog::clear_export_options_widget()
{
	if (d_export_options_widget)
	{
		d_export_options_widget_layout->removeWidget(d_export_options_widget.get());
		delete d_export_options_widget.get();

		d_export_options_widget = boost::none;
	}

	widget_export_options->setEnabled(false);
	widget_export_options->setVisible(false);
}


void
GPlatesQtWidgets::EditExportParametersDialog::set_export_options_widget(
		const GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr &export_configuration)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_export_id,
			GPLATES_ASSERTION_SOURCE);

	GPlatesGui::ExportAnimationRegistry &export_animation_registry =
			d_export_animation_context_ptr->view_state().get_export_animation_registry();

	if (d_export_options_widget)
	{
		d_export_options_widget_layout->removeWidget(d_export_options_widget.get());
		delete d_export_options_widget.get();
	}

	// Create an export options widget to edit the export configuration.
	d_export_options_widget = export_animation_registry.create_export_options_widget(
			d_export_id.get(),
			this,
			export_configuration);

	if (d_export_options_widget)
	{
		d_export_options_widget.get()->layout()->setContentsMargins(0, 0, 0, 0);
		// We 'insert' rather than 'add' the widget so that the spacer item added in constructor
		// is always last.
		d_export_options_widget_layout->insertWidget(0, d_export_options_widget.get());

		widget_export_options->setEnabled(true);
		widget_export_options->setVisible(true);
	}
	else
	{
		widget_export_options->setEnabled(false);
		widget_export_options->setVisible(false);
	}
}
