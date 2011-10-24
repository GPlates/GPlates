/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include "global/python.h"
#if !defined(GPLATES_NO_PYTHON)

#include "PythonUtils.h"
#include "gui/Colour.h"
#include "gui/ColourPalette.h"
#include "gui/DrawStyleAdapters.h"

namespace bp = boost::python;
using namespace boost::python;
namespace GPlatesApi
{
	inline
	GPlatesGui::Colour
	red()
	{
		return GPlatesGui::Colour::get_red();
	}

	inline
	GPlatesGui::Colour
	blue()
	{
		return GPlatesGui::Colour::get_blue();
	}

	inline
	GPlatesGui::Colour
	white()
	{
		return GPlatesGui::Colour::get_white();
	}
}

	
void
export_colour()
{
	class_<GPlatesGui::Colour>("Colour")
		.def(init<const float,const float,const float,const float>())
		.add_static_property("blue", &GPlatesApi::blue)
		.add_static_property("red", &GPlatesApi::red)
		.add_static_property("white", &GPlatesApi::white)
		;

	class_<GPlatesApi::Palette>("Palette", no_init)
		.def("get_color", &GPlatesApi::Palette::get_color)
		;

	class_<GPlatesGui::Palette::Key>("PaletteKey")
		.def(init<const long>())
		.def(init<const float>())
		.def(init<const double>())
		.def(init<const char*>())
		;
}

#include "gui/DrawStyleManager.h"
#include "PyFeature.h"
void
export_style()
{
	using namespace boost::python;

	class_<GPlatesGui::DrawStyle>("DrawStyle")
		.def_readwrite("colour", &GPlatesGui::DrawStyle::colour)
		;
}
#endif








