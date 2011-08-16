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

#include <QDebug>

#include "ManageFeatureCollectionsEditConfigurations.h"

#include "GMTFileFormatConfigurationDialog.h"
#include "ManageFeatureCollectionsDialog.h"


void
GPlatesQtWidgets::ManageFeatureCollections::register_default_edit_configurations(
		ManageFeatureCollectionsDialog &manage_feature_collections_dialog)
{
	manage_feature_collections_dialog.register_edit_configuration(
			GPlatesFileIO::FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT,
			GMTEditConfiguration::shared_ptr_type(new GMTEditConfiguration()));
}


GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
GPlatesQtWidgets::ManageFeatureCollections::GMTEditConfiguration::edit_configuration(
		const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_reference,
		const GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type &
				current_configuration,
		QWidget *parent_widget)
{
	// Cast to the correct derived configuration type.
	GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type
			current_gmt_configuration =
					boost::dynamic_pointer_cast<
							const GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration>(
									current_configuration);
	// If, for some reason, we were passed the wrong derived type then just emit a warning and
	// return the configuration passed to us.
	if (!current_gmt_configuration)
	{
		// Note that we would normally assert here but being a bit lenient since might have
		// detected the wrong file format somewhere.
		qWarning() << "Internal Error: Expected a GMT configuration but got something else.";

		return current_configuration;
	}

	GMTFileFormatConfigurationDialog dialog(current_gmt_configuration, parent_widget);
	dialog.exec();

	return dialog.get_configuration();
}
