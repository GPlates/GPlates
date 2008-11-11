/* $Id$ */

/**
 * \file 
 * Qt dialog for inputting style of GMT header.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include "GMTHeaderFormatDialog.h"

GPlatesQtWidgets::GMTHeaderFormatDialog::GMTHeaderFormatDialog(
	QWidget *parent_) :
QDialog(parent_),
d_header_format(GPlatesFileIO::FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER)
{
	setupUi(this);

	radio_button_plates4_header->setChecked(true);

	QObject::connect(push_button_finished, SIGNAL(clicked()), this, SLOT(finished()));
}

void
GPlatesQtWidgets::GMTHeaderFormatDialog::finished()
{
	if (radio_button_plates4_header->isChecked())
	{
		d_header_format =
			GPlatesFileIO::FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER;
	}
	else if (radio_button_feature_properties->isChecked())
	{
		d_header_format =
			GPlatesFileIO::FeatureCollectionWriteFormat::GMT_VERBOSE_HEADER;
	}
	else if (radio_button_prefer_plate4_style->isChecked())
	{
		d_header_format =
			GPlatesFileIO::FeatureCollectionWriteFormat::GMT_PREFER_PLATES4_STYLE_HEADER;
	}
	else
	{
		d_header_format =
			GPlatesFileIO::FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER;
	}

	close();
}
