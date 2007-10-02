/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREID_H
#define GPLATES_MODEL_FEATUREID_H

#include <unicode/unistr.h>
#include "utils/UniqueId.h"


namespace GPlatesModel {

	/**
	 * A feature ID acts as a persistent unique identifier for a feature.
	 *
	 * Feature IDs enable features to reference other features.
	 *
	 * To enable the construction and representation of a "unique" identifier (actually, it's
	 * at best a "reasonably unique" identifier), feature IDs are currently based upon strings.
	 *
	 * To enable feature IDs to serve as XML IDs (which might one day be useful), all feature
	 * ID strings must conform to the NCName production which defines the set of string values
	 * which are valid for the XML ID type:
	 *  - http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#ID
	 *  - http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
	 */
	class FeatureId {

	public:

		FeatureId() :
			d_id(GPlatesUtils::generate_unique_id()) {  }

		/**
		 * Construct a feature ID from a UnicodeString instance.
		 *
		 * The string should conform to the XML NCName production (see the class comment
		 * for FeatureId for justification).  (Note however that this constructor won't
		 * validate the contents of the input string.)
		 *
		 * This constructor is intended for use when parsing features from file which
		 * already possess a feature ID.
		 */
		FeatureId(
				const UnicodeString &id) :
			d_id(id) {  }

		/**
		 * Access the Unicode string of the feature ID for this instance.
		 */
		const UnicodeString &
		get() const {
			return d_id;
		}

		/**
		 * Determine whether another FeatureId instance contains the same feature ID
		 * as this instance.
		 */
		bool
		is_equal_to(
				const FeatureId &other) const {
			return d_id == other.d_id;
		}

	private:

		UnicodeString d_id;

	};


	inline
	bool
	operator==(
			const FeatureId &fi1,
			const FeatureId &fi2) {
		return fi1.is_equal_to(fi2);
	}


	inline
	bool
	operator!=(
			const FeatureId &fi1,
			const FeatureId &fi2) {
		return ! fi1.is_equal_to(fi2);
	}

}

#endif  // GPLATES_MODEL_FEATUREID_H
