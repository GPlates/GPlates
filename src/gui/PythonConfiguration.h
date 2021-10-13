/* $Id: CommandLineParser.h 7795 2010-03-16 04:26:54Z jcannon $ */

/**
* \file
* File specific comments.
*
* Most recent change:
*   $Date: 2010-03-16 15:26:54 +1100 (Tue, 16 Mar 2010) $
*
* Copyright (C) 2011 The University of Sydney, Australia
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
#ifndef GPLATES_GUI_PYTHON_CONFIGURATION_H
#define GPLATES_GUI_PYTHON_CONFIGURATION_H

#include <vector>
#include <map>

#include "global/python.h"

#include <boost/foreach.hpp>

#include <QString>
#include <QVariant>

namespace GPlatesGui
{
	class Palette;
	class Colour;

	class ConfigurationItem
	{
	public:
		explicit
		ConfigurationItem(
				const QVariant& v) :
			d_value(v)
		{ }

		ConfigurationItem()
		{ }


		virtual
		const QVariant&
		value() const
		{
			return d_value;
		}


		virtual
		void
		set_value(
				const QVariant& v)
		{
			d_value = v;
		}

		virtual
		ConfigurationItem*
		clone() const = 0;

		virtual
		~ConfigurationItem()
		{ }

	protected:
		QVariant d_value;
	};


	class Configuration
	{
	public:
		Configuration()
		{ }

		const ConfigurationItem*
		get(
				const QString& name) const
		{
			CfgItemMap::const_iterator it = d_items.find(name);
			return (it != d_items.end()) ? it->second : NULL;
		}


		ConfigurationItem*
		get(
				const QString& name) 
		{
			CfgItemMap::iterator it = d_items.find(name);
			return (it != d_items.end()) ? it->second : NULL;
		}


		void
		set(
				const QString& name,
				ConfigurationItem* new_item) 
		{
			CfgItemMap::iterator it = d_items.find(name);
			if(it != d_items.end())
			{
				delete it->second;
				it->second = new_item;
			}
			else
			{
				d_items[name] = new_item;
			}
		}

		std::vector<QString>
		all_cfg_item_names() const 
		{
			std::vector<QString> names;
			BOOST_FOREACH(const CfgItemMap::value_type& item, d_items)
			{
				names.push_back(item.first);
			}
			return names;
		}


		Configuration(
					const Configuration& rhs)
		{
			BOOST_FOREACH(const CfgItemMap::value_type& item, rhs.d_items)
			{
				d_items[item.first] = item.second->clone() ;
			}
		}


		Configuration&
		operator=(
				const Configuration& rhs)
		{
			if(this == &rhs)
				return *this;

			BOOST_FOREACH(const CfgItemMap::value_type& item, rhs.d_items)
			{
				d_items[item.first] = item.second->clone();
			}
			return *this;
		}

		~Configuration()
		{
			BOOST_FOREACH(const CfgItemMap::value_type& item, d_items)
			{
				delete item.second;
			}
		}

	protected:
		
		typedef std::map<QString, ConfigurationItem*> CfgItemMap;
		CfgItemMap d_items;
	};

#if !defined(GPLATES_NO_PYTHON)

	namespace bp = boost::python ;
	
	class PythonCfgItem : public ConfigurationItem
	{
	public:
		PythonCfgItem()
		{ }
		
		const boost::python::object
		py_object() const
		{
			return d_py_obj;
		}

		QString
		get_value()
		{
			return d_value.toString();
		}

		virtual
		PythonCfgItem*
		clone() const = 0;
		
		virtual
		~PythonCfgItem(){ }

	protected:
		boost::python::object d_py_obj;
	};

	
	class PythonCfgColor : public PythonCfgItem
	{
	public:
		PythonCfgColor(
				const QString& cfg_name,
				const QString& color_name) ;

		PythonCfgColor(
				const QString& cfg_name,
				const Colour& color) ;

		void
		set_value(
				const QVariant& value);

		PythonCfgColor*
		clone() const 
		{
			return new PythonCfgColor(*this);
		}
	};


	class PythonCfgPalette : public PythonCfgItem
	{
	public:
		PythonCfgPalette(
				const QString& cfg_name,
				const QString& palette_name);
		
		PythonCfgPalette(
				const QString& cfg_name,
				const Palette* palette) ;

		~PythonCfgPalette();

		void
		set_value(
				const QVariant& value);

		PythonCfgPalette*
		clone() const 
		{
			return new PythonCfgPalette(*this);
		}
	private:
		boost::shared_ptr<Palette> d_palette;
	};


	class PythonCfgString : public PythonCfgItem
	{
	public:
		PythonCfgString(
				const QString& cfg_name,
				const QString& str_value) 
			{
				set_value(str_value);
			}

		~PythonCfgString(){}

		void
		set_value(
				const QVariant& new_value)
		{
			d_value = new_value;
			QString new_str = d_value.toString().trimmed();
			d_py_obj = bp::object(new_str.toStdString());
		}

		PythonCfgString*
		clone() const 
		{
			return new PythonCfgString(*this);
		}

	private:
	};
#endif //GPLATES_NO_PYTHON
}
#endif    //GPLATES_GUI_PYTHON_CONFIGURATION_H


