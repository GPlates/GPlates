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

#ifndef GPLATES_GUI_DRAWSTYLEADAPTERS_H
#define GPLATES_GUI_DRAWSTYLEADAPTERS_H
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/ref.hpp>
#include <map>
#include <string>
#include <QVariant>
#include <vector>
#include "Colour.h"
#include "global/python.h"
#include "api/PythonUtils.h"
#include "api/PythonInterpreterLocker.h"
#include "model/FeatureHandle.h"
#include "AgeColourPalettes.h"
#include "ColourPalette.h"
#include "ColourScheme.h"
#include "FeatureTypeColourPalette.h"
#include "Palette.h"
#include "PlateIdColourPalettes.h"
#include "PythonConfiguration.h"

namespace GPlatesGui
{
	class DrawStyleManager;
	class StyleCategory;

	struct DrawStyle
	{
		DrawStyle(){ }
		Colour colour;
	};
	

	class StyleAdapter
	{
		friend class DrawStyleManager;
	public:
		explicit
		StyleAdapter(const StyleCategory& cata) :
			d_catagory(cata),
			d_name("Unnamed"),
			d_cfg_dirty(true)
		{ }

		virtual
		const DrawStyle
		get_style(
				GPlatesModel::FeatureHandle::weak_ref f) const = 0;
	
		
		const Configuration&
		configuration() const
		{
			return d_cfg;
		}


		Configuration&
		configuration()
		{
			d_cfg_dirty = true;
			return d_cfg;
		}


		void
		set_dirty_flag(
				bool flag)
		{
			d_cfg_dirty = flag;
		}

		const QString&
		name() const
		{
			return d_name;
		}

		
		void
		set_name(const QString& str)
		{
			d_name = str;
		}


		virtual
		StyleAdapter*
		deep_clone() const = 0;

		
		const StyleCategory&
		catagory() const
		{
			return d_catagory;
		}
			

		virtual
		~StyleAdapter() 
		{
		//	qDebug() << "Destructing StyleAdapter....";
		}


		virtual
		bool
		operator==(
				const StyleAdapter& other)
		{
			return d_id == other.d_id;
		}

	protected:
		const StyleCategory& d_catagory;
		unsigned d_id;
		QString d_name;
		mutable bool d_cfg_dirty;
		Configuration d_cfg;
	};


	namespace bp = boost::python;
	
	class PythonStyleAdapter : 
		public StyleAdapter
	{
	public:
		PythonStyleAdapter(
				bp::object& obj,
				const StyleCategory& cata) ;

				
		const DrawStyle
		get_style(
				GPlatesModel::FeatureHandle::weak_ref f) const;

		
		StyleAdapter*
		deep_clone() const;


		/**
		  * Query the Python class for a dict-of-dicts representing a set of alternative
		  * configuration values, then instantiate additional StyleAdapters and register
		  * them with the DrawStyleManager as variants within the same category as this
		  * PythonStyleAdapter.
		  */
		void
		register_alternative_draw_styles(
			DrawStyleManager& dsm);

		~PythonStyleAdapter();


	protected:
		
		/*
		* This function creates python configuration objects from C++ Configuration object.
		* Callers pass boost python dictionary by reference, into which this function output result data.
		*/
		void
		populate_py_dict(
				boost::python::dict& cfgs) const;

		/*
		* Read python configuration information from python script and create empty Configuration items.
		*/
		void
		init_configuration();
		
		/*
		* Push the configuration data back to python object.
		*/
		void
		update_cfg() const;

		/*
		* Create configuration items according to the config definition map.
		*/
		PythonCfgItem*
		create_cfg_item(
				const std::map<QString, QString>& data);

	private:
		mutable bp::object d_py_obj;
	};


	/*
	* This class is here for historical reason.
	*/
	class ColourStyleAdapter : 
		public StyleAdapter
	{
	public:
		ColourStyleAdapter(
				boost::shared_ptr<const ColourScheme> scheme,
				const StyleCategory& cata,
				const QString s_name = QString()) : 
			StyleAdapter(cata),
			d_scheme(scheme)
			{
				d_name = s_name;
			}
		
		
		const DrawStyle
		get_style(
				GPlatesModel::FeatureHandle::weak_ref f) const;

		
		StyleAdapter*
		deep_clone() const
		{
			return new ColourStyleAdapter(*this);
		}
		

	private:
		const boost::shared_ptr<const ColourScheme> d_scheme;
	};
}

#endif  // GPLATES_GUI_DRAWSTYLEADAPTERS_H




