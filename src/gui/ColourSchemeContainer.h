/* $Id$ */

/**
 * @file 
 * Contains the defintion of the ColourSchemeContainer class.
 * 
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_COLOURSCHEMECONTAINER_H
#define GPLATES_GUI_COLOURSCHEMECONTAINER_H

#include <map>
#include <iterator>
#include <QString>

#include "ColourScheme.h"
#include "ColourSchemeInfo.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	namespace ColourSchemeCategory
	{
		/**
		 * The different categories that colour schemes can fall into.
		 */
		enum Type
		{
			PLATE_ID,
			SINGLE_COLOUR,
			FEATURE_AGE,
			FEATURE_TYPE,

			NUM_CATEGORIES // Must be the last one.
		};

		/*
		* Are you joking me?
		*/
		class Iterator :
				public std::iterator<std::input_iterator_tag, Type>
		{
		public:

			Iterator(
					Type curr) :
				d_curr(curr)
			{
			}

			Type &
			operator*()
			{
				return d_curr;
			}

			Type *
			operator->()
			{
				return &d_curr;
			}

			Iterator &
			operator++()
			{
				if (d_curr != NUM_CATEGORIES)
				{
					d_curr = static_cast<Type>(d_curr + 1);
				}
				return *this;
			}

			Iterator
			operator++(int)
			{
				Iterator dup(*this);
				++(*this);
				return dup;
			}

			bool
			operator==(
					const Iterator &other)
			{
				return d_curr == other.d_curr;
			}

		private:

			Type d_curr;
		};


		/**
		 * Returns a 'begin' iterator over the colour scheme category enums.
		 */
		Iterator
		begin();


		/**
		 * Returns an 'end' iterator over the colour scheme category enums.
		 */
		Iterator
		end();


		/**
		 * Returns a human-readable description for the given @a category.
		 */
		QString
		get_description(
				Type category);
	}


	/**
	 * ColourSchemeContainer is a container that stores all loaded colour schemes.
	 * Colour schemes are sorted into categories (based on the property by which
	 * they colour a geometry by).
	 *
	 * ColourSchemeContainer does not store which of these loaded colour schemes
	 * is the active colour scheme in the GUI. That is the job of
	 * ColourSchemeDelegator.
	 */
	class ColourSchemeContainer :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * The type of an ID that uniquely identifies a colour scheme loaded
		 * into this ColourSchemeContainer.
		 */
		typedef size_t id_type;

		/**
		 * The type of the underlying container that holds all of the colour schemes
		 * in a particular category.
		 */
		typedef std::map<id_type, ColourSchemeInfo> container_type;

		/**
		 * The type of an iterator over the container of ColourSchemeInfo objects for
		 * a particular category.
		 */
		typedef container_type::const_iterator iterator;

		/**
		 * Constructor.
		 *
		 * Loads a set of built-in colour schemes into this ColourSchemeContainer.
		 */
		explicit
		ColourSchemeContainer(
				GPlatesAppLogic::ApplicationState &application_state);

		/**
		 * Returns a 'begin' iterator over the colour schemes in @a category.
		 */
		iterator
		begin(
				ColourSchemeCategory::Type category) const;

		/**
		 * Returns an 'end' iterator over the colour schemes in @a category.
		 */
		iterator
		end(
				ColourSchemeCategory::Type category) const;

		/**
		 * Adds @a colour_scheme to @a category.
		 */
		id_type
		add(
				ColourSchemeCategory::Type category,
				const ColourSchemeInfo &colour_scheme);

		/**
		 * Removes the colour scheme with the given @a id in @a category.
		 */
		void
		remove(
				ColourSchemeCategory::Type category,
				id_type id);

		/**
		 * Returns the colour scheme with the given @a id in @a category.
		 *
		 * Behaviour is undefined if the id is invalid.
		 */
		const ColourSchemeInfo &
		get(
				ColourSchemeCategory::Type category,
				id_type id) const;

		/**
		 * Adds a colour scheme to the category Single Colour.
		 */
		id_type
		add_single_colour_scheme(
				const Colour &colour,
				const QString &colour_name,
				bool is_built_in = true);

		/**
		 * Changes the Single Colour scheme with given @a id to @a colour.
		 */
		void
		edit_single_colour_scheme(
				id_type id,
				const Colour &colour,
				const QString &colour_name);

	signals:

		void
		colour_scheme_edited(
				GPlatesGui::ColourSchemeCategory::Type,
				GPlatesGui::ColourSchemeContainer::id_type);

	private:

		/**
		 * Creates the built-in colour schemes and places them into the categories.
		 */
		void
		create_built_in_colour_schemes(
				GPlatesAppLogic::ApplicationState &application_state);

		ColourSchemeInfo
		create_single_colour_scheme(
				const Colour &colour,
				const QString &colour_name,
				bool is_built_in);

		/**
		 * Remembers the next id to be given out to a ColourSchemeInfo when inserted.
		 */
		id_type d_next_id;

		/**
		 * Stores the loaded colour schemes, sorted into categories.
		 */
		container_type d_colour_schemes[ColourSchemeCategory::NUM_CATEGORIES];
	};
}

#endif  // GPLATES_GUI_COLOURSCHEMECONTAINER_H
