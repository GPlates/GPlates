/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURSCHEMEDELEGATOR_H
#define GPLATES_GUI_COLOURSCHEMEDELEGATOR_H

#include <map>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <QObject>

#include "ColourScheme.h"
#include "ColourSchemeContainer.h"

#include "model/FeatureCollectionHandle.h"
#include "utils/non_null_intrusive_ptr.h"

namespace GPlatesAppLogic
{
	class ReconstructionGeometry;
}

namespace GPlatesGui
{
	/**
	 * Keeps track of changing target colour schemes - allows switching
	 * of the actual colour scheme implementation without having to change
	 * reference(s) to it (just refer to @a ColourTableDelegator instead).
	 */
	class ColourSchemeDelegator :
			public QObject,
			public ColourScheme
	{
		Q_OBJECT

	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ColourSchemeDelegator>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ColourSchemeDelegator> non_null_ptr_type;

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const ColourSchemeDelegator>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ColourSchemeDelegator> non_null_ptr_to_const_type;

		/**
		 * Holds the category and id of a colour scheme together.
		 */
		typedef std::pair<ColourSchemeCategory::Type, ColourSchemeContainer::id_type> colour_scheme_handle;

		/**
		 * The type of the map from feature collection to colour scheme.
		 */
		typedef std::map<GPlatesModel::FeatureCollectionHandle::const_weak_ref, colour_scheme_handle> colour_schemes_map_type;

		/**
		 * Constructor.
		 */
		explicit
		ColourSchemeDelegator(
				const ColourSchemeContainer &colour_scheme_container);

		/**
		 * Changes the colour scheme for the given @a feature_collection to the one
		 * with the given @a id in the given @a category.
		 *
		 * If the @a feature_collection argument is not provided,
		 * or if it is an invalid weak-ref, the global colour scheme is changed instead.
		 */
		void
		set_colour_scheme(
				ColourSchemeCategory::Type category,
				ColourSchemeContainer::id_type id,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
					GPlatesModel::FeatureCollectionHandle::const_weak_ref());

		/**
		 * Unsets the colour scheme for the given @a feature_collection, if that
		 * feature collection has a special colour scheme set.
		 */
		void
		unset_colour_scheme(
				GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection);

		/**
		 * Gets the colour scheme for @a feature_collection.
		 *
		 * If the @a feature_collection argument is not provided, or if it is an
		 * invalid weak-ref, the global colour scheme is returned instead.
		 *
		 * If the @a feature_collection does not have a special colour scheme set,
		 * boost::none is returned instead.
		 */
		boost::optional<colour_scheme_handle>
		get_colour_scheme(
				GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
					GPlatesModel::FeatureCollectionHandle::const_weak_ref()) const;

		/**
		 * Retrieves a colour for a @a reconstruction_geometry from the current
		 * colour scheme.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureHandle& feature) const
		{
			//TODO:
			return boost::none;
		}

	signals:

		void
		changed();

	private slots:

		void
		handle_colour_scheme_edited(
				GPlatesGui::ColourSchemeCategory::Type category,
				GPlatesGui::ColourSchemeContainer::id_type id);

	private:

		/**
		 * Applies the @a colour_scheme to the @a reconstruction_geometry.
		 */
		boost::optional<Colour>
		apply_colour_scheme(
				const colour_scheme_handle &colour_scheme,
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

		/**
		 * Stores all loaded colour schemes, sorted by category.
		 */
		const ColourSchemeContainer &d_colour_scheme_container;

		/**
		 * The colour scheme to be used by all feature collections for which there is
		 * no special colour scheme.
		 */
		colour_scheme_handle d_global_colour_scheme;

		/**
		 * A map of feature collection to colour scheme, for those feature collections
		 * that have a special colour scheme.
		 */
		colour_schemes_map_type d_special_colour_schemes;

	};
}

#endif // GPLATES_GUI_COLOURSCHEMEDELEGATOR_H
