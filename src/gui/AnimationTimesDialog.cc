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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <sstream>
#include <iomanip>
#include <wx/wx.h>
#include "AnimationTimesDialog.h"
#include "FPValidator.h"

GPlatesGui::AnimationTimesDialog::AnimationTimesDialog(wxWindow* parent,
 GPlatesGlobal::fpdata_t start_time, GPlatesGlobal::fpdata_t end_time,
 GPlatesGlobal::fpdata_t time_delta, bool finish_on_end) :

 wxDialog(parent, -1, _("Constructing Animation...")),
 _start_ctrl_str(AnimationTimesDialog::fpToWxString(start_time)),
 _end_ctrl_str(AnimationTimesDialog::fpToWxString(end_time)),
 _time_delta_ctrl_str(AnimationTimesDialog::fpToWxString(time_delta))
{
	static const int BORDER_SIZE  = 10;
	
	// FIXME: are all these 'new's going to be deleted?
	wxBoxSizer* msgsizer = new wxBoxSizer(wxHORIZONTAL);
	msgsizer->Add(new wxStaticText(this, -1,
		_("Enter the start and end times of the animation\n"
		 "(in units of \"millions of years ago\").\n\n"
		 "Additionally, you may vary the time-delta\n"
		 "between successive frames of the animation\n"
		 "(in units of \"millions of years\").\n\n"
		 "The time-delta must be greater than zero.\n")),
		0, wxALL, BORDER_SIZE);

	// A text entry thingo with a text note to the left
	wxBoxSizer* entrysizer1 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer1->Add(new wxStaticText(this, -1,
	 _("Enter start time: (Ma)")), 0, wxALL, BORDER_SIZE);
	entrysizer1->Add(_start_ctrl = new wxTextCtrl(this, -1,
	 "", wxDefaultPosition, wxDefaultSize, 0,
	 FPValidator(0, &_start_ctrl_str)),
	 0, wxALL, BORDER_SIZE);

	wxBoxSizer* entrysizer2 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer2->Add(new wxStaticText(this, -1,
	 _("Enter end time: (Ma)")), 0, wxALL, BORDER_SIZE);
	entrysizer2->Add(_end_ctrl = new wxTextCtrl(this, -1,
	 "", wxDefaultPosition, wxDefaultSize, 0,
	 FPValidator(0, &_end_ctrl_str)),
	 0, wxALL, BORDER_SIZE);

	wxBoxSizer* entrysizer3 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer3->Add(new wxStaticText(this, -1,
	 _("Enter time-delta: (M)")), 0, wxALL, BORDER_SIZE);
	entrysizer3->Add(_time_delta_ctrl = new wxTextCtrl(this, -1,
	 "", wxDefaultPosition, wxDefaultSize, 0,
	 FPValidator(FPValidator::DISALLOW_NEG | FPValidator::DISALLOW_ZERO,
	  &_time_delta_ctrl_str)),
	 0, wxALL, BORDER_SIZE);

	wxBoxSizer* entrysizer4 = new wxBoxSizer(wxHORIZONTAL);
	entrysizer4->Add(_finish_on_end = new wxCheckBox(this, -1,
	 _("Finish animation exactly on end time.")), 0, wxALL, BORDER_SIZE);
	_finish_on_end->SetValue(finish_on_end);

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
	mainsizer->Add(entrysizer4, 0);
	mainsizer->Add(buttonsizer, 0);

	mainsizer->SetSizeHints(this);
	SetSizer(mainsizer);
}


GPlatesGlobal::fpdata_t
GPlatesGui::AnimationTimesDialog::GetStartTime() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_start_ctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}


GPlatesGlobal::fpdata_t
GPlatesGui::AnimationTimesDialog::GetEndTime() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream iss(std::string(_end_ctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}


GPlatesGlobal::fpdata_t
GPlatesGui::AnimationTimesDialog::GetTimeDelta() const
{
	GPlatesGlobal::fpdata_t res;
	
	std::istringstream
	 iss(std::string(_time_delta_ctrl->GetValue().mb_str()));
	iss >> res;

	// FIXME: check for error

	return res;
}


bool
GPlatesGui::AnimationTimesDialog::GetFinishOnEnd() const
{
	return _finish_on_end->IsChecked();
}


wxString
GPlatesGui::AnimationTimesDialog::fpToWxString(GPlatesGlobal::fpdata_t f)
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(1) << f.dval();
	return wxString(oss.str().c_str());
}
