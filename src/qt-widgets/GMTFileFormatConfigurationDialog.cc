/* $Id$ */

/**
 * \file 
 * Qt dialog for inputting style of GMT header.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "GMTFileFormatConfigurationDialog.h"

#include "global/GPlatesAssert.h"


GPlatesQtWidgets::GMTFileFormatConfigurationDialog::GMTFileFormatConfigurationDialog(
		const GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type &configuration,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_configuration(new GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration(*configuration))
{
	setupUi(this);

	switch (d_configuration->get_header_format())
	{
	case GPlatesFileIO::GMTFormatWriter::PLATES4_STYLE_HEADER:
		radio_button_plates4_header->setChecked(true);
		break;
	case GPlatesFileIO::GMTFormatWriter::VERBOSE_HEADER:
		radio_button_feature_properties->setChecked(true);
		break;
	case GPlatesFileIO::GMTFormatWriter::PREFER_PLATES4_STYLE_HEADER:
		radio_button_prefer_plate4_style->setChecked(true);
		break;
	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	QObject::connect(push_button_finished, SIGNAL(clicked()), this, SLOT(finished()));
}

void
GPlatesQtWidgets::GMTFileFormatConfigurationDialog::finished()
{
	if (radio_button_plates4_header->isChecked())
	{
		d_configuration->set_header_format(GPlatesFileIO::GMTFormatWriter::PLATES4_STYLE_HEADER);
	}
	else if (radio_button_feature_properties->isChecked())
	{
		d_configuration->set_header_format(GPlatesFileIO::GMTFormatWriter::VERBOSE_HEADER);
	}
	else if (radio_button_prefer_plate4_style->isChecked())
	{
		d_configuration->set_header_format(GPlatesFileIO::GMTFormatWriter::PREFER_PLATES4_STYLE_HEADER);
	}
	else
	{
		d_configuration->set_header_format(GPlatesFileIO::GMTFormatWriter::PLATES4_STYLE_HEADER);
	}

	close();
}
