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

#ifndef _GPLATES_GUI_FPVALIDATOR_H_
#define _GPLATES_GUI_FPVALIDATOR_H_

#include <wx/validate.h>
#include <wx/textctrl.h>
#include <wx/string.h>


namespace GPlatesGui
{
	/**
	 * A wxWindows-style validator, used to validate floating-point data.
	 */
	class FPValidator : public wxValidator {

		public:
			// The values in this enumeration will be bitwise ORed.
			enum {

				DISALLOW_NEG = 1,
				DISALLOW_ZERO = 2
			};


			explicit
			FPValidator(long style = 0, wxString *val = NULL) :
			 wxValidator(), _style(style), _val(val) {  }


			FPValidator(const FPValidator &other);


			~FPValidator() {  }


			/**
			 * Return an identical copy of this validator.
			 *
			 * This is necessary because validators are passed
			 * to control constructors as references, which must
			 * be cloned internally.
			 */
			virtual wxObject *
			Clone() const {

				return new FPValidator(*this);
			}

			/**
			 * Transfer the data from the control.
			 *
			 * Return false if there is a problem.
			 */
			virtual bool TransferFromWindow();

			/**
			 * Transfer the data to the control.
			 *
			 * Return false if there is a problem.
			 */
			virtual bool TransferToWindow();

			/**
			 * Called when the value in the associated window
			 * must be validated.
			 *
			 * Return true if the data in the control is valid.
			 * Return false, and pop up an error dialog, if the
			 * data is not valid.
			 */
			virtual bool Validate(wxWindow *parent);

			virtual void OnChar(wxKeyEvent &ev);

			DECLARE_EVENT_TABLE()

		private:
			long _style;
			wxString *_val;

			wxTextCtrl *GetTextCtrl() const;

			bool CheckValidator() const;

			bool CheckValue(wxString &val,
			                wxString &error_msg) const;
	};
}

#endif  /* _GPLATES_GUI_FPVALIDATOR_H_ */
