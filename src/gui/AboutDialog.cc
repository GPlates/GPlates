/* $Id$ */

/**
 * @file 
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
#include <sstream>
#include <wx/wx.h>
#include <wx/datetime.h>
#include "AboutDialog.h"

// This is a temporary fix (just while we're still using wxWidgets) because <wx/wx.h> includes
// <wx/defs.h>, which includes <wx/platform.h>, which defines HAVE_BOOL and HAVE_EXPLICIT, which
// our own configuration file "global/config.h" (whose inclusion immediately follows) also defines.
// We're not undefining these macros because we're concerned about the values being different, but
// because the compiler is complaining about the macros being redefined.
#ifdef HAVE_BOOL
#undef HAVE_BOOL
#endif  // HAVE_BOOL
#ifdef HAVE_EXPLICIT
#undef HAVE_EXPLICIT
#endif  // HAVE_EXPLICIT

#include "global/config.h"


GPlatesGui::AboutDialog::AboutDialog(wxWindow* parent)
	: wxDialog(parent, -1, wxString(_("About GPlates...")))
{
	static const int BORDER_SIZE = 10;
	
	_msizer = new wxBoxSizer (wxVERTICAL);
	_msizer->Add (new wxStaticText (this, -1, wxString (PACKAGE_STRING)),
					0, wxALIGN_CENTER, BORDER_SIZE);
	wxString msg;
	msg = COPYRIGHT_STRING "\n\n";
	msg += _(
	    "This program is free software; you can redistribute it and/or\n"
	    "modify it under the terms of the GNU General Public License,\n"
	    "version 2, as published by the Free Software Foundation.\n"
	    "\n"
	    "This program is distributed in the hope that it will be useful,\n"
	    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	    "See the GNU General Public License for more details."
	);

	_top = new wxStaticText (this, -1, msg);
	_msizer->Add (_top, 0, wxALL, BORDER_SIZE);

	_msizer->Add (new wxButton (this, wxID_OK, _("OK")),
					 1, wxALIGN_CENTER, BORDER_SIZE);
	wxBoxSizer *extSizer = new wxBoxSizer (wxHORIZONTAL);
	extSizer->Add (_msizer, 0, wxALL, BORDER_SIZE);
	extSizer->SetSizeHints (this);
	SetSizer (extSizer);
}
