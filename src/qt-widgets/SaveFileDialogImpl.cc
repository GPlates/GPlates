/* $Id$ */

/**
 * @file 
 * Contains implementation of SaveFileDialog.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * (some code in this file was originally part of "SaveFileDialog.cc")
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

#include <QStringList>
#include <QFileInfo>

#include "SaveFileDialogImpl.h"

#include "presentation/ViewState.h"

#include <boost/foreach.hpp>

namespace
{
	QString
	get_file_extension(
			const QString &filename)
	{
		// Special handling for ".gpml.gz".
		static const QString GPML_GZ_EXT = "gpml.gz";
		static const QString GPML_GZ_EXT_WITH_DOT = ".gpml.gz";
		if (filename.endsWith(GPML_GZ_EXT_WITH_DOT))
		{
			return GPML_GZ_EXT;
		}

		QStringList parts = filename.split(".");
		if (parts.size() > 1)
		{
			return parts.at(parts.size() - 1);
		}
		else
		{
			return QString();
		}
	}


	void
	add_exts_to_map(
			const std::vector<QString> &extensions,
			const QString &filter_string,
			std::map<QString, QString> &map)
	{
		BOOST_FOREACH(const QString &extension, extensions)
		{
			map.insert(std::make_pair(extension, filter_string));
		}
	}
}


GPlatesQtWidgets::SaveFileDialogInternals::NativeSaveFileDialog::NativeSaveFileDialog(
		QWidget *parent_,
		const QString &caption,
		const filter_list_type &filters,
		GPlatesPresentation::ViewState &view_state) :
	d_parent_ptr(parent_),
	d_caption(caption),
	d_view_state(view_state)
{
	set_filters(filters);
}


boost::optional<QString>
GPlatesQtWidgets::SaveFileDialogInternals::NativeSaveFileDialog::get_file_name(
		QString *selected_filter)
{
	QString file_extension = get_file_extension(d_last_file_name);
	QString temp_selected_filter;
	std::map<QString, QString>::const_iterator iter = d_filter_map_ext_to_text.find(file_extension);
	if (iter != d_filter_map_ext_to_text.end())
	{
		temp_selected_filter = iter->second;
	}

	QString filename = QFileDialog::getSaveFileName(
			d_parent_ptr,
			d_caption,
			d_last_file_name.isEmpty()
				? d_view_state.get_last_open_directory()
				: d_last_file_name,
			d_filters,
			&temp_selected_filter);

	if (filename.isEmpty())
	{
		return boost::none;
	}

	d_last_file_name = filename;
	d_view_state.get_last_open_directory() = QFileInfo(filename).path();
	if (selected_filter)
	{
		*selected_filter = temp_selected_filter;
	}
	return filename;
}


void
GPlatesQtWidgets::SaveFileDialogInternals::NativeSaveFileDialog::set_filters(
		const filter_list_type &filters)
{
	d_filter_map_ext_to_text.clear();

	BOOST_FOREACH(const FileDialogFilter &filter, filters)
	{
		add_exts_to_map(filter.get_extensions(), filter.create_filter_string(), d_filter_map_ext_to_text);
	}

	// Save combined filter string.
	d_filters = FileDialogFilter::create_filter_string(filters.begin(), filters.end());
}


void
GPlatesQtWidgets::SaveFileDialogInternals::NativeSaveFileDialog::select_file(
		const QString &file_path)
{
	d_last_file_name = file_path;
}


GPlatesQtWidgets::SaveFileDialogInternals::QtSaveFileDialog::QtSaveFileDialog(
		QWidget *parent_,
		const QString &caption,
		const filter_list_type &filters,
		GPlatesPresentation::ViewState &view_state) :
	d_view_state(view_state),
	d_file_dialog_ptr(
			new QFileDialog(
				parent_,
				caption))
{
	d_file_dialog_ptr->setFileMode(QFileDialog::AnyFile);
	d_file_dialog_ptr->setAcceptMode(QFileDialog::AcceptSave);

	set_filters(filters);

	// Listen to changes to the filter in the dialog box.
	QObject::connect(
			d_file_dialog_ptr.get(),
			SIGNAL(filterSelected(const QString&)),
			this,
			SLOT(handle_filter_changed()));
}


boost::optional<QString>
GPlatesQtWidgets::SaveFileDialogInternals::QtSaveFileDialog::get_file_name(
		QString *selected_filter)
{
	if(d_file_dialog_ptr->defaultSuffix().isEmpty())
	{
		handle_filter_changed();
	}
	// If no file currently selected, use the last open directory.
	if (!QFileInfo(d_file_dialog_ptr->selectedFiles().front()).isFile())
	{
		d_file_dialog_ptr->setDirectory(d_view_state.get_last_open_directory());
	}

	if (!d_file_dialog_ptr->exec())
	{
		return boost::none;
	}

	QString filename = d_file_dialog_ptr->selectedFiles().front();

	if (filename.isEmpty())
	{
		return boost::none;
	}

	d_view_state.get_last_open_directory() = QFileInfo(filename).path();
	if (selected_filter)
	{
		*selected_filter = d_file_dialog_ptr->selectedNameFilter();
	}
	return boost::optional<QString>(filename);
}


void
GPlatesQtWidgets::SaveFileDialogInternals::QtSaveFileDialog::handle_filter_changed()
{
	QString filter = d_file_dialog_ptr->selectedNameFilter();
	QString suffix_string = d_filter_map_text_to_ext[filter];
	d_file_dialog_ptr->setDefaultSuffix(suffix_string);
}


void
GPlatesQtWidgets::SaveFileDialogInternals::QtSaveFileDialog::set_filters(
		const filter_list_type &filters)
{
	// Tell the QFileDialog what the filter is.
	QString combined_filter_string = FileDialogFilter::create_filter_string(
				filters.begin(), filters.end());
	d_file_dialog_ptr->setNameFilter(combined_filter_string);

	// Store the filters in a map for quick reference.
	d_filter_map_text_to_ext.clear();
	d_filter_map_ext_to_text.clear();
	d_file_dialog_ptr->selectNameFilter(QString());
	d_file_dialog_ptr->setDefaultSuffix(QString());
	for (filter_list_type::const_iterator iter = filters.begin(); iter != filters.end(); ++iter)
	{
		const FileDialogFilter &filter = *iter;
		const std::vector<QString> &extensions = filter.get_extensions();
		QString filter_string = filter.create_filter_string();
		add_exts_to_map(extensions, filter_string, d_filter_map_ext_to_text);
		if (iter == filters.begin())
		{
			d_file_dialog_ptr->selectNameFilter(filter_string);
		}
		if (extensions.size() > 0)
		{
			d_filter_map_text_to_ext.insert(std::make_pair(filter_string, extensions[0]));
			if (iter == filters.begin())
			{
				d_file_dialog_ptr->setDefaultSuffix(extensions[0]);
			}
		}
	}
}


void
GPlatesQtWidgets::SaveFileDialogInternals::QtSaveFileDialog::select_file(
		const QString &file_path)
{
	d_file_dialog_ptr->selectFile(file_path);

	// If the file does not exist, on Ubuntu 8.04 at least, the file name field in the
	// dialog box has a backslash at the front, which means that if the user just clicks
	// ok, the save operation is most likely going to fail, because GPlates will attempt
	// to save the file in the root directory. Selecting the file again with just the
	// file name seems to solve this problem.
	QStringList tokens = file_path.split("/");
	d_file_dialog_ptr->selectFile(tokens.last());

	QString file_ext = get_file_extension(file_path);
	d_file_dialog_ptr->selectNameFilter(d_filter_map_ext_to_text[file_ext]);
	d_file_dialog_ptr->setDefaultSuffix(file_ext);
}

