/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <iostream>

#include "global/config.h"
#include "global/Exception.h"

#include "GPlatesApp.h"
#include "GLFrame.h"

using namespace GPlatesGui;


bool
GPlatesApp::OnInit()
{
	/*
	 * Note that this 'try ... catch' block can only catch exceptions
	 * thrown during the instantiation of the new GLFrame.  It CANNOT
	 * catch exceptions thrown at any later stage.
	 */
	try {
		/*
		 * Note that '_(str)' is a gettext-style macro alias for
		 * 'wxGetTranslation(str)'.
		 */
		GLFrame* frame =
		 new GLFrame(NULL, _(PACKAGE_STRING), wxSize(640, 640));
		frame->Show(TRUE);
		SetTopWindow(frame);

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr
		 << "During GPlates-init phase (GPlatesApp::OnInit), "
		 << "caught GPlates exception:\n"
		 << e
		 << std::endl;
		return FALSE;

	} catch (const std::exception &e) {

		std::cerr
		 << "During GPlates-init phase (GPlatesApp::OnInit), "
		 << "caught standard exception:\n"
		 << e.what()
		 << std::endl;
		return FALSE;

	} catch (...) {

		std::cerr
		 << "During GPlates-init phase (GPlatesApp::OnInit), "
		 << "caught unrecognised exception."
		 << std::endl;
		return FALSE;
	}
	return TRUE;
}
