/* $Id$ */

/**
 * \file 
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
 
#ifndef GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H
#define GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QWidget>

#include "ManageFeatureCollectionsActionWidgetUi.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormat.h"


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}
}

namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsDialog;
	
	
	class ManageFeatureCollectionsActionWidget:
			public QWidget, 
			protected Ui_ManageFeatureCollectionsActionWidget
	{
		Q_OBJECT

	public:
		/**
		 * Constructor.
		 *
		 * NOTE: This disables all buttons and functionality.
		 * You need to call @a update at least once to set things up.
		 */
		explicit
		ManageFeatureCollectionsActionWidget(
				ManageFeatureCollectionsDialog &feature_collections_dialog,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
				QWidget *parent_ = NULL);

		/**
		 * Updates with a new filename.
		 *
		 * @a file_format is boost::none if the file's format could not be determined.
		 */
		void
		update(
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				const GPlatesFileIO::FileInfo &fileinfo,
				boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format,
				bool enable_edit_configuration);

		//! Returns the file referenced by this action widget.
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		get_file_reference() const
		{
			return d_file_reference;
		}

	private slots:
		
		void
		handle_edit_configuration();

		void
		handle_save();
		
		void
		handle_save_as();
		
		void
		handle_save_copy();
		
		void
		handle_reload();

		void
		handle_unload();
	
	private:
		ManageFeatureCollectionsDialog &d_feature_collections_dialog;
		GPlatesAppLogic::FeatureCollectionFileState::file_reference d_file_reference;
	};
}

#endif  // GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSACTIONWIDGET_H
