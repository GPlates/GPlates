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
#include <wx/wx.h>

#include "global/config.h"
#include "global/Exception.h"

#include "GPlatesApp.h"
#include "MainWindow.h"

using namespace GPlatesGui;


bool
GPlatesApp::OnInit()
{
#ifdef PACKAGE_IS_BETA
	// Warning message for beta versions
	wxMessageDialog dlg (0,
		"WARNING!\n"
		"This is a BETA version of GPlates and is not supported!\n"
		"Please check http://www.gplates.org regularly for updated\n"
		"versions. You have been warned!\n"
		"\n"
		"Given that, do you still want to use it?",
		"WARNING: Beta Version",
		wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION);
	if (dlg.ShowModal () == wxID_NO)
		return FALSE;
#endif

	/*
	 * Note that this 'try ... catch' block can only catch exceptions
	 * thrown during the instantiation of the new MainWindow.
	 * It CANNOT catch exceptions thrown at any later stage.
	 */
	try {
		/*
		 * Note that '_(str)' is a gettext-style macro alias for
		 * 'wxGetTranslation(str)'.
		 */
		MainWindow* main_win =
		 new MainWindow(NULL, _(PACKAGE_STRING), wxSize(640, 640));
		main_win->Show(TRUE);
		SetTopWindow(main_win);

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
