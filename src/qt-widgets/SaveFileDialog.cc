/* $Id$ */

/**
 * @file 
 * Contains implementation of SaveFileDialog.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "presentation/ViewState.h"
#include "SaveFileDialog.h"

#include "SaveFileDialogImpl.h"


// We use the native dialog, instead of the Qt dialog, on Windows and Mac OS X.
#if defined(Q_WS_WIN) || defined(Q_WS_MAC)
#	define GPLATES_USE_NATIVE_FILE_DIALOG
#endif


GPlatesQtWidgets::SaveFileDialog::SaveFileDialog(
		QWidget *parent,
		const QString &caption,
		const filter_list_type &filters,
		GPlatesPresentation::ViewState &view_state) :
#if defined(GPLATES_USE_NATIVE_FILE_DIALOG)
	d_impl(
			new SaveFileDialogInternals::NativeSaveFileDialog(
				parent,
				caption,
				filters,
				view_state.get_file_io_directory_configurations().feature_collection_configuration()))
#else
	d_impl(
			new SaveFileDialogInternals::QtSaveFileDialog(
				parent,
				caption,
				filters,
				view_state.get_file_io_directory_configurations().feature_collection_configuration()))
#endif
{  }

GPlatesQtWidgets::SaveFileDialog::SaveFileDialog(
		QWidget *parent,
		const QString &caption,
		const filter_list_type &filters,
		GPlatesGui::DirectoryConfiguration &configuration) :
#if defined(GPLATES_USE_NATIVE_FILE_DIALOG)
	d_impl(
			new SaveFileDialogInternals::NativeSaveFileDialog(
				parent,
				caption,
				filters,
				configuration))
#else
	d_impl(
			new SaveFileDialogInternals::QtSaveFileDialog(
				parent,
				caption,
				filters,
				configuration))
#endif
{  }


GPlatesQtWidgets::SaveFileDialog::~SaveFileDialog()
{  }


boost::optional<QString>
GPlatesQtWidgets::SaveFileDialog::get_file_name(
		QString *selected_filter)
{
	return d_impl->get_file_name(selected_filter);
}


void
GPlatesQtWidgets::SaveFileDialog::set_filters(
		const filter_list_type &filters)
{
	d_impl->set_filters(filters);
}


void
GPlatesQtWidgets::SaveFileDialog::select_file(
		const QString &file_path)
{
	d_impl->select_file(file_path);
}

