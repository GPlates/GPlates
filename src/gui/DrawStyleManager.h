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
#include <boost/ref.hpp>
#include <map>
#include <QObject>
#include <QVariant>
#include <vector>
#include "model/FeatureHandle.h"
#include "DrawStyleAdapters.h"
#include "app-logic/UserPreferences.h"

namespace GPlatesGui
{
	class DrawStyleManager;

	class StyleCategory
	{
		friend class DrawStyleManager;
	public:
		const QString&
		name() const
		{
			return d_name;
		}

		const QString&
		desc() const
		{
			return d_desc;
		}
		
		bool
		operator==(const StyleCategory& other) const
		{
			return d_id == other.d_id;
		}

	private:
		explicit
		StyleCategory(
				const QString& name_ = QString(), 
				const QString& desc_ = QString()) : 
			d_name(name_), 
			d_desc(desc_)
		{ }

		unsigned d_id;
		QString d_name;
		QString d_desc;
	};
	

	class DrawStyleManager :
		public QObject,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		typedef std::vector<StyleAdapter*> StyleContainer;
		typedef std::vector<StyleCategory*> CatagoryContainer;

		static
		DrawStyleManager*
		instance()
		{
			//static variable will be initialized only once.
			static DrawStyleManager* inst = new DrawStyleManager();
			return inst;
		}

		/*
		* This function is not so elegant.
		* However, we need to avoid using DrawStyleManager after it has been destructed.
		* This could only happen during GPlates destructing.
		*/
		static
		bool
		is_alive()
		{
			return d_alive_flag;
		}

		void
		register_style(
				StyleAdapter* sa,
				bool built_in = false);

		static
		bool
		is_built_in_style(const StyleAdapter& style)
		{
			return style.d_id >= BUILT_IN_OFFSET;
		}


		bool
		can_be_removed(const StyleAdapter& style)
		{
			if(get_styles(style.catagory()).size() <=1)
				return false;
			else
				return true;
		}


		bool
		remove_style(StyleAdapter* style);

		
		/*
		* Get the number of how many layers are referencing this style.
		*/
		unsigned
		get_ref_number(const StyleAdapter& style) const;

		
		void
		increase_ref(const StyleAdapter& style);

		
		void
		decrease_ref(const StyleAdapter& style);


		/*
		* Since the style object contains a boost python object, 
		* it is necessary to "deep copy" the python object when cloning StyleAdapter.
		* The template StyleAdapter is a StyleAdapter object which contains a "clean" python object.
		* The "clean" python object means it has NOT been configured in any way.
		* It is relatively easy to "deep copy" a "clean" python object.
		*/
		void
		register_template_style(
				const StyleCategory* cata,
				const StyleAdapter* adapter)
		{
			d_template_map[cata] = adapter;
		}


		/*
		* Get all user defined styles.
		*/
		std::vector<StyleAdapter*>
		get_saved_styles(
				const StyleCategory& cata);

		/*
		* Get all built-in styles.
		*/
		std::vector<StyleAdapter*>
		get_built_in_styles(
				const StyleCategory& cata);


		const StyleAdapter*
		get_template_style(
				const StyleCategory& cata);


		const StyleAdapter*
		default_style();


		const StyleCategory*
		register_style_catagory(
				const QString& name,
				const QString& desc = QString(),
				bool built_in = false);


		~DrawStyleManager()
		{
			save_user_defined_styles();
			clear_container(d_styles);
			clear_container(d_catagories);
			d_alive_flag = false;
			
			if(d_use_local_user_pref)
				delete d_user_prefs;
			d_user_prefs = NULL;
		}

		void
		emit_style_changed()
		{
			emit draw_style_changed();
		}

		StyleContainer
		get_styles(const StyleCategory& cata);


		CatagoryContainer&
		all_catagories()
		{
			return d_catagories;
		}

				
		const StyleCategory*
		get_catagory(const QString& _name) const;


		void
		save_user_defined_styles();


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

	private:
		DrawStyleManager(bool local_user_pref=true);
		DrawStyleManager(const DrawStyleManager&);

		StyleContainer d_styles;
		CatagoryContainer d_catagories;
		
		unsigned d_next_cata_id;
		unsigned d_next_style_id;
		const static unsigned BUILT_IN_OFFSET = 0x80000000;
		
		typedef std::map<const StyleAdapter*, unsigned> RefenceMap;
		typedef std::map<const StyleCategory*, const StyleAdapter*> TemplateMap;

		RefenceMap d_reference_map;
		TemplateMap d_template_map;
		const static QString draw_style_prefix;
		GPlatesAppLogic::UserPreferences* d_user_prefs;
		GPlatesAppLogic::UserPreferences::KeyValueMap d_values_map;

		const GPlatesGui::StyleAdapter* d_default_style; 

		static bool d_alive_flag;
		bool d_use_local_user_pref;
	};
}

#endif  // GPLATES_GUI_DRAWSTYLEMANAGER_H


