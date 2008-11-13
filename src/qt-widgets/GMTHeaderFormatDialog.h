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

#ifndef GPLATES_QTWIDGETS_GMTHEADERFORMATDIALOG_H
#define GPLATES_QTWIDGETS_GMTHEADERFORMATDIALOG_H

#include "GMTHeaderFormatDialogUi.h"
#include "file-io/FeatureCollectionFileFormat.h"

namespace GPlatesQtWidgets
{
	/**
	 * Dialog for inputing the style GMT header to write to file.
	 */
	class GMTHeaderFormatDialog :
		public QDialog,
		protected Ui_GMTHeaderFormatDialog
	{
		Q_OBJECT

	public:
		explicit
		GMTHeaderFormatDialog(
				QWidget *parent_ = NULL);

		/**
		 * Returns GMT header format selected by user after dialog closes.
		 */
		GPlatesFileIO::FeatureCollectionWriteFormat::Format
				get_header_format() const
		{
			return d_header_format;
		}

	public slots:
		void
		finished();

	private:
		GPlatesFileIO::FeatureCollectionWriteFormat::Format d_header_format;
	};
}

#endif // GPLATES_QTWIDGETS_GMTHEADERFORMATDIALOG_H
