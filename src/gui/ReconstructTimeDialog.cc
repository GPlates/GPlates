/* Id: ReconstructTimeDialog.cc,v 1.7 2004/04/20 23:32:24 hlaw Exp $ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <sstream>
#include <wx/wx.h>
#include "ReconstructTimeDialog.h"
#include "FPValidator.h"

GPlatesGui::ReconstructTimeDialog::ReconstructTimeDialog(wxWindow* parent) :
 wxDialog(parent, -1, wxString(_("Reconstruct to..."))), _time_ctrl_str("0.0")
{
	static const int BORDER_SIZE  = 10;
	
	wxBoxSizer* msgsizer = new wxBoxSizer(wxHORIZONTAL);
	msgsizer->Add(new wxStaticText(this, -1,
		_("Enter the time for the reconstruction\n"
		 "(in units of \"millions of years ago\").\n")), 
		0, wxALL, BORDER_SIZE);

	// A text entry thingo with a text note to the left
	wxBoxSizer* entrysizer = new wxBoxSizer(wxHORIZONTAL);
	entrysizer->Add(new wxStaticText(this, -1,
	 _("Enter time: (Ma)")), 0, wxALL, BORDER_SIZE);
	entrysizer->Add(_time_ctrl = new wxTextCtrl(this, -1,
	 "", wxDefaultPosition, wxDefaultSize, 0,
	 FPValidator(0, &_time_ctrl_str)),
	 0, wxALL, BORDER_SIZE);

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
GPlatesGui::ReconstructTimeDialog::GetTime() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_time_ctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}
