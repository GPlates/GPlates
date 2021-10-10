/* $Id$ */

/**
 * @file 
 * Contains the main function of GPlates.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
 
#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <iostream>
#include <string>
#include <utility>
#include <algorithm>
#include <vector>
#include <QStringList>
#include <QTextStream>
#include <QtGui/QApplication>
#include "qt-widgets/ViewportWindow.h"


namespace {

	// This type is a pair of lists: the first a list of line format files,
	// the second is a list of rotation files.
	typedef std::pair<QStringList, QStringList> cmdline_options_type;

	void
	print_usage(const std::string &progname) {

		std::cerr << "Usage: " << progname 
			<< " -r PLATES_ROTATION_FILE_1 -r PLATES_ROTATION_FILE_2 ... "\
				" PLATES_LINE_FILE_1 PLATES_LINE_FILE_2 ..."
		   	<< std::endl;
	}

	void
	print_usage_and_exit(const std::string &progname) {

		print_usage(progname);
		exit(1);
	}

	cmdline_options_type
	process_command_line_options(int argc, char *argv[], 
			const std::string &progname) {

		static const QString ROTATION_FILE_OPTION = "-r";

		QStringList cmdline;
		std::copy(&argv[1], &argv[argc], std::back_inserter(cmdline));

		cmdline_options_type res;

		QStringList::iterator iter = cmdline.begin();
		for ( ; iter != cmdline.end(); ++iter) {
			if (*iter == ROTATION_FILE_OPTION) {
				if (++iter != cmdline.end()) {
					// Next argument after the ROTATION_FILE_OPTION should be
					// the rotation file name.
					res.second.push_back(*iter);
				} else {
					// We got the ROTATION_FILE_OPTION but there's no associated
					// rotation file.
					break;
				}
			} else {
				// We didn't get the ROTATION_FILE_OPTION so this command line
				// argument must be a plates line format file.
				res.first.push_back(*iter);
			}
		}

		// N.B. We allow the situtation where no rotation files have been specified.

		return res;
	}
}


int main(int argc, char* argv[])
{
	QApplication application(argc, argv);

	// All the libtool cruft causes the value of 'argv[0]' to be not what the user invoked,
	// so we'll have to hard-code this for now.
	const std::string prog_name = "gplates-demo";

	cmdline_options_type cmdline = process_command_line_options(
			application.argc(), application.argv(), prog_name);

	GPlatesQtWidgets::ViewportWindow viewport_window;

	viewport_window.show();
	viewport_window.load_files(cmdline.first + cmdline.second);
	viewport_window.reconstruct_to_time_with_root(0.0, 0);

	return application.exec();
}
