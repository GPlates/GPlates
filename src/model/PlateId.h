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

#ifndef GPLATES_MODEL_PLATEID_H
#define GPLATES_MODEL_PLATEID_H


namespace GPlatesModel {

	class PlateId {

	public:

		PlateId(
				unsigned int id) :
			d_id(id) {  }

		/**
		 * Determine whether another PlateId instance contains the same plate ID
		 * as this instance.
		 */
		bool
		is_equal_to(
				const PlateId &other) const {
			return d_id == other.d_id;
		}

	private:

		unsigned int d_id;

	};


	bool
	operator==(
			const PlateId &pi1,
			const PlateId &pi2) {
		return pi1.is_equal_to(pi2);
	}


	bool
	operator!=(
			const PlateId &pi1,
			const PlateId &pi2) {
		return ! pi1.is_equal_to(pi2);
	}

}

#endif  // GPLATES_MODEL_PLATEID_H
