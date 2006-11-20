/* $Id$ */

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

#ifndef _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_
#define _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_

#include <wx/dialog.h>
#include "global/types.h"

namespace GPlatesGui
{
	class ReconstructTimeDialog : public wxDialog
	{
		public:
			explicit
			ReconstructTimeDialog(wxWindow* parent);
	
			GPlatesGlobal::fpdata_t
			GetTime() const;

		private:
			wxTextCtrl* _time_ctrl;

			wxString _time_ctrl_str;
	};
}

#endif  /* _GPLATES_GUI_RECONSTRUCTTIMEDIALOG_H_ */
