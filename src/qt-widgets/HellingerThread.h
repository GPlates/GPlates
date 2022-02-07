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

#include <QThread>

namespace GPlatesQtWidgets
{
	enum ThreadType
	{
		TWO_WAY_POLE_THREAD_TYPE = 0,
		THREE_WAY_POLE_THREAD_TYPE,
		TWO_WAY_UNCERTAINTY_THREAD_TYPE,
		THREE_WAY_UNCERTAINTY_THREAD_TYPE,

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

		QString
		temp_pick_filename() const;

		QString
		temp_result_filename() const;

		QString
		temp_par_filename() const;

		QString
		path() const{
			return d_output_path;
		}

		bool
		thread_failed() const
		{
			return d_thread_failed;
		}

		void
		initialise(const QString &python_file,
				const QString &output_path,
				const QString &results_filename_root,
				const QString &temporary_path);

		void
		set_python_script_type(ThreadType thread_type);


	private:

		void
		calculate_two_way_fit();

		void
		calculate_three_way_fit();

		void
		calculate_two_way_uncertainties();

		void
		calculate_three_way_uncertainties();

		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;

		ThreadType d_thread_type;



		/**
		 * @brief d_python_file - the main hellinger python file (hellinger.py) including the path.
		 */
		QString d_python_file;

		/**
		 * @brief d_output_path - path for outputting results
		 */
		QString d_output_path;

		QString d_results_filename_root;;

		/**
		 * @brief d_path_for_temporary_files - Data are communicated to and from the python scripts by
		 * file - these are stored in the location given by @a d_path_for_temporary_files.
		 */
		QString d_path_for_temporary_files;

		// Various temporary files for communication with python.
		QString d_temp_pick_file;
		QString d_temp_result_filename;
		QString d_temp_par;
		QString d_temp_res;

		bool d_thread_failed;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERTHREAD_H
