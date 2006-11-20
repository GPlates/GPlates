/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include <cstdlib>
#include <iostream>
#include "Lifetime.h"
#include "global/AlreadyInitialisedSingletonException.h"
#include "global/NullParameterException.h"
#include "global/UninitialisedSingletonException.h"


GPlatesControls::Lifetime *
GPlatesControls::Lifetime::_instance = NULL;

bool
GPlatesControls::Lifetime::_is_initialised = false;

GPlatesGui::MainWindow *
GPlatesControls::Lifetime::_main_win = NULL;


void
GPlatesControls::Lifetime::init(GPlatesGui::MainWindow *main_win) {

	if (_is_initialised) {

		throw GPlatesGlobal::AlreadyInitialisedSingletonException(
		 "Attempted to initialise GPlatesControls::Lifetime again.");
	}
	if (main_win == NULL) {

		throw GPlatesGlobal::NullParameterException(
		 "Attempted to initialise GPlatesControls::Lifetime with a NULL MainWindow.");
	}
	_main_win = main_win;
	_is_initialised = true;
}


GPlatesControls::Lifetime *
GPlatesControls::Lifetime::instance() {

	if ( ! _is_initialised) {

		throw GPlatesGlobal::UninitialisedSingletonException(
		 "Attempted to instantiate GPlatesControls::Lifetime uninitialised.");
	}
	if (_instance == NULL) {

		_instance = new Lifetime();
	}
	return _instance;
}


void
GPlatesControls::Lifetime::terminate(const std::string &reason) {

	std::cerr << "Terminating program: " << reason << "." << std::endl;

	// FIXME: Offer to save work, if appropriate.

	/*
	 * This (deleting the top window) is the "correct" way to end the
	 * program, according to the reference documentation for the function
	 * 'wxApp::ExitMainLoop'.
	 */
	delete _main_win;

	std::exit(1);  // FIXME: is this necessary?
}
