/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_GEOSCIMLPROGRESSDIALOG_H
#define GPLATES_QTWIDGETS_GEOSCIMLPROGRESSDIALOG_H

#include <boost/scoped_ptr.hpp>
#include <QProgressDialog>

#include "file-io/GeoscimlProfile.h"


namespace GPlatesQtWidgets
{
	/**
	 * The @a QProgressDialog shown while @a GPlatesFileIO::GeoscimlProfile translates features.
	 *
	 * Registered with @a GPlatesFileIO::GeoscimlProfile::set_progress_reporter_factory by
	 * @a GPlatesPresentation::register_file_io_injections, since file-io cannot depend on
	 * Qt Widgets.
	 */
	class GeoscimlProgressDialog :
			public GPlatesFileIO::GeoscimlProfile::ProgressReporter
	{
	public:

		GeoscimlProgressDialog();

		virtual
		void
		set_count(
				int count);

		virtual
		bool
		update(
				int index);

	private:

		boost::scoped_ptr<QProgressDialog> d_progress_dialog;
		int d_count;
	};
}

#endif  // GPLATES_QTWIDGETS_GEOSCIMLPROGRESSDIALOG_H
