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
	if (getenv ("EE"))
		_msizer->Add (new wxButton (this, 42, "", wxDefaultPosition,
			wxDefaultSize, wxBU_EXACTFIT), 1, wxALIGN_RIGHT, 0);

	wxBoxSizer *extSizer = new wxBoxSizer (wxHORIZONTAL);
	extSizer->Add (_msizer, 0, wxALL, BORDER_SIZE);
	extSizer->SetSizeHints (this);
	SetSizer (extSizer);
}

/// \hideinitializer
static const char *c_strings[] = {
	"@no$\\irz)Zgmyk|0P`v4Ws{ww};Hr>JS\017",
	"Cpjjboio)Sdy-Lnsz2G{5B\177}9Jiyn{qT\017",
	"Ugohlh`(]bn,Ka}dd|v4Zp7Lq\177;KrlsD\017\014\015\004gGDC^KYH^\000",
	"Rkw$Jh'\\acx,L`k0C}guas9",
	"Rknqigsagm+Khac\177v{puy6Gjvy~on{l\016\017\014\003mK\006`MFFDKDMN\\\021fZYP\030",
	"Qnbp`&Smj~dbdm|0<?3Zza7Yuwtoi>^S\001dBWQ\006f[\011~CI\015|JQ]\022g\\\\XP\031",
	"Hl#Wjpnm}*Yy~}fq=2G|p6Gtxn~o=LpT@VF\004|IR\011"
};

void GPlatesGui::AboutDialog::okClick (wxCommandEvent &event)
{
	time_t now = wxDateTime::Now ().GetTicks ();
	int idx = now % (sizeof (c_strings) / sizeof (c_strings[0]));
	std::ostringstream ss;

	ss << PACKAGE_NAME << ": ";
	for (unsigned char i = 0; i < strlen (c_strings[idx]); ++i)
		ss << (char) (c_strings[idx][i] ^ (i + 1));
	_top->SetLabel (wxString (ss.str ().c_str ()));
	_msizer->Layout ();
}
