/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_MATHS_ROTATIONHISTORY_H_
#define _GPLATES_MATHS_ROTATIONHISTORY_H_

#include <list>

#include "types.h"  /* real_t */
#include "RotationSequence.h"


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/**
	 * This class represents the rotation history of a moving plate.
	 * It is a collection of the various rotational sequences which
	 * describe the motion of this plate with respect to various
	 * fixed plates.
	 *
	 * Note that the collection of rotation sequences is not guaranteed
	 * to be continuous through time: there may be gaps or overlaps.
	 */
	class RotationHistory
	{
		public:

			typedef std::list< RotationSequence > seq_type;

			typedef seq_type::const_iterator const_iterator;


			/**
			 * Create a rotation history.
			 */
			RotationHistory() : _is_modified(false) {  }


			/**
			 * Returns whether this rotation history is "defined"
			 * at a particular point in time.
			 *
			 * A rotation history is "defined" at a particular
			 * point in time if it contains at least one rotation
			 * sequence which is defined at that point in time.
			 */
			bool
			isDefinedAtTime(real_t t) const;


			/**
			 * Insert another rotation sequence into the list.
			 */
			void insert(const RotationSequence &rseq) {

				_seq.push_back(rseq);
				_is_modified = true;
			}


			/**
			 * If this rotation history is defined at time 't',
			 * return a const iterator which points to a rotation
			 * sequence which can be used to rotate the moving
			 * plate back to its location at time 't'.
			 *
			 * @throw InvalidOperationException if the rotation
			 *   history is not defined at time @a t. 
			 */
			const_iterator atTime(real_t t) const;

		private:

			/*
			 * This member is mutable because its value needs
			 * to be changed in the const member function
			 * 'ensureSeqSorted'.
			 */
			mutable seq_type _seq;

			/*
			 * Whether the list of rotation sequences has been
			 * modified since it was last sorted.
			 *
			 * This member is mutable because its value needs
			 * to be changed in the const member function
			 * 'ensureSeqSorted'.
			 */
			mutable bool _is_modified;


			void
			ensureSeqSorted() const {

				if (_is_modified) {

					/*
					 * The sequence has been modified
					 * since last sort.
					 */
					_seq.sort();
					_is_modified = false;
				}
			}
	};
}

#endif  // _GPLATES_MATHS_ROTATIONHISTORY_H_
