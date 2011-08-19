/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2009 Geological Survey of Norway
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

#include "ShapefileFileFormatConfigurationDialog.h"

#include "InformationDialog.h"
#include "ShapefileAttributeWidget.h"
#include "QtWidgetUtils.h"

#include "utils/UnicodeStringUtils.h"


namespace GPlatesQtWidgets
{
	namespace
	{
		const QString HELP_DATELINE_WRAP_DIALOG_TITLE = QObject::tr("Dateline wrap");
		const QString HELP_DATELINE_WRAP_DIALOG_TEXT = QObject::tr(
				"<html><body>\n"
				"<h3>Enable/disable dateline wrapping</h3>"
				"<p>If this option is enabled then polyline and polygon geometries will be clipped "
				"to the dateline (if they intersect it) and wrapped to the other side as needed.</p>"
				"<p>Note that this can break a polyline into multiple polylines or a polygon into "
				"multiple polygons - and once saved this process is irreversible - in other words "
				"reloading the saved file will not undo the wrapping.</p>"
				"<p><em>This option is provided to support ArcGIS users - it prevents horizontal "
				"lines across the display when viewing geometries, in ArcGIS, that cross the dateline.</em></p>"
				"</body></html>\n");
	}
}

GPlatesQtWidgets::ShapefileFileFormatConfigurationDialog::ShapefileFileFormatConfigurationDialog(
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_help_dateline_wrap_dialog(
			new InformationDialog(
					HELP_DATELINE_WRAP_DIALOG_TEXT,
					HELP_DATELINE_WRAP_DIALOG_TITLE,
					this))
{
	setupUi(this);

	// Connect the help dialogs.
	QObject::connect(
			push_button_help_dateline_wrap, SIGNAL(clicked()),
			d_help_dateline_wrap_dialog, SLOT(show()));

	// Button box signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(clicked(QAbstractButton *)),
			this,
			SLOT(handle_buttonbox_clicked(QAbstractButton *)));
}


void
GPlatesQtWidgets::ShapefileFileFormatConfigurationDialog::setup(
		bool &dateline_wrap,
		const QString &filename,
		const QStringList &field_names,
		QMap< QString,QString > &model_to_attribute_map)
{
	// Dateline wrapping check box.
	d_dateline_wrap = &dateline_wrap;
	check_box_wrap_dateline->setChecked(dateline_wrap);

	d_shapefile_attribute_widget = new ShapefileAttributeWidget(this,filename,field_names,model_to_attribute_map,true);
	QtWidgetUtils::add_widget_to_placeholder(
			d_shapefile_attribute_widget,
			widget_shapefile_attribute);
}


void
GPlatesQtWidgets::ShapefileFileFormatConfigurationDialog::accept()
{
	if (d_dateline_wrap)
	{
		*d_dateline_wrap = check_box_wrap_dateline->isChecked();
	}

	d_shapefile_attribute_widget->accept_fields();

	done(QDialog::Accepted);
}


void
GPlatesQtWidgets::ShapefileFileFormatConfigurationDialog::reset_fields()
{
	d_shapefile_attribute_widget->reset_fields();

	// Default is to disable dateline wrapping.
	check_box_wrap_dateline->setChecked(false);
}


void
GPlatesQtWidgets::ShapefileFileFormatConfigurationDialog::handle_buttonbox_clicked(
		QAbstractButton *button)
{
	if (main_buttonbox->buttonRole(button) == QDialogButtonBox::ResetRole)
	{
		reset_fields();
	}
}

