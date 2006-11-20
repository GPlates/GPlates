/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_REVISIONID_H
#define GPLATES_MODEL_REVISIONID_H

#include <unicode/unistr.h>
#include "util/UniqueId.h"


namespace GPlatesModel {

	/**
	 * A revision ID acts as a persistent unique identifier for a single revision of a feature.
	 *
	 * Revision IDs enable features to reference specific revisions of other features.
	 *
	 * To enable the construction and representation of a "unique" identifier (actually, it's
	 * at best a "reasonably unique" identifier), revision IDs are currently based upon
	 * strings.
	 *
	 * To enable revision IDs to serve as XML IDs (which might one day be useful), all revision
	 * ID strings must conform to the NCName production which defines the set of string values
	 * which are valid for the XML ID type:
	 *  - http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#ID
	 *  - http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
	 */
	class RevisionId {

	public:

		RevisionId() :
			d_id(GPlatesUtil::UniqueId::generate()) {  }

		/**
		 * Construct a revision ID from a UnicodeString instance.
		 *
		 * The string should conform to the XML NCName production (see the class comment
		 * for RevisionId for justification).  (Note however that this constructor won't
		 * validate the contents of the input string.)
		 *
		 * This constructor is intended for use when parsing features from file which
		 * already possess a revision ID.
		 */
		RevisionId(
				const UnicodeString &id) :
			d_id(id) {  }

		/**
		 * Access the Unicode string of the revision ID for this instance.
		 */
		const UnicodeString &
		get() const {
			return d_id;
		}

		/**
		 * Determine whether another RevisionId instance contains the same revision ID
		 * as this instance.
		 */
		bool
		is_equal_to(
				const RevisionId &other) const {
			return d_id == other.d_id;
		}

	private:

		UnicodeString d_id;

	};


	inline
	bool
	operator==(
			const RevisionId &ri1,
			const RevisionId &ri2) {
		return ri1.is_equal_to(ri2);
	}


	inline
	bool
	operator!=(
			const RevisionId &ri1,
			const RevisionId &ri2) {
		return ! ri1.is_equal_to(ri2);
	}

}

#endif  // GPLATES_MODEL_REVISIONID_H
