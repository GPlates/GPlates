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
#include "PyOldFeature.h"

#include "gui/Colour.h"
#include "gui/ColourPalette.h"
#include "gui/DrawStyleAdapters.h"
#include "gui/DrawStyleManager.h"



namespace bp = boost::python;
using namespace boost::python;

namespace GPlatesApi
{
	//I have trouble to use the static functions , for example "static const Colour &get_red();",
	//in GPlatesGui::Colour class for boost python class member function export due to the fact that
	//boost python seems have problem with the return values which are reference.
	//So, I have to wrap those functions here.  

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

	inline
	GPlatesGui::Colour
	black()
	{
		return GPlatesGui::Colour::get_black();
	}


	inline
	GPlatesGui::Colour
	green()
	{
		return GPlatesGui::Colour::get_green();
	}

	inline
	GPlatesGui::Colour
	grey()
	{
		return GPlatesGui::Colour::get_grey();
	}

	inline
	GPlatesGui::Colour
	silver()
	{
		return GPlatesGui::Colour::get_silver();
	}

	inline
	GPlatesGui::Colour
	purple()
	{
		return GPlatesGui::Colour::get_purple();
	}


	inline
	GPlatesGui::Colour
	yellow()
	{
		return GPlatesGui::Colour::get_yellow();
	}

	inline
	GPlatesGui::Colour
	navy()
	{
		return GPlatesGui::Colour::get_navy();
	}

	//TODO
	// 	static const Colour &get_maroon();
	// 
	// 	static const Colour &get_fuchsia();
	// 	static const Colour &get_lime();
	// 	static const Colour &get_olive();
	// 
	// 	static const Colour &get_teal();
	// 	static const Colour &get_aqua();
}

	
void
export_colour()
{
	class_<GPlatesGui::Colour>("Colour")
		.def(init<const float,const float,const float,const float>())
		.add_static_property("blue", &GPlatesApi::blue)
		.add_static_property("red", &GPlatesApi::red)
		.add_static_property("white", &GPlatesApi::white)
		.add_static_property("black", &GPlatesApi::black)
		.add_static_property("green", &GPlatesApi::green)
		.add_static_property("grey", &GPlatesApi::grey)
		.add_static_property("silver", &GPlatesApi::silver)
		.add_static_property("purple", &GPlatesApi::purple)
		.add_static_property("yellow", &GPlatesApi::yellow)
		.add_static_property("navy", &GPlatesApi::navy)
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


void
export_style()
{
	using namespace boost::python;

	class_<GPlatesGui::DrawStyle>("DrawStyle")
		.def_readwrite("colour", &GPlatesGui::DrawStyle::colour)
		;
}
#endif








