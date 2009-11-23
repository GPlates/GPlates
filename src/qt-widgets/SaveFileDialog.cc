/* $Id$ */

/**
 * @file 
 * Contains implementation of SaveFileDialog.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "SaveFileDialog.h"

namespace
{
	QString
	get_output_filters(
			const std::vector<std::pair<QString, QString> > &filters)
	{
		QStringList file_filters;
		typedef std::vector<std::pair<QString, QString> >::const_iterator iterator_type;
		for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
		{
			file_filters << iter->first;
		}

		return file_filters.join(";;");
	}

	QString
	get_file_extension(
			const QString &filename)
	{
		// hack for gpml.gz
		if (filename.endsWith("gpml.gz"))
		{
			return "gpml.gz";
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
}

GPlatesQtWidgets::SaveFileDialog::~SaveFileDialog()
{
}

boost::shared_ptr<GPlatesQtWidgets::SaveFileDialog>
GPlatesQtWidgets::SaveFileDialog::get_save_file_dialog(
		QWidget *parent,
		const QString &caption,
		const std::vector<std::pair<QString, QString> > &filters,
		unsigned int default_filter)
{
#ifdef __WINDOWS__
	return boost::shared_ptr<GPlatesQtWidgets::SaveFileDialog>(
			new WindowsSaveFileDialog(
				parent,
				caption,
				filters,
				default_filter));
#else
	return boost::shared_ptr<GPlatesQtWidgets::SaveFileDialog>(
			new OtherSaveFileDialog(
				parent,
				caption,
				filters,
				default_filter));
#endif
}

GPlatesQtWidgets::WindowsSaveFileDialog::WindowsSaveFileDialog(
		QWidget *parent_,
		const QString &caption,
		const std::vector<std::pair<QString, QString> > &filters,
		unsigned int default_filter) :
	d_parent_ptr(parent_),
	d_caption(caption)
{
	set_filters(filters, default_filter);
}

boost::optional<QString>
GPlatesQtWidgets::WindowsSaveFileDialog::get_file_name()
{
	QString filename = QFileDialog::getSaveFileName(
			d_parent_ptr,
			d_caption,
			d_last_file_name,
			d_filters,
			&d_last_filter);
	
	if (filename.isEmpty())
	{
		return boost::none;
	}

	d_last_file_name = filename;
	d_last_filter = d_filter_map_ext_to_text[get_file_extension(filename)];

	return boost::optional<QString>(filename);
}

void
GPlatesQtWidgets::WindowsSaveFileDialog::set_filters(
		const std::vector<std::pair<QString, QString> > &filters,
		unsigned int default_filter)
{
	d_filter_map_ext_to_text.clear();

	// store the filters in a map for quick reference
	typedef std::vector<std::pair<QString, QString> >::const_iterator iterator_type;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_ext_to_text.insert(std::make_pair(iter->second, iter->first));
	}
	
	// save filters
	d_filters = get_output_filters(filters);
	
	// set default filter
	if (filters.size() > 0 && default_filter < filters.size())
	{
		d_last_filter = filters[default_filter].first;
	}
}

void
GPlatesQtWidgets::WindowsSaveFileDialog::set_filters(
		const std::vector<std::pair<QString, QString> > &filters)
{
	d_filter_map_ext_to_text.clear();

	// store the filters in a map for quick reference
	typedef std::vector<std::pair<QString, QString> >::const_iterator iterator_type;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_ext_to_text.insert(std::make_pair(iter->second, iter->first));
	}
	
	// save filters
	d_filters = get_output_filters(filters);

	// see whether d_last_filter is still in the list of filters
	bool found = false;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		if (iter->first == d_last_filter)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		// use the first filter, or if no filters, use empty string
		if (filters.size() > 0)
		{
			d_last_filter = filters[0].first;
		}
		else
		{
			d_last_filter = QString();
		}
	}
}

void
GPlatesQtWidgets::WindowsSaveFileDialog::select_file(
		const QString &file_path)
{
	d_last_file_name = file_path;
	QString file_ext = get_file_extension(file_path);
	d_last_filter = d_filter_map_ext_to_text[file_ext];
}

GPlatesQtWidgets::OtherSaveFileDialog::OtherSaveFileDialog(
		QWidget *parent_,
		const QString &caption,
		const std::vector<std::pair<QString, QString> > &filters,
		unsigned int default_filter) :
	d_file_dialog_ptr(new QFileDialog(
				parent_,
				caption))
{
	d_file_dialog_ptr->setFileMode(QFileDialog::AnyFile);
	d_file_dialog_ptr->setAcceptMode(QFileDialog::AcceptSave);

	set_filters(filters, default_filter);

	// listen to changes to the filter in the dialog box
	QObject::connect(
			d_file_dialog_ptr.get(),
			SIGNAL(filterSelected(const QString&)),
			this,
			SLOT(handle_filter_changed()));
}

boost::optional<QString>
GPlatesQtWidgets::OtherSaveFileDialog::get_file_name()
{
	if (!d_file_dialog_ptr->exec())
	{
		return boost::none;
	}

	QString filename = d_file_dialog_ptr->selectedFiles().front();

	if (filename.isEmpty())
	{
		return boost::none;
	}

	return boost::optional<QString>(filename);
}

void
GPlatesQtWidgets::OtherSaveFileDialog::handle_filter_changed()
{
	QString filter = d_file_dialog_ptr->selectedFilter();
	QString suffix_string = d_filter_map_text_to_ext[filter];
	d_file_dialog_ptr->setDefaultSuffix(suffix_string);
}

void
GPlatesQtWidgets::OtherSaveFileDialog::set_filters(
		const std::vector<std::pair<QString, QString> > &filters,
		unsigned int default_filter)
{
	// tell the QFileDialog what the filter is
	d_file_dialog_ptr->setFilter(get_output_filters(filters));

	// store the filters in a map for quick reference
	typedef std::vector<std::pair<QString, QString> >::const_iterator iterator_type;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_text_to_ext.insert(std::make_pair(iter->first, iter->second));
	}
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_ext_to_text.insert(std::make_pair(iter->second, iter->first));
	}
	
	// set the default suffix
	if (filters.size() > 0 && default_filter < filters.size())
	{
		d_file_dialog_ptr->setDefaultSuffix(filters[default_filter].second);
	}
}

void
GPlatesQtWidgets::OtherSaveFileDialog::set_filters(
		const std::vector<std::pair<QString, QString> > &filters)
{
	// we need to save the selected filter before we set the new filter
	// because it will change it once we call setFilter()
	QString selected_filter = d_file_dialog_ptr->selectedFilter();

	// tell the QFileDialog what the filter is
	d_file_dialog_ptr->setFilter(get_output_filters(filters));

	// store the filters in a map for quick reference
	typedef std::vector<std::pair<QString, QString> >::const_iterator iterator_type;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_text_to_ext.insert(std::make_pair(iter->first, iter->second));
	}
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		d_filter_map_ext_to_text.insert(std::make_pair(iter->second, iter->first));
	}

	// see if selected filter is still in the list of filters
	bool found = false;
	for (iterator_type iter = filters.begin(); iter != filters.end(); ++iter)
	{
		if (selected_filter == iter->first)
		{
			// make sure it is still selected
			d_file_dialog_ptr->selectFilter(selected_filter);
			d_file_dialog_ptr->setDefaultSuffix(iter->second);

			found = true;
			break;
		}
	}
	if (!found)
	{
		// use the first filter, or if no filters, use empty string
		if (filters.size() > 0)
		{
			d_file_dialog_ptr->selectFilter(filters[0].first);
			d_file_dialog_ptr->setDefaultSuffix(filters[0].second);
		}
		else
		{
			d_file_dialog_ptr->selectFilter(QString());
			d_file_dialog_ptr->setDefaultSuffix(QString());
		}
	}
}

void
GPlatesQtWidgets::OtherSaveFileDialog::select_file(
		const QString &file_path)
{
	d_file_dialog_ptr->selectFile(file_path);
	QString file_ext = get_file_extension(file_path);
	d_file_dialog_ptr->selectFilter(d_filter_map_ext_to_text[file_ext]);
	d_file_dialog_ptr->setDefaultSuffix(file_ext);
}

