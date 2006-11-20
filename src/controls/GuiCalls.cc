/* $Id$ */

/**
 * @file 
 * File specific comments.
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

#include <wx/event.h>
#include "GuiCalls.h"

void
GPlatesControls::GuiCalls::RepaintCanvas() {

	if (_canvas != NULL) {

		wxPaintEvent evt;
		_canvas->ProcessEvent(evt);
	}
}

void
GPlatesControls::GuiCalls::SetCurrentTime(const GPlatesGlobal::fpdata_t &t) {

	if (_window != NULL) {

		_window->SetCurrentTime(t);
	}
}

void
GPlatesControls::GuiCalls::SetComponents(GPlatesGui::MainWindow *window,
 GPlatesGui::GLCanvas *canvas) {

	_window = window;
	_canvas = canvas;
}

void
GPlatesControls::GuiCalls::SetOpModeToAnimation() {

	if (_window != NULL) {

		_window->SetOpModeToAnimation();
	}
}

void
GPlatesControls::GuiCalls::ReturnOpModeToNormal() {

	if (_window != NULL) {

		_window->ReturnOpModeToNormal();
	}
}

void
GPlatesControls::GuiCalls::StopAnimation(bool interrupted) {

	if (_window != NULL) {

		_window->StopAnimation(interrupted);
	}
}


GPlatesGui::MainWindow *GPlatesControls::GuiCalls::_window = NULL;
GPlatesGui::GLCanvas   *GPlatesControls::GuiCalls::_canvas = NULL;
