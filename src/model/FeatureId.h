/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_FEATUREID_H
#define GPLATES_MODEL_FEATUREID_H

#include <string>
#include "util/UniqueId.h"


namespace GPlatesModel {

	/**
	 * A feature ID acts as a persistent unique identifier for a feature.
	 *
	 * Feature IDs enable features to reference other features.
	 *
	 * To enable the construction and representation of a "unique" identifier (actually, it's
	 * at best a "reasonably unique" identifier), feature IDs are currently based upon strings.
	 *
	 * To enable feature IDs to serve as XML IDs (which might one day be useful) without
	 * becoming too complicated for us programmers, all feature ID strings must conform to the
	 * regexp "[A-Za-z_][-A-Za-z_0-9.]*", which is a subset of the NCName production which
	 * defines the set of string values which are valid for the XML ID type:
	 *  - http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#ID
	 *  - http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
	 */
	class FeatureId {

	public:

		FeatureId() :
			d_id(GPlatesUtil::UniqueId::generate()) {  }

		/**
		 * Construct a feature ID from a 'std::string'.
		 *
		 * The string should conform to the regexp "[A-Za-z_][-A-Za-z_0-9.]*" (see the
		 * class comment for FeatureId for justification).  (Note however that this
		 * constructor won't verify the contents of the input string.)  Since all of the
		 * characters produced by this regexp are ASCII characters, the input string is a
		 * 'std::string' rather than, say, a UnicodeString.
		 *
		 * This constructor is intended for use when parsing features from file which
		 * already possess a feature ID.
		 */
		FeatureId(
				const std::string &id) :
			d_id(id) {  }

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

		std::string d_id;

	};


	bool
	operator==(
			const FeatureId &fi1,
			const FeatureId &fi2) {
		return fi1.is_equal_to(fi2);
	}


	bool
	operator!=(
			const FeatureId &fi1,
			const FeatureId &fi2) {
		return ! fi1.is_equal_to(fi2);
	}

}

#endif  // GPLATES_MODEL_FEATUREID_H
