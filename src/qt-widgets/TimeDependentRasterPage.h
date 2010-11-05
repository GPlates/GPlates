/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_TIMEDEPENDENTRASTERPAGE_H
#define GPLATES_QTWIDGETS_TIMEDEPENDENTRASTERPAGE_H

#include <map>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QWizardPage>
#include <QFileInfo>
#include <QValidator>

#include "TimeDependentRasterPageUi.h"


namespace GPlatesQtWidgets
{
	// Forward declaration.
	class TimeDependentRasterSequence;

	class TimeDependentRasterPage: 
			public QWizardPage,
			protected Ui_TimeDependentRasterPage
	{
		Q_OBJECT
		
	public:

		/**
		 * Assists with finding out which editor is editing which index.
		 */
		typedef std::map<QModelIndex, QWidget *> index_to_editor_map_type;

		explicit
		TimeDependentRasterPage(
				QString &open_file_path,
				TimeDependentRasterSequence &raster_sequence,
				const boost::function<void (unsigned int)> &set_number_of_bands_function,
				QWidget *parent_ = NULL);

		virtual
		bool
		isComplete() const;

	private slots:

		void
		handle_add_directory_button_clicked();

		void
		handle_add_files_button_clicked();

		void
		handle_remove_selected_button_clicked();

		void
		handle_sort_by_time_button_clicked();

		void
		handle_sort_by_file_name_button_clicked();

		void
		handle_show_full_paths_button_toggled(
				bool checked);

		void
		handle_table_selection_changed();

		void
		handle_table_cell_changed(
				int row,
				int column);

	private:

		void
		make_signal_slot_connections();

		void
		check_if_complete();

		void
		populate_table();

		void
		remove_selected_from_table();

		void
		add_files_to_sequence(
				const QFileInfoList &file_infos);

		static
		boost::optional<double>
		deduce_time(
				const QFileInfo &file_info);

		QString &d_open_file_path;
		TimeDependentRasterSequence &d_raster_sequence;
		boost::function<void (unsigned int)> d_set_number_of_bands_function;

		QValidator *d_validator;

		bool d_is_complete;
		bool d_show_full_paths;

		boost::shared_ptr<index_to_editor_map_type> d_index_to_editor_map;
		QWidget *d_widget_to_focus;
	};
}

#endif  // GPLATES_QTWIDGETS_TIMEDEPENDENTRASTERPAGE_H
