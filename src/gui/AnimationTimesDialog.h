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

#ifndef _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_
#define _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_

#include <wx/dialog.h>
#include "maths/types.h"
#include "global/types.h"

namespace GPlatesGui
{
	class AnimationTimesDialog : public wxDialog
	{
		public:
			AnimationTimesDialog(wxWindow* parent);
	
			GPlatesMaths::real_t
			GetStartTime() const;
			
			GPlatesMaths::real_t
			GetEndTime() const;

			GPlatesGlobal::integer_t
			GetNSteps() const;

		private:
			wxTextCtrl* _startctrl;
			wxTextCtrl* _endctrl;
			wxTextCtrl* _nstepsctrl;
	};
}

#endif  /* _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_ */
