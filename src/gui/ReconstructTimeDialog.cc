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
using namespace GPlatesMaths;

class TimeValidator : public wxValidator
{
	public:
		TimeValidator(real_t& time) 
			: wxValidator(), _time(time) {  }
	
		virtual bool
		TransferFromWindow() { return true; }

	private:
		real_t& _time;
};

ReconstructTimeDialog::ReconstructTimeDialog(wxWindow* parent)
	: wxDialog(parent, -1, "Reconstruct to...")
{
	static const int ALLOW_RESIZE = 1;
	static const int DISALLOW_RESIZE = 0;
	static const int BORDER_SIZE  = 10;
	
	// A text entry thingo with a text note to the left
	wxBoxSizer* entrysizer = new wxBoxSizer(wxHORIZONTAL);
	entrysizer->Add(new wxStaticText(this, -1, "Enter time: (Ma)"), 
					ALLOW_RESIZE, wxALIGN_LEFT | wxALL, BORDER_SIZE);
	entrysizer->Add(_txtctrl = new wxTextCtrl(this, -1, "0.0"), 
					ALLOW_RESIZE, wxALIGN_RIGHT | wxALL, BORDER_SIZE);  
							// This needs a validator
	
	wxBoxSizer* buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsizer->Add(new wxButton(this, wxID_OK, "OK"),
					 DISALLOW_RESIZE, wxALIGN_LEFT | wxALL, BORDER_SIZE);
	buttonsizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 
					 DISALLOW_RESIZE, wxALIGN_RIGHT | wxALL, BORDER_SIZE);
	
	wxBoxSizer* mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->Add(entrysizer, 1, wxALIGN_TOP | wxALIGN_CENTRE_HORIZONTAL);
	mainsizer->Add(buttonsizer, 1, wxALIGN_BOTTOM | wxALIGN_CENTRE_HORIZONTAL);

	SetSizer(mainsizer);
	mainsizer->SetSizeHints(this);
}


GPlatesMaths::real_t
ReconstructTimeDialog::GetInput() const
{
	GPlatesMaths::real_t res;
	
	std::istringstream iss(_txtctrl->GetValue().c_str());
	iss >> res;

	// FIXME: check for error

	return res;
}
