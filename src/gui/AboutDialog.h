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

#ifndef _GPLATES_GUI_ABOUTDIALOG_H_
#define _GPLATES_GUI_ABOUTDIALOG_H_

#include <wx/dialog.h>
#include <wx/event.h>
#include "global/types.h"

// forward declarations
class wxSizer;
class wxStaticText;

namespace GPlatesGui
{
	class AboutDialog : public wxDialog
	{
		public:
			explicit
			AboutDialog (wxWindow *parent);

		private:
			wxSizer *_msizer;
			wxStaticText *_top;
	};
}

#endif  /* _GPLATES_GUI_ABOUTDIALOG_H_ */
