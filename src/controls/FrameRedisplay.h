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

#ifndef _GPLATES_CONTROLS_VIEW_FRAMEREDISPLAY_H_
#define _GPLATES_CONTROLS_VIEW_FRAMEREDISPLAY_H_

#include <wx/wx.h>

namespace GPlatesControls
{
	namespace View
	{
		class FrameRedisplay
		{
			public:
				FrameRedisplay(wxFrame* frame = NULL) 
					: _frame(frame) {  }

				void
				SetFrame(wxFrame* frame) { _frame = frame; }

				void operator()() { 
					wxPaintEvent evt;
					_frame->AddPendingEvent(evt);
				}
			
			private:
				wxFrame* _frame;
		};
	}
}

#endif  /* _GPLATES_CONTROLS_VIEW_FRAMEREDISPLAY_H_ */
