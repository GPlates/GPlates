/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H
#define GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H

#include <vector>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>
#include <QWizard>

#include "OpenFileDialog.h"

#include "model/PropertyValue.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesGui
{
	class FileIOFeedback;
	class UnsavedChangesTracker;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ImportScalarField3DDialog :
			public QWizard
	{
		Q_OBJECT

	public:

		explicit
		ImportScalarField3DDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
				GPlatesGui::FileIOFeedback *file_io_feedback,
				QWidget *parent_ = NULL);

		/**
		 * Call this to open the import scalar field wizard, instead of @a show().
		 */
		void
		display(
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);

	private:

		GPlatesModel::PropertyValue::non_null_ptr_type
		create_scalar_field_3d_file(
				const QFileInfo &file_info);

		QString
		create_gpml_file_path(
				const QFileInfo &file_info) const;

		static const QString GPML_EXT;

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesGui::UnsavedChangesTracker *d_unsaved_changes_tracker;
		GPlatesGui::FileIOFeedback *d_file_io_feedback;
		OpenFileDialog d_open_file_dialog;

		// For communication between pages.
		bool d_save_after_finish;

		int d_scalar_field_feature_collection_page_id;
	};
}

#endif // GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H
