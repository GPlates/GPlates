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

#ifndef _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_
#define _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_

#include <wx/dialog.h>
#include "global/types.h"

namespace GPlatesGui
{
	class ReconstructTimeDialog : public wxDialog
	{
		public:
			ReconstructTimeDialog(wxWindow* parent);
	
			GPlatesGlobal::fpdata_t
			GetTime() const;

		private:
			wxTextCtrl* _time_ctrl;

			wxString _time_ctrl_str;
	};
}

#endif  /* _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_ */
