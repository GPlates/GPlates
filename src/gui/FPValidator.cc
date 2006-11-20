/* $Id$ */

/**
 * @file 
 * A great deal of the code in this file was either based upon,
 * or copied directly from, the files "include/wx/valtext.h" and
 * "src/common/valtext.cpp" in the 'wxGTK' wxWindows release.
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

#include <iostream>
#include <cstdlib>
#include <cctype>
#include <wx/defs.h>  /* wxCHECK_MSG */
#include <wx/intl.h>  /* _ */
#include <wx/msgdlg.h>
#include "FPValidator.h"


GPlatesGui::FPValidator::FPValidator(const FPValidator &other) :
 wxValidator(), _style(other._style), _val(other._val) {

	wxValidator::Copy(other);
}


bool
GPlatesGui::FPValidator::TransferFromWindow() {

	wxTextCtrl *ctrl = GetTextCtrl();
	if (ctrl == NULL) return false;

	(*_val) = ctrl->GetValue();
	return true;
}


bool
GPlatesGui::FPValidator::TransferToWindow() {

	wxTextCtrl *ctrl = GetTextCtrl();
	if (ctrl == NULL) return false;

	ctrl->SetValue(*_val);
	return true;
}


bool
GPlatesGui::FPValidator::Validate(wxWindow *parent) {

	wxTextCtrl *ctrl = GetTextCtrl();
	if (ctrl == NULL) return false;

	// If window is disabled, simply return
	if ( ! ctrl->IsEnabled()) return true;

	wxString val(ctrl->GetValue());
	wxString error_msg;
	if ( ! CheckValue(val, error_msg)) {

		// invalid string
		wxMessageBox(error_msg, _("Invalid field"),
		             wxOK | wxICON_EXCLAMATION, parent);
		return false;
	}
	return true;
}


void
GPlatesGui::FPValidator::OnChar(wxKeyEvent &ev) {

	if (m_validatorWindow) {

		int key_code = (int)ev.KeyCode();

		// Don't filter special keys and Delete
		if ( ! (key_code < WXK_SPACE ||
		        key_code == WXK_DELETE ||
		        key_code > WXK_START) &&
		     ! (isdigit(key_code) ||
		        key_code == '.' ||
		        key_code == ',' ||
		        key_code == '-')) {

			// invalid character -- complain and return
			if ( ! wxValidator::IsSilent()) wxBell();
			return;
		}
	}
	ev.Skip();
}


namespace {

	inline bool
	isDecimalPlace(char c) {

		// Allow ',' for Europe.
		return ((c == '.') || (c == ','));
	}
}


bool
GPlatesGui::FPValidator::CheckValue(wxString &val, wxString &error_msg)
 const {

	size_t len = val.Len();
	if (len == 0) {

		error_msg =
		 _("The field is empty.\n"
		  "A valid number is required.");
		return false;
	}

	// The index of the current character
	size_t i = 0;

	// Allow field to begin with '-' in *some* cases.
	if (val.GetChar(0) == '-') {

		if (len == 1) {

			// Field is empty except for '-'.
			error_msg =
			 _("'-' is not a valid number.\n"
			  "A valid number is required.");
			return false;
		}

		// else, field contains '-' followed by other stuff.
		if (_style & DISALLOW_NEG) {

			// Negative numbers are not allowed.
			wxString fmt =
			 _("'%s' appears to be negative,\n"
			 "which is invalid for this field.");
			error_msg.Printf(fmt, val.c_str());
			return false;

		} else {

			// Negative numbers are allowed.
			i++;
		}
	}

	// Whether a decimal place has yet been encountered.
	bool have_read_dp = false;

	// Whether a digit has yet been encountered.
	bool have_read_digit = false;

	// Whether a non-zero digit has yet been encountered.
	bool have_read_nz_digit = false;

	for ( ; i < len; i++) {

		char c = val.GetChar(i);
		if (isDecimalPlace(c) && ( ! have_read_dp)) {

			have_read_dp = true;
			continue;
		}
		if (std::isdigit(c)) {

			have_read_digit = true;
			if (c != '0') have_read_nz_digit = true;
			continue;
		}

		// else, contents of field are invalid
		wxString fmt = 
		 _("'%s' is not a valid number.\n"
		  "A valid number is required.");
		error_msg.Printf(fmt, val.c_str());
		return false;
	}

	// Whole string has been processed.
	if ( ! have_read_digit) {

		// Processed whole string, but found no digits.
		wxString fmt =
		 _("'%s' is not a valid number.\n"
		  "A valid number is required.");
		error_msg.Printf(fmt, val.c_str());
		return false;
	}
	if ((_style & DISALLOW_ZERO) && ( ! have_read_nz_digit)) {

		// Processed whole string, but found no non-zero digits.
		// Zero is not allowed.
		wxString fmt =
		 _("'%s' appears to be zero,\n"
		 "which is invalid for this field.");
		error_msg.Printf(fmt, val.c_str());
		return false;
	}

	return true;
}


wxTextCtrl *
GPlatesGui::FPValidator::GetTextCtrl() const {

	if ( ! CheckValidator()) {

		// Validator unable to operate properly.
		return NULL;
	}

	wxTextCtrl *ctrl = dynamic_cast< wxTextCtrl * >(m_validatorWindow);
	if (ctrl == NULL) {

		// CheckValidator should already have *checked* this...
		std::cerr << "Internal error in function "
		 << "'GPlatesGui::FPValidator::TransferFromWindow':\n"
		 << "dynamic_cast< wxTextCtrl * > failed!" << std::endl;
		std::exit(1);
	}
	return ctrl;
}


bool
GPlatesGui::FPValidator::CheckValidator() const {

	wxCHECK_MSG( m_validatorWindow, false,
	             _("No window associated with validator") );
	wxCHECK_MSG( m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)), false,
	             _("FPValidator is only for wxTextCtrls") );
	wxCHECK_MSG( _val, false,
	             _("No variable storage for validator") );

	return true;
}


BEGIN_EVENT_TABLE(GPlatesGui::FPValidator, wxValidator)
	EVT_CHAR(GPlatesGui::FPValidator::OnChar)
END_EVENT_TABLE()
