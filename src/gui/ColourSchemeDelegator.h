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
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include "ColourScheme.h"

#include "model/FeatureCollectionHandle.h"
#include "utils/non_null_intrusive_ptr.h"

namespace GPlatesModel
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
			public ColourScheme
	{
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
		 * The type of the map from feature collection to colour scheme.
		 */
		typedef std::map<
				GPlatesModel::FeatureCollectionHandle::const_weak_ref,
				ColourScheme::non_null_ptr_type>
			colour_schemes_map_type;

		/**
		 * Constructs the ColourSchemeDelegator with an initial @a global_colour_scheme.
		 */
		explicit
		ColourSchemeDelegator(
				ColourScheme::non_null_ptr_type global_colour_scheme);

		/**
		 * Changes the colour scheme for the given @a feature_collection to
		 * @a colour_scheme. If the @a feature_collection argument is not provided,
		 * or if it is an invalid weak-ref, the global colour scheme is changed instead.
		 */
		void
		set_colour_scheme(
				ColourScheme::non_null_ptr_type colour_scheme,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
					GPlatesModel::FeatureCollectionHandle::const_weak_ref());

		/**
		 * Gets the colour scheme for @a feature_collection. If the
		 * @a feature_collection argument is not provided, or if it is an invalid
		 * weak-ref, the global colour scheme is returned instead. The global colour
		 * scheme is also returned for feature collections that don't have a special
		 * colour scheme.
		 */
		ColourScheme::non_null_ptr_type
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
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const;

	private:

		/**
		 * The global colour scheme, which is used to colour a geometry in a feature
		 * collection without a special rule.
		 */
		ColourScheme::non_null_ptr_type d_global_colour_scheme;

		/**
		 * A map of feature collection to colour scheme, for those feature collections
		 * that have a special colour scheme.
		 */
		colour_schemes_map_type d_special_colour_schemes;

	};
}

#endif // GPLATES_GUI_COLOURSCHEMEDELEGATOR_H
