/* $Id: ApplicationState.cc 11614 2011-05-20 15:10:51Z jcannon $ */

/**
 * \file 
 * $Revision: 11614 $
 * $Date: 2011-05-21 01:10:51 +1000 (Sat, 21 May 2011) $
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#include "api/PythonInterpreterLocker.h"
#include "api/PythonUtils.h"

#include "Colour.h"
#include "DrawStyleManager.h"
#include "Palette.h"
#include "PythonConfiguration.h"


GPlatesGui::PythonCfgItem::~PythonCfgItem()
{
	// If our Python object is going to be destroyed then do it while holding the Python GIL.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// If holding only reference to Python object then force its destruction at end of scope.
	boost::python::object py_obj = d_py_obj;
	d_py_obj = boost::python::object();
}


GPlatesGui::PythonCfgColor::PythonCfgColor(
		const QString& cfg_name,
		const QString& color_name) 
{
	set_value(color_name);
}

GPlatesGui::PythonCfgColor::PythonCfgColor(
		const QString& cfg_name,
		const Colour& color) 
{
	d_py_obj = bp::object(color);
	set_value("");
}

void
GPlatesGui::PythonCfgColor::set_value(const QVariant& val)
{
	d_value = val;

	// Previous Python object could get destroyed.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	
	d_py_obj = bp::object(Colour(QColor(val.toString())));
}


GPlatesGui::PythonCfgPalette::PythonCfgPalette(
		const QString& cfg_name,
		const QString& palette_name)
{
	set_value(palette_name);
}
		
GPlatesGui::PythonCfgPalette::PythonCfgPalette(
		const QString& cfg_name,
		const Palette* palette) 
{
	d_py_obj = bp::object(GPlatesApi::Palette(palette));
	set_value("");
}

GPlatesGui::PythonCfgPalette::~PythonCfgPalette()
{
	
}

void
GPlatesGui::PythonCfgPalette::set_value(const QVariant& val)
{
	d_value = val;
	QString filename = d_value.toString();
	
	QFileInfo file_info(filename);
	if(file_info.isFile() && file_info.isReadable())
	{
		try
		{
			d_palette.reset(new CptPalette(filename));
		}
		catch (GPlatesGlobal::LogException& ex)
		{
			d_palette.reset();
			ex.write(std::cerr);
		}

		// Previous Python object could get destroyed.
		GPlatesApi::PythonInterpreterLocker interpreter_locker;

		d_py_obj = bp::object(GPlatesApi::Palette(d_palette.get()));
	}
	else
	{
		// Previous Python object could get destroyed.
		GPlatesApi::PythonInterpreterLocker interpreter_locker;

		d_py_obj = bp::object(GPlatesApi::Palette(built_in_palette(filename)));
	}
}

bool
GPlatesGui::PythonCfgPalette::is_built_in_palette() const
{
	return built_in_palette(d_value.toString()) != NULL;
}
