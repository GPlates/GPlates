/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2016 Geological Survey of Norway
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

#include <ogr_spatialref.h>
#include "OgrSrsWriteOptionDialog.h"

GPlatesQtWidgets::OgrSrsWriteOptionDialog::OgrSrsWriteOptionDialog(
	QWidget *parent_):
	QDialog(parent_)
{
	setupUi(this);
	set_up_connections();

	radio_wgs84_srs->setChecked(true);
	button_group->setId(radio_wgs84_srs,WRITE_TO_WGS84_SRS);
	button_group->setId(radio_original_srs,WRITE_TO_ORIGINAL_SRS);
}

void GPlatesQtWidgets::OgrSrsWriteOptionDialog::initialise(
		const QString &filename,
		const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type &srs)
{
	// Grab a readable form of the projection using the OGR API
	char *pretty_wkt;
	srs->get_ogr_srs().exportToPrettyWkt(&pretty_wkt);
	plain_text_wkt->setPlainText(pretty_wkt);

	label_file->setText(QObject::tr("The file"));

	// Set bold filename
	QFont font_ = label_filename->font();
	font_.setBold(true);
	label_filename->setText(filename);
	label_filename->setFont(font_);

	// Set explanatory text
	QString text;
	text.append(QObject::tr("has a non-WGS84 spatial reference system associated with it, \n"));
	text.append(QObject::tr("which was converted to WGS84 on input to GPlates.\n\n"));
	text.append(QObject::tr("The original spatial reference system was: "));

	label_info->setText(text);
}


void
GPlatesQtWidgets::OgrSrsWriteOptionDialog::set_up_connections()
{
	QObject::connect(button_ok,SIGNAL(clicked()),this,SLOT(handle_ok()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(handle_cancel()));
}

void
GPlatesQtWidgets::OgrSrsWriteOptionDialog::handle_ok()
{
	done(button_group->checkedId());
}

void GPlatesQtWidgets::OgrSrsWriteOptionDialog::handle_cancel()
{
	done(DO_NOT_WRITE);
}
