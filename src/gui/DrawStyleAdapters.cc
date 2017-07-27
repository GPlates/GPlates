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
#include "DrawStyleManager.h"
#include "api/PyOldFeature.h"
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

	GPlatesApi::OldFeature py_feature(f);
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
	// If our Python object is going to be destroyed then do it while holding the Python GIL.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// If holding only reference to Python object then force its destruction at end of scope.
	boost::python::object py_obj = d_py_obj;
	d_py_obj = boost::python::object();
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
GPlatesGui::PythonStyleAdapter::register_alternative_draw_styles(
		GPlatesGui::DrawStyleManager &dsm)
{
	// Query the Python object for a dict of dicts, providing alternative 'built in' styles
	// for this PythonStyleAdapter. Rather than attempting to return them in C++, which sucks,
	// we may as well go and register them with the DrawStyleManager directly.
	try
	{
		GPlatesApi::PythonInterpreterLocker l;
		// Test to see if we have declared the get_config_variants() function.
		// Not having it declared is not an error.
		//
		// Use the Python C API to determine if attribute string is present.
		// We could just get the attribute and catch the Python error, but then we'd need to
		// call 'PyErr_Fetch()' to clear the error indicator (otherwise it'll just get raised again).
		// Note that 'PythonUtils::get_error_message()' calls 'PyErr_Fetch()' internally which is
		// why it works in other areas of the code (but we don't want to print an error message).
		if (!PyObject_HasAttrString(d_py_obj.ptr(), "get_config_variants"))
		{
			return;
		}

		bp::object fun;
		try {
			fun = d_py_obj.attr("get_config_variants");
		} catch (const  boost::python::error_already_set &) {
			// Shouldn't get here - we already checked presence of attribute above.
			qWarning() << GPlatesApi::PythonUtils::get_error_message();
			return;
		}

		// Invoke the function get_config_variants() on the python object; expect it to return
		// a dict where keys are variant draw style names and values are dicts of config settings.
		bp::dict cfg_variants = bp::extract<bp::dict>(fun());
		bp::list var_items = cfg_variants.items();
		int num_var_items = bp::len(var_items);

		for (int i = 0; i < num_var_items; i++) {
			bp::tuple variant_tuple = bp::extract<bp::tuple>(var_items[i]);
			QString variant_name = QString::fromUtf8(bp::extract<const char*>(variant_tuple[0]));
			bp::dict variant_config = bp::extract<bp::dict>(variant_tuple[1]);

			// Create a clone of this StyleAdapter to handle the config variant.
			StyleAdapter* variant_style_adapter = this->deep_clone();
			if( ! variant_style_adapter) {
				return;
			}
			variant_style_adapter->set_name(variant_name);

			// Get its Configuration object.
			Configuration& variant_cfg = variant_style_adapter->configuration();

			// Iterate through the key-value dict supplying the default config values.
			bp::list variant_config_items = variant_config.items();
			int num_variant_config_items = bp::len(variant_config_items);

			for (int j = 0; j < num_variant_config_items; j++) {
				bp::tuple var_cfg_tuple = bp::extract<bp::tuple>(variant_config_items[j]);
				QString key = QString::fromUtf8(bp::extract<const char*>(var_cfg_tuple[0]));
				QString val = QString::fromUtf8(bp::extract<const char*>(var_cfg_tuple[1]));

				// Set the appropriate key-value on the Configuration.
				ConfigurationItem* cfg_item = variant_cfg.get(key);
				if (cfg_item) {
					cfg_item->set_value(val);
				}
			}
			// Now that we've configured it, register it with the DrawStyleManager.
			dsm.register_style(variant_style_adapter, true);
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






















