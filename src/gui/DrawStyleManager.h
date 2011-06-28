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

#ifndef GPLATES_GUI_DRAWSTYLEMANAGER_H
#define GPLATES_GUI_DRAWSTYLEMANAGER_H
#include <iostream>
#include <boost/foreach.hpp>
#include <map>
#include <QObject>
#include <QVariant>
#include <vector>
#include "global/python.h"
#include "api/PythonUtils.h"
#include "api/PythonInterpreterLocker.h"
#include "model/FeatureHandle.h"
#include "gui/PythonManager.h"
#include "ColourScheme.h"

namespace GPlatesGui
{
	struct DrawStyle
	{
		DrawStyle() : dummy(0){}
		int dummy;
	};

	class Feature
	{
		int dummy;
	};

	class DrawStyleManager;

	class StyleCatagory
	{
		friend class DrawStyleManager;
	public:
		const QString
		name() const
		{
			return d_name;
		}

		const QString
		desc() const
		{
			return d_desc;
		}
		
		bool
		operator==(const StyleCatagory& other) const
		{
			return d_id == other.d_id;
		}

	private:
		explicit
		StyleCatagory(
				const QString& name_ = QString(), 
				const QString& desc_ = QString()) : 
			d_name(name_), d_desc(desc_)
		{ }
		unsigned d_id;
		QString d_name;
		QString d_desc;
	};


	class StyleAdapter
	{
		friend class DrawStyleManager;
	public:
		explicit
		StyleAdapter(const StyleCatagory * cata) :
			d_catagory(cata)
		{ }

		virtual
		const DrawStyle
		get_style(
				GPlatesModel::FeatureHandle::weak_ref f) const = 0;
	
		virtual
		const QString
		name() const = 0;

		const StyleCatagory&
		catagory()
		{
			return *d_catagory;
		}

		virtual
		~StyleAdapter() { }

		bool
		operator==(const StyleAdapter& other)
		{
			return d_id == other.d_id;
		}

	private:
		const StyleCatagory *d_catagory;
		unsigned d_id;
	};

	class PythonStyleAdapter : 
		public StyleAdapter
	{
	public:
		PythonStyleAdapter(
				const boost::python::object& obj,
				const StyleCatagory * cata) : 
			StyleAdapter(cata),	
			d_obj(obj)
			{ }
		
		const DrawStyle
		get_style(GPlatesModel::FeatureHandle::weak_ref f) const
		{
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			Feature f15;
			DrawStyle ds;
			d_obj.attr("get_style")(f15,ds);
			std::cout << "not crashing.... haha " << std::endl;
			return ds;
		}

		const QString
		name() const 
		{
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			return QString(boost::python::extract<const char*>(d_obj.attr("name")));
		}

	private:
		boost::python::object d_obj;
	};


	class ColourStyleAdapter : 
		public StyleAdapter
	{
	public:
		ColourStyleAdapter(
				boost::shared_ptr<const ColourScheme> scheme,
				const StyleCatagory * cata,
				const QString s_name = QString()) : 
			StyleAdapter(cata),
			d_name(s_name),
			d_scheme(scheme)
			{ }
		
		const DrawStyle
		get_style(GPlatesModel::FeatureHandle::weak_ref f) const
		{
			DrawStyle ds;
			return ds;
		}

		const QString
		name() const 
		{
			return d_name;
		}

	private:
		QString d_name;
		const boost::shared_ptr<const ColourScheme> d_scheme;
	};

	const static char COLOUR_PLATE_ID[]     = "Colour by Plate ID";
	const static char COLOUR_SINGLE[]       = "Colour by Single Colour";
	const static char COLOUR_FEATURE_AGE[]  = "Colour by Feature Age";
	const static char COLOUR_FEATURE_TYPE[] = "Colour by Feature Type";

	class DrawStyleManager :
		public QObject,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		typedef std::vector<StyleAdapter*> StyleContainer;
		typedef std::vector<StyleCatagory*> CatagoryContainer;

		static
		DrawStyleManager*
		instance()
		{
			static DrawStyleManager* inst = new DrawStyleManager();
			return inst;
		}

		void
		register_style(
				StyleAdapter* sa,
				bool built_in = false)
		{
			d_styles.push_back(sa); 
			d_styles.back()->d_id = built_in ? BUILT_IN_OFFSET + d_next_style_id : d_next_style_id;
			++d_next_style_id;
		}

		const StyleAdapter*
		get_style(unsigned id)
		{
			BOOST_FOREACH(StyleContainer::value_type s, d_styles)
			{
				if(s->d_id == id)
					return s;
			}
			return NULL;
		}

		const StyleCatagory*
		register_style_catagory(
				const QString& name,
				const QString& desc = QString(),
				bool built_in = false)
		{
			StyleCatagory* cata = new StyleCatagory(name,desc);
			d_catagories.push_back(cata);
			d_catagories.back()->d_id = built_in ? BUILT_IN_OFFSET + d_next_cata_id : d_next_cata_id ;
			++d_next_cata_id;
			return d_catagories.back();
		}

		~DrawStyleManager()
		{
			clear_container(d_styles);
			clear_container(d_catagories);
		}

		void
		emit_style_changed()
		{
			emit draw_style_changed();
		}

		StyleContainer
		get_styles(const StyleCatagory& cata)
		{
			StyleContainer ret;
			BOOST_FOREACH(StyleContainer::value_type s, d_styles)
			{
				if(s->catagory() == cata)
				{
					ret.push_back(s);
				}
			}
			return ret;
		}

		CatagoryContainer
		all_catagories()
		{
			return d_catagories;
		}

	signals:
		void
		draw_style_changed();

	protected:
		
		template<class ContainerType>
		void
		clear_container(ContainerType& container)
		{
			BOOST_FOREACH(typename ContainerType::value_type s, container)
			{
				delete s;
			}
			container.clear();
		}

		void
		init_built_in_styles();

	private:
		DrawStyleManager();
		StyleContainer d_styles;
		CatagoryContainer d_catagories;
		unsigned d_next_cata_id;
		unsigned d_next_style_id;
		const static unsigned BUILT_IN_OFFSET = 0x80000000;
	};
}

#endif  // GPLATES_GUI_DRAWSTYLEMANAGER_H


