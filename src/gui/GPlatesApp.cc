/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#include <iostream>
#include <wx/wx.h>

#include "global/config.h"
#include "global/Exception.h"

#include "GPlatesApp.h"
#include "MainWindow.h"
#include "controls/Lifetime.h"

namespace {

	/*
	 * Set the MESA_NO_SSE environment variable to 1 to circumvent a Mesa 
	 * bug described at
	 * 
	 *  http://www.geosci.usyd.edu.au/users/hlaw/pmwiki/pmwiki.php?pagename=Main.NurbsBug
	 * 
	 * This will have no effect on systems that do not have this bug, nor 
	 * on systems that do not use Mesa at all.
	 */
	void
	fix_mesa_bug() {

		wxString var(_("MESA_NO_SSE"), *wxConvCurrent);
		wxString val(_("1"), *wxConvCurrent);
		wxSetEnv(var, val);
	}
}

bool
GPlatesGui::GPlatesApp::OnInit() {

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

	fix_mesa_bug();

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
		d_main_win =
		 new MainWindow(NULL, _(PACKAGE_STRING), wxSize (600, 500));
		d_main_win->Show(TRUE);
		SetTopWindow(d_main_win);
		GPlatesControls::Lifetime::init(d_main_win);

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

int
GPlatesGui::GPlatesApp::OnExit() {

	std::cerr << "In GPlatesGui::GPlatesApp::OnExit" << std::endl; 

	delete d_main_win;

	std::cerr
	 << "Just deleted Main Window in GPlatesGui::GPlatesApp::OnExit"
	 << std::endl;

	return 0;
}
