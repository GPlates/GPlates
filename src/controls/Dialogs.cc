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

#include <sstream>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include "Dialogs.h"

void
GPlatesControls::Dialogs::ErrorMessage(const char* title, const char* message,
 const char* result)
{
	std::ostringstream msg;
	msg << "Message:" << std::endl
	 << message << std::endl << std::endl
	 << "Result:" << std::endl
	 << result << std::endl;

	// Annoy user:
	wxMessageBox(wxString(msg.str().c_str(), *wxConvCurrent), 
	             wxString(title, *wxConvCurrent),
	             wxOK | wxICON_ERROR);
}

void
GPlatesControls::Dialogs::InfoMessage(const char *title, const char *message)
{
	wxMessageBox(wxString(message, *wxConvCurrent),
	             wxString(title, *wxConvCurrent),
	             wxOK | wxICON_INFORMATION);
}
