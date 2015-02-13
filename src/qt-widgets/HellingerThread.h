/* $Id: CalculateReconstructionPoleDialog.h 10010 2010-10-26 04:26:05Z elau $ */

/**
 * \file
 * $Revision: 10010 $
 * $Date: 2010-10-26 06:26:05 +0200 (Út, 26 říj 2010) $
 *
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGERTHREAD_H
#define GPLATES_QTWIDGETS_HELLINGERTHREAD_H

#include <vector>

#include <QThread>

namespace GPlatesQtWidgets
{
	enum ThreadType
	{
		POLE_THREAD_TYPE = 0,
		STATS_THREAD_TYPE,
		TESTING,

		NUM_THREAD_TYPES
	};

	class HellingerDialog;
	class HellingerModel;

	class HellingerThread:
			public QThread
	{
		Q_OBJECT
	public:

		HellingerThread(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model);


		void
		run();

		void
		initialise_pole_calculation(
				const QString &import_file_path,
				const std::vector<double> &input_data,
				const std::vector<int> &bool_data,
				const int &iteration,
				const QString &python_file,
				const QString &temporary_folder,
				const QString &temp_pick_file,
				const QString &temp_result,
				const QString &temp_par,
				const QString &temp_res);

		void
		initialise_stats_calculation(
				const QString &path_file,
				const QString &file_name,
				const QString &filename_dat,
				const QString &filename_up,
				const QString &filename_do,
				const QString &python_file,
				const QString &temporary_folder,
				const QString &temp_pick_file,
				const QString &temp_result,
				const QString &temp_par,
				const QString &temp_res);

		void
		set_python_script_type(ThreadType thread_type);


	private:

		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;
		std::vector<double> d_input_data;
		std::vector<int> d_bool_data;
		int d_iteration;
		QString d_import_file_path;
		ThreadType d_thread_type;
		QString d_path;
		QString d_file_name;
		QString d_filename_dat;
		QString d_filename_up;
		QString d_filename_do;
		QString d_python_file;
		QString d_temporary_folder;
		QString d_temp_pick_file;
		QString d_temp_result;
		QString d_temp_par;
		QString d_temp_res;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERTHREAD_H
