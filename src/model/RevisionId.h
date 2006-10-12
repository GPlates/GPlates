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

#ifndef GPLATES_MODEL_REVISIONID_H
#define GPLATES_MODEL_REVISIONID_H

#include <string>
#include "util/UniqueId.h"


namespace GPlatesModel {

	class RevisionId {

	public:

		RevisionId(
				const std::string &id) :
			d_id(id) {  }

		RevisionId() :
			d_id(GPlatesUtil::UniqueId::generate()) {  }

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

		std::string d_id;

	};


	bool
	operator==(
			const RevisionId &ri1,
			const RevisionId &ri2) {
		return ri1.is_equal_to(ri2);
	}


	bool
	operator!=(
			const RevisionId &ri1,
			const RevisionId &ri2) {
		return ! ri1.is_equal_to(ri2);
	}

}

#endif  // GPLATES_MODEL_REVISIONID_H
