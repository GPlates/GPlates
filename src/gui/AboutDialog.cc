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

#include <sstream>
#include <wx/wx.h>
#include <wx/datetime.h>
#include "AboutDialog.h"
#include "global/config.h"


BEGIN_EVENT_TABLE(GPlatesGui::AboutDialog, wxDialog)
EVT_BUTTON(42, GPlatesGui::AboutDialog::okClick)
END_EVENT_TABLE()

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
	// TODO
	//_msizer->Add (new wxButton (this, 42, "", wxDefaultPosition,
	//		wxDefaultSize, wxBU_EXACTFIT), 1, wxALIGN_RIGHT, 0);

	wxBoxSizer *extSizer = new wxBoxSizer (wxHORIZONTAL);
	extSizer->Add (_msizer, 0, wxALL, BORDER_SIZE);
	extSizer->SetSizeHints (this);
	SetSizer (extSizer);
}

/// \hideinitializer
static const char *c_strings[] = {
	"All Your Plates Are Belong To Us.",
	"Bringing You Back To The Present.",
	"Telling The Fortune Of The World... Backwards.",
	"Sit On This And Rotate.",
	"Simulating Geological Processes In Geological Time.",
	"Plate Tectonics -- Now Almost As Fast As The Real Thing!",
	"In Soviet Russia, The Plates Rotate You!"
};

void GPlatesGui::AboutDialog::okClick (wxCommandEvent &event)
{
	time_t now = wxDateTime::Now ().GetTicks ();
	int idx = now % (sizeof (c_strings) / sizeof (c_strings[0]));
	std::ostringstream ss;

	ss << PACKAGE_NAME << ": " << c_strings[idx];
	_top->SetLabel (wxString (ss.str ().c_str ()));
	_msizer->Layout ();
}
