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
 */

#include <sstream>
#include <wx/wx.h>
#include "ReconstructTimeDialog.h"

using namespace GPlatesGui;

ReconstructTimeDialog::ReconstructTimeDialog(wxWindow* parent)
	: wxDialog(parent, -1, _("Reconstruct to..."))
{
	static const int BORDER_SIZE  = 10;
	
	wxBoxSizer* msgsizer = new wxBoxSizer(wxHORIZONTAL);
	msgsizer->Add(new wxStaticText(this, -1,
		_("Enter the time for which you wish the reconstruction\n"
		"to take place in units of millions of years ago.\n")), 
		0, wxALL, BORDER_SIZE);

	// A text entry thingo with a text note to the left
	wxBoxSizer* entrysizer = new wxBoxSizer(wxHORIZONTAL);
	entrysizer->Add(new wxStaticText(this, -1, _("Enter time: (Ma)")), 
					0, wxALL, BORDER_SIZE);
	entrysizer->Add(_txtctrl = new wxTextCtrl(this, -1, _("0.0")), 
					0, wxALL, BORDER_SIZE);  
							// This needs a validator

	wxBoxSizer* buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsizer->Add(new wxButton(this, wxID_OK, _("OK")),
					 1, wxALL, BORDER_SIZE);
	buttonsizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), 
					 1, wxALL, BORDER_SIZE);
	
	wxBoxSizer* mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->Add(msgsizer, 0);
	mainsizer->Add(entrysizer, 0);
	mainsizer->Add(buttonsizer, 0);

	mainsizer->SetSizeHints(this);
	SetSizer(mainsizer);
}


GPlatesGlobal::fpdata_t
ReconstructTimeDialog::GetInput() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_txtctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}
