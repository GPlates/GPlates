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
 *
 */

#include <cstdlib>
#include <sstream>
#include <wx/wx.h>
#include <wx/datetime.h>
#include "AboutDialog.h"
#include "global/config.h"


GPlatesGui::AboutDialog::AboutDialog(wxWindow* parent)
	: wxDialog(parent, -1, _("About GPlates..."))
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
