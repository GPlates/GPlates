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

#ifndef GPLATES_QTWIDGETS_GMTFILEFORMATCONFIGURATIONDIALOG_H
#define GPLATES_QTWIDGETS_GMTFILEFORMATCONFIGURATIONDIALOG_H

#include "GMTFileFormatConfigurationDialogUi.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatConfigurations.h"
#include "file-io/GMTFormatWriter.h"


namespace GPlatesQtWidgets
{
	/**
	 * Dialog for configuring write-only ".xy" GMT file format.
	 *
	 * Current configuration includes:
	 *  - the style GMT header to write to file.
	 */
	class GMTFileFormatConfigurationDialog :
		public QDialog,
		protected Ui_GMTFileFormatConfigurationDialog
	{
		Q_OBJECT

	public:
		explicit
		GMTFileFormatConfigurationDialog(
				const GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type &
						configuration,
				QWidget *parent_ = NULL);

		//! Returns configuration selected by user after dialog closes.
		GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_to_const_type
		get_configuration() const
		{
			return d_configuration;
		}

	public slots:
		void
		finished();

	private:
		GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::shared_ptr_type d_configuration;
	};
}

#endif // GPLATES_QTWIDGETS_GMTFILEFORMATCONFIGURATIONDIALOG_H
