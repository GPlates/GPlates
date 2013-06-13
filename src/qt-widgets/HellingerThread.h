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

#include <QWidget>
#include <QThread>

namespace GPlatesQtWidgets
{
	enum ThreadType
	{
		POLE_THREAD_TYPE = 0,
		STATS_THREAD_TYPE,

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
		initialization_pole_part(QString &import_file_path, std::vector<double> &input_data, std::vector<int> &bool_data, int &iteration, QString &python_file, QString &temporary_folder, QString &temp_pick_file, QString &temp_result, QString &temp_par, QString &temp_res);
		void
		initialization_stats_part(QString &path_file, QString &file_name, QString &filename_dat, QString &filename_up, QString &filename_do, QString &python_file, QString &temporary_folder, QString &temp_pick_file, QString &temp_result, QString &temp_par, QString &temp_res);
		void
		set_python_script(ThreadType thread_type);


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
