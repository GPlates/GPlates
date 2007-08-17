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
#include <QtGui/QApplication>
#include "gui-qt/ViewportWindow.h"

int main(int argc, char* argv[])
{
	QApplication application(argc, argv);

	// All the libtool cruft causes the value of 'argv[0]' to be not what the user invoked,
	// so we'll have to hard-code this for now.
	const std::string prog_name = "gplates-demo";

	std::string plates_line_fname;
	std::string plates_rot_fname;

	if (argc >= 3) {
		plates_line_fname = argv[1];
		plates_rot_fname = argv[2];
	} else {
		std::cerr << prog_name << ": missing line and rotation file operands\n\n";
		std::cerr << "Usage: " << prog_name << " PLATES_LINE_FILE PLATES_ROTATION_FILE";
		std::cerr << std::endl;
		std::exit(1);
	}

	GPlatesGui::ViewportWindow viewport_window(plates_line_fname, plates_rot_fname);
	viewport_window.show();
	return application.exec();
}
