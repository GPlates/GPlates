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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <wx/wx.h>
#include "AboutDialog.h"
#include "global/config.h"

using namespace GPlatesGui;

AboutDialog::AboutDialog(wxWindow* parent)
	: wxDialog(parent, -1, _("About GPlates..."))
{
	static const int BORDER_SIZE = 10;
	
	wxBoxSizer* sizer = new wxBoxSizer (wxVERTICAL);
	sizer->Add (new wxStaticText (this, -1, wxString (PACKAGE_STRING)),
		0, wxALIGN_CENTER, BORDER_SIZE);
	wxString msg;
	msg = _(
	    "Copyright (C) 2003 The GPlates Consortium\n"
	    "\n"
	    "This program is free software; you can redistribute it and/or\n"
	    "modify it under the terms of the GNU General Public License,\n"
	    "version 2, as published by the Free Software Foundation.\n"
	    "\n"
	    "This program is distributed in the hope that it will be useful,\n"
	    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	    "See the GNU General Public License for more details."
	);
		
	sizer->Add (new wxStaticText (this, -1, msg), 0, wxALL, BORDER_SIZE);

	sizer->Add (new wxButton (this, wxID_OK, _("OK")),
					 1, wxALIGN_CENTER, BORDER_SIZE);

	wxBoxSizer *extSizer = new wxBoxSizer (wxHORIZONTAL);
	extSizer->Add (sizer, 0, wxALL, BORDER_SIZE);
	extSizer->SetSizeHints (this);
	SetSizer (extSizer);
}
