/* $Id: Colour.h 11559 2011-05-16 07:41:01Z mchin $ */

/**
 * @file 
 * Contains the definition of the Colour class.
 *
 * Most recent change:
 *   $Date: 2011-05-16 17:41:01 +1000 (Mon, 16 May 2011) $
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 The University of Sydney, Australia
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
#include "ColourScheme.h"
#include "DrawStyleAdapters.h"
#include "api/PyFeature.h"
#include "api/PythonUtils.h"
#include "utils/Profile.h"

#ifndef GPLATES_NO_PYTHON

const GPlatesGui::DrawStyle
GPlatesGui::PythonStyleAdapter::get_style(
		GPlatesModel::FeatureHandle::weak_ref f) const
{
	PROFILE_FUNC();
	if(d_cfg_dirty)
	{
		update_cfg();
		d_cfg_dirty = false;
	}

	GPlatesApi::Feature py_feature(f);
	DrawStyle ds; 
	try
	{
		GPlatesApi::PythonInterpreterLocker lock;
		d_py_obj.attr("get_style")(py_feature, boost::ref(ds));
	}
	catch (const boost::python::error_already_set &)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
	return ds;
}


GPlatesGui::PythonStyleAdapter::PythonStyleAdapter(
		boost::python::object& obj,
		const GPlatesGui::StyleCategory& cata) : 
	StyleAdapter(cata),	
	d_py_obj(obj)
{
	try
	{
		GPlatesApi::PythonInterpreterLocker lock;
		boost::python::object py_class = d_py_obj.attr("__class__");
		d_name	=  QString::fromUtf8(boost::python::extract<const char*>(py_class.attr("__name__")));

		init_configuration();
	}
	catch(const  boost::python::error_already_set&)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


GPlatesGui::PythonStyleAdapter::~PythonStyleAdapter()
{
	GPlatesApi::PythonUtils::python_manager().recycle_python_object(d_py_obj);
	//qDebug() << "Destructing PythonStyleAdapter....";
}


GPlatesGui::StyleAdapter*
GPlatesGui::PythonStyleAdapter::deep_clone() const
{
	bp::object new_py_obj;
	try
	{
		GPlatesApi::PythonInterpreterLocker l;
		bp::object py_copy = bp::import("copy");
		new_py_obj = py_copy.attr("deepcopy")(d_py_obj);
	}
	catch(const bp::error_already_set&)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
	PythonStyleAdapter* p = new PythonStyleAdapter(new_py_obj, d_catagory);
	p->d_cfg = d_cfg;
	return p;
}


void
GPlatesGui::PythonStyleAdapter::init_configuration()
{
	try
	{
		GPlatesApi::PythonInterpreterLocker l;
		bp::dict cfg_defs = bp::extract<bp::dict>(d_py_obj.attr("get_config")());
		bp::list tmp_cfg_items =  cfg_defs.items();
		int len = bp::len(tmp_cfg_items);
		QString cfg_name;
		std::map<QString, QString> cfg_map;

		for (int i = 0; i < len; i++)
		{
			bp::tuple t = bp::extract<bp::tuple>(tmp_cfg_items[i]);
			QString key = QString::fromUtf8(bp::extract<const char*>(t[0]));
			QString value = QString::fromUtf8(bp::extract<const char*>(t[1]));
			QString sub_key = key.right(key.length() - key.indexOf('/') -1);
			key.chop(key.length() - key.indexOf('/'));
			if(key == cfg_name)
			{
				cfg_map[sub_key] = value;
			}
			else
			{
				cfg_map[sub_key] = value;
				cfg_name = key;
				d_cfg.set(cfg_name, create_cfg_item(cfg_map));
				cfg_map.clear();
			}
		}
	}
	catch(const  boost::python::error_already_set&)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
	return;
}


void
GPlatesGui::PythonStyleAdapter::populate_py_dict(boost::python::dict& cfgs) const
{
	BOOST_FOREACH(const QString& cfg_name, d_cfg.all_cfg_item_names())
	{
		const ConfigurationItem* cfg_item = d_cfg.get(cfg_name);
		const PythonCfgItem* py_cfg_item = dynamic_cast<const PythonCfgItem*>(cfg_item);
		if(py_cfg_item)
			cfgs[cfg_name.toStdString()] = py_cfg_item->py_object();
	}
	return;
}


void
GPlatesGui::PythonStyleAdapter::update_cfg() const
{
	try
	{
		GPlatesApi::PythonInterpreterLocker lock;
		bp::dict py_cfg;
		populate_py_dict(py_cfg);
		d_py_obj.attr("set_config")(py_cfg);
	}
	catch(const bp::error_already_set&)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


GPlatesGui::PythonCfgItem*
GPlatesGui::PythonStyleAdapter::create_cfg_item(const std::map<QString, QString>& data)
{
	std::map<QString, QString>::const_iterator it = data.find("type");
	if(it == data.end())
	{
		qWarning() << "No type found in python configuration definition.";
		return NULL;
	}

	if(it->second == "Color")
	{
		return new PythonCfgColor("Color", "white");
	}
	else if(it->second == "Palette")
	{
		return new PythonCfgPalette("Palette", "DeaultPalette");
	} 
	return new PythonCfgString("String", " ");
}


#endif 

const GPlatesGui::DrawStyle
GPlatesGui::ColourStyleAdapter::get_style(
		GPlatesModel::FeatureHandle::weak_ref f) const
{
	PROFILE_FUNC();
	DrawStyle ds;
	boost::optional<Colour> c = d_scheme->get_colour(*f);
	if(c)
	{
		ds.colour = *c;
	}
	return ds;
}






















