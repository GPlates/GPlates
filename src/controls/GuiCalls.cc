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
