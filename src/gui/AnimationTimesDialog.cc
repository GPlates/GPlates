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
#include "AnimationTimesDialog.h"

using namespace GPlatesGui;

AnimationTimesDialog::AnimationTimesDialog(wxWindow* parent)
	: wxDialog(parent, -1, _("Constructing Animation..."))
{
	static const int BORDER_SIZE  = 10;
	
	wxBoxSizer* msgsizer = new wxBoxSizer(wxHORIZONTAL);
	msgsizer->Add(new wxStaticText(this, -1,
		_("Enter the start and end times between which you\n"
		"wish the reconstruction to take place in units\n"
		"of millions of years ago.  Additionaly, specify\n"
		"the number of steps to take for the animation.\n"
		"(More steps -> better quality, but slower.)\n")), 
		0, wxALL, BORDER_SIZE);

	// A text entry thingo with a text note to the left
	wxBoxSizer* entrysizer1 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer1->Add(new wxStaticText(this, -1, _("Enter start time: (Ma)")), 
					0, wxALL, BORDER_SIZE);
	entrysizer1->Add(_startctrl = new wxTextCtrl(this, -1, _("0.0")), 
					0, wxALL, BORDER_SIZE);  
							// This needs a validator

	wxBoxSizer* entrysizer2 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer2->Add(new wxStaticText(this, -1, _("Enter end time: (Ma)")), 
					0, wxALL, BORDER_SIZE);
	entrysizer2->Add(_endctrl = new wxTextCtrl(this, -1, _("0.0")), 
					0, wxALL, BORDER_SIZE);  
							// This needs a validator

	wxBoxSizer* entrysizer3 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer3->Add(new wxStaticText(this, -1, _("Enter number of steps: ")), 
					0, wxALL, BORDER_SIZE);
	entrysizer3->Add(_nstepsctrl = new wxTextCtrl(this, -1, _("0")), 
					0, wxALL, BORDER_SIZE);  
							// This needs a validator

	wxBoxSizer* buttonsizer = new wxBoxSizer(wxHORIZONTAL);
	buttonsizer->Add(new wxButton(this, wxID_OK, _("OK")),
					 1, wxALL, BORDER_SIZE);
	buttonsizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), 
					 1, wxALL, BORDER_SIZE);
	
	wxBoxSizer* mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->Add(msgsizer, 0);
	mainsizer->Add(entrysizer1, 0);
	mainsizer->Add(entrysizer2, 0);
	mainsizer->Add(entrysizer3, 0);
	mainsizer->Add(buttonsizer, 0);

	mainsizer->SetSizeHints(this);
	SetSizer(mainsizer);
}


GPlatesGlobal::fpdata_t
AnimationTimesDialog::GetStartTime() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_startctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}

GPlatesGlobal::fpdata_t
AnimationTimesDialog::GetEndTime() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_endctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}

GPlatesGlobal::integer_t
AnimationTimesDialog::GetNSteps() const
{
	GPlatesGlobal::integer_t res;
	
	std::istringstream iss(std::string(_nstepsctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}
