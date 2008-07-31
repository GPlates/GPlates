/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "TemporaryGeometryFocus.h"


void
GPlatesGui::TemporaryGeometryFocus::set_focus(
		GPlatesModel::TemporaryGeometry::non_null_ptr_type new_geometry)
{
	if ( ! new_geometry) {
		unset_focus();
		return;
	}
	if (d_focused_geometry == new_geometry) {
		// Avoid infinite signal/slot loops like the plague!
		return;
	}
	d_focused_geometry = new_geometry.get();

	emit focus_changed(d_focused_geometry);
}


void
GPlatesGui::TemporaryGeometryFocus::unset_focus()
{
	d_focused_geometry = NULL;
	emit focus_changed(d_focused_geometry);
}


void
GPlatesGui::TemporaryGeometryFocus::announce_modfication_of_focused_geometry()
{
	if ( ! d_focused_geometry) {
		// You can't have modified it, nothing is focused!
		return;
	}
	emit focused_geometry_modified(d_focused_geometry);
}
