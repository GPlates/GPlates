/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "DatelineWrapOptionsWidget.h"

#include "InformationDialog.h"


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


GPlatesQtWidgets::DatelineWrapOptionsWidget::DatelineWrapOptionsWidget(
		QWidget *parent_,
		bool wrap_to_dateline) :
	QWidget(parent_),
	d_help_dateline_wrap_dialog(
			new InformationDialog(
					HELP_DATELINE_WRAP_DIALOG_TEXT,
					HELP_DATELINE_WRAP_DIALOG_TITLE,
					this))
{
	setupUi(this);

	// Connect the help dialog.
	QObject::connect(
			push_button_help_dateline_wrap, SIGNAL(clicked()),
			d_help_dateline_wrap_dialog, SLOT(show()));

	// Set initial options.
	check_box_wrap_dateline->setChecked(wrap_to_dateline);
}


void
GPlatesQtWidgets::DatelineWrapOptionsWidget::set_options(
		bool wrap_to_dateline)
{
	// Dateline wrapping check box.
	check_box_wrap_dateline->setChecked(wrap_to_dateline);
}


bool
GPlatesQtWidgets::DatelineWrapOptionsWidget::get_wrap_to_dateline() const
{
	return check_box_wrap_dateline->isChecked();
}


void
GPlatesQtWidgets::DatelineWrapOptionsWidget::reset_options()
{
	// Default is no dateline wrapping.
	check_box_wrap_dateline->setChecked(false);
}
