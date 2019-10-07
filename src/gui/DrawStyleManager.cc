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
#include <algorithm>
#include "app-logic/PropertyExtractors.h"
#include "global/CompilerWarnings.h"
#include "presentation/Application.h"	
#include "AgeColourPalettes.h"
#include "DrawStyleManager.h"
#include "DrawStyleAdapters.h"
#include "FeatureTypeColourPalette.h"
#include "GenericColourScheme.h"
#include "HTMLColourNames.h"
#include "PlateIdColourPalettes.h"
#include "SingleColourScheme.h"

#include "utils/ConfigBundle.h"

DISABLE_GCC_WARNING("-Wold-style-cast")

bool GPlatesGui::DrawStyleManager::d_alive_flag = false;

GPlatesGui::DrawStyleManager::DrawStyleManager(
		bool local_user_pref) :
	d_next_cata_id(0),
	d_next_style_id(0),
	d_default_style(NULL),
	d_use_local_user_pref(local_user_pref)
{ 
	d_alive_flag = true;

	//Since DrawStyleManager is a singleton, it is safer to use a local UserPreferences by default.
	if(d_use_local_user_pref)
		d_user_prefs = new GPlatesAppLogic::UserPreferences(this);
	else
		d_user_prefs = &GPlatesPresentation::Application::instance().get_application_state().get_user_preferences();

	//register built-in categories
	register_style_catagory("PlateId");
	register_style_catagory("SingleColour");
	register_style_catagory("FeatureAge");
	register_style_catagory("FeatureType");
}


void
GPlatesGui::DrawStyleManager::register_style(
		StyleAdapter* sa,
		bool built_in)
{
	d_styles.push_back(sa); 
	d_styles.back()->d_id = built_in ? BUILT_IN_OFFSET + d_next_style_id : d_next_style_id;
	++d_next_style_id;
}


unsigned
GPlatesGui::DrawStyleManager::get_ref_number(
		const GPlatesGui::StyleAdapter& style) const
{
	RefenceMap::const_iterator it = d_reference_map.find(&style);
	if(it == d_reference_map.end())
	{
		return 0;
	}
	else
	{
		return it->second;
	}
}

		
void
GPlatesGui::DrawStyleManager::increase_ref(
		const GPlatesGui::StyleAdapter& style)
{
	RefenceMap::iterator it = d_reference_map.find(&style);
	if(it == d_reference_map.end())
	{
		d_reference_map[&style] = 1;
	}
	else
	{
		++it->second;
	}
}

		
void
GPlatesGui::DrawStyleManager::decrease_ref(
		const GPlatesGui::StyleAdapter& style)
{
	RefenceMap::iterator it = d_reference_map.find(&style);
	if(it == d_reference_map.end())
	{
		qWarning() << "Cannot find style.";
		return;
	}
	else
	{
		if(it->second <= 1)
			d_reference_map.erase(it);
		else
			--it->second;
	}
}


const GPlatesGui::StyleAdapter*
GPlatesGui::DrawStyleManager::get_template_style(
		const GPlatesGui::StyleCategory& cata)
{
	TemplateMap::const_iterator it = d_template_map.find(&cata);
	if(it!=d_template_map.end())
		return it->second;
	else
		return NULL;
}


const GPlatesGui::StyleAdapter*
GPlatesGui::DrawStyleManager::default_style()
{
	if(!d_default_style)
	{
		const StyleCategory* cat = get_catagory("PlateId");
		if(cat)
		{
			const StyleContainer& styles = get_styles(*cat);

			BOOST_FOREACH(const StyleAdapter* s, styles)
			{
				if(s->name() == "Default")
				{
					d_default_style = s;
				}
			}
		}
		if(!d_default_style)
		{
			qWarning() << "Cannot find default draw style setting.";
			boost::shared_ptr<const ColourScheme> cs (
					new GenericColourScheme<GPlatesAppLogic::PlateIdPropertyExtractor>(
							DefaultPlateIdColourPalette::create(),
							GPlatesAppLogic::PlateIdPropertyExtractor()));
			const StyleCategory* temp_cata = new StyleCategory("PlateID");
			d_default_style = new ColourStyleAdapter(cs,*temp_cata,"Default");
			qWarning() << "A temporary default draw style has been created.";
		}
	}

	return d_default_style;
}


const GPlatesGui::StyleCategory*
GPlatesGui::DrawStyleManager::register_style_catagory(
		const QString& name,
		const QString& desc,
		bool built_in)
{
	StyleCategory* cata = new StyleCategory(name, desc);
	d_catagories.push_back(cata);
	d_catagories.back()->d_id = built_in ? BUILT_IN_OFFSET + d_next_cata_id : d_next_cata_id ;
	++d_next_cata_id;
	return d_catagories.back();
}


bool
GPlatesGui::DrawStyleManager::remove_style(GPlatesGui::StyleAdapter* style)
{
	StyleContainer::iterator it = std::find(d_styles.begin(), d_styles.end(), style);
	if(it == d_styles.end())
	{
		qWarning() << "Cannot find style adapter to remove.";
		return false;
	}

	if(style->d_id >= BUILT_IN_OFFSET)
	{
		qWarning() << "Cannot remove built-in style.";
		return false;
	}

	if(get_ref_number(*style) > 1)
	{
		qWarning() << "Cannot remove in-use style.";
		return false;
	}

#if !defined(GPLATES_NO_PYTHON)
	// Deleting a style also destroys Python objects.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
#endif

	d_styles.erase(it);
	delete style;
	return true;
}


const GPlatesGui::StyleCategory*
GPlatesGui::DrawStyleManager::get_catagory(
		const QString& _name) const
{
	BOOST_FOREACH(const GPlatesGui::StyleCategory* s_cat, d_catagories)
	{
		if(s_cat->name() == _name)
			return s_cat;
	}
	return NULL;
}


GPlatesGui::DrawStyleManager::StyleContainer
GPlatesGui::DrawStyleManager::get_styles(
		const GPlatesGui::StyleCategory& cata)
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

const QString GPlatesGui::DrawStyleManager::draw_style_prefix = "draw_styles/user-defined";

// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")
void
GPlatesGui::DrawStyleManager::save_user_defined_styles()
{
	d_user_prefs->clear_prefix(draw_style_prefix);

	BOOST_FOREACH(const StyleContainer::value_type& style, d_styles)
	{
		if(is_built_in_style(*style))
			continue;

		GPlatesUtils::ConfigBundle cfg_bundle(this);
		const Configuration& cfg = style->configuration();
		BOOST_FOREACH(const QString& item_name, cfg.all_cfg_item_names())
		{
			cfg_bundle.set_value(item_name, cfg.get(item_name)->value());
		}
		d_user_prefs->insert_keyvalues_from_configbundle(
				draw_style_prefix + "/" + style->catagory().name() + "/" + style->name(),
				cfg_bundle);
	}
}


std::vector<GPlatesGui::StyleAdapter*>
GPlatesGui::DrawStyleManager::get_saved_styles(const GPlatesGui::StyleCategory& cata)
{
	std::vector<StyleAdapter*> ret;
	
	const StyleAdapter* template_adapter = get_template_style(cata);
	if(!template_adapter)
		return ret;

	GPlatesUtils::ConfigBundle* styles_in_catagory = 
		d_user_prefs->extract_keyvalues_as_configbundle(draw_style_prefix + "/" + cata.name());
	
	QSet<QString> style_names;
	Q_FOREACH(QString subkey, styles_in_catagory->subkeys()) 
	{
		subkey = subkey.simplified();
		style_names.insert(subkey.split("/").first());
	}

	Q_FOREACH(QString style_name, style_names) 
	{
		if(style_name == "paths")//TODO: why "paths" is here?
			continue;

		GPlatesUtils::ConfigBundle* style_bundle = 
			d_user_prefs->extract_keyvalues_as_configbundle(draw_style_prefix + "/" + cata.name() + "/" + style_name);
		StyleAdapter* new_adapter = template_adapter->deep_clone();
		if(!new_adapter)
			continue;

		new_adapter->set_name(style_name);
		Configuration& cfg = new_adapter->configuration();

		Q_FOREACH(QString subkey, style_bundle->subkeys()) 
		{
			subkey = subkey.simplified();
			ConfigurationItem* cfg_item = cfg.get(subkey);
			if(cfg_item)
				cfg_item->set_value(style_bundle->get_value(subkey));
		}
		ret.push_back(new_adapter);
	}
	return ret;
}


namespace
{
	using namespace GPlatesGui;

	StyleAdapter*
	create_built_in_palette_adapter(
			const QString& cfg_name, 
			const QString& palette_name,
			const StyleAdapter* template_adapter)  
	{ 
		StyleAdapter* new_adapter = template_adapter->deep_clone();
		if(!new_adapter) 
			return NULL;

		new_adapter->set_name(cfg_name);
		Configuration& cfg = new_adapter->configuration();
		ConfigurationItem* cfg_item = cfg.get("Palette");
		//we should be able to find an "Palette" item
		if(cfg_item)
		{
			cfg_item->set_value(palette_name);
		}
		else
		{
			//if cannot find "Palette" item, try our best...
			BOOST_FOREACH(const QString& item_name, cfg.all_cfg_item_names())
			{
				cfg.get(item_name)->set_value(palette_name);
			}
		}
		return new_adapter;
	}
}

std::vector<GPlatesGui::StyleAdapter*>
GPlatesGui::DrawStyleManager::get_built_in_styles(
		const GPlatesGui::StyleCategory& cata) 
{
	std::vector<StyleAdapter*> ret;
	static const char* color_names[] = {"white","blue","black","silver","gold","pink","green","orange"};

	const StyleAdapter* t_adapter = get_template_style(cata);
	if(!t_adapter)
		return ret;

	const StyleAdapter& adapter = *t_adapter;
	if(adapter.name() == "SingleColour")
	{
		for(unsigned int i=0; i< sizeof(color_names)/sizeof(char*);i++)
		{
			StyleAdapter* new_adapter = adapter.deep_clone();
			if(!new_adapter)
				continue;

			new_adapter->set_name(color_names[i]);
			Configuration& cfg = new_adapter->configuration();
			ConfigurationItem* cfg_item = cfg.get("Colour");
			//we should be able to find an "Colour" item
			if(cfg_item)
			{
				cfg_item->set_value(color_names[i]);
			}
			else
			{
				//if cannot find "Colour" item, try our best...
				BOOST_FOREACH(const QString& item_name, cfg.all_cfg_item_names())
				{
					cfg.get(item_name)->set_value(color_names[i]);
				}
			}
			ret.push_back(new_adapter);
		}
	}
	else if(adapter.name() == "PlateId")
	{
		ret.push_back(create_built_in_palette_adapter("Default", "DefaultPlateId", &adapter));
		ret.push_back(create_built_in_palette_adapter("Region", "Region", &adapter));
	}
	else if(adapter.name() == "FeatureAge")
	{
		ret.push_back(create_built_in_palette_adapter("Default", "FeatureAgeDefault", &adapter));
		ret.push_back(create_built_in_palette_adapter("Monochrome", "FeatureAgeMono", &adapter));
	}
	else if(adapter.name() == "FeatureType")
	{
		ret.push_back(create_built_in_palette_adapter("Default", "FeatureType", &adapter));
	}
	// Note: Rather than hack in some hard-coded variants for the ArbitraryColours python style adapter here,
	// I've implemented a GPlatesGui::PythonStyleAdapter::register_alternative_draw_styles() which gets invoked
	// by PyApplication during the python style loading process; this queries the python objects for a
	// function called get_config_variants() which is assumed to return a dict of (str -> dict of (str->str))
	// representing an assortment of variant styles' configs, to be presented in the large preview pane to
	// the right of the category list.
// 	else
// 	{
// 		StyleAdapter* new_adapter = adapter.deep_clone();
// 		new_adapter->set_name("Default");
// 		ret.push_back(new_adapter);
// 	}
	return ret;
}























