/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_MATHS_ROTATIONHISTORY_H_
#define _GPLATES_MATHS_ROTATIONHISTORY_H_

#include <list>

#include "types.h"  /* real_t */
#include "RotationSequence.h"


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/**
	 * Compare two rotation sequences by their most recent time.
	 *
	 * This operation provides a strict weak ordering, which enables
	 * rotation sequences to be sorted by STL algorithms.
	 */
	inline bool
	compareMRT(const RotationSequence &rs1, const RotationSequence &rs2) {

		return (rs1.mostRecentTime() < rs2.mostRecentTime());
	}


	/**
	 * This class represents the rotation history of a moving plate.
	 * It is a collection of the various rotational sequences which
	 * describe the motion of this plate with respect to various
	 * fixed plates.
	 *
	 * Note that the collection of rotation sequences is not guaranteed
	 * to be continuous through time: there may be gaps or overlaps.
	 *
	 * Update, 2004-07-12: DM says that overlaps are "not allowed" other
	 * than at "cross-over" points (points in time at which one sequence
	 * ends and another begins).  However, (i) we still need to handle
	 * cross-over points, and (ii) we should still check for overlaps
	 * (perhaps when loading rotation files) because these people have
	 * a habit of hand-editing rotation files and introducing bugs...
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
			 * Returns whether this rotation history is defined
			 * at a particular point in time @a t.
			 *
			 * A rotation history is "defined" at a particular
			 * point in time if it contains at least one rotation
			 * sequence which is defined at that point in time.
			 */
			bool isDefinedAtTime(real_t t) const;


			/**
			 * Return an iterator pointing to the first rotation
			 * sequence in the collection which is defined at time
			 * @a t, or an iterator for the end of the collection,
			 * if an appropriate rotation sequence is not found.
			 */
			const_iterator findAtTime(real_t t) const;


			/**
			 * Return an iterator for the first rotation sequence
			 * in the collection.
			 *
			 * If the collection is empty, this is equivalent to
			 * @p end().
			 *
			 * Note that the collection of rotation sequences is
			 * sorted using the binary predicate @p compareMRT().
			 */
			const_iterator
			begin() const {

				ensureSeqSorted();
				return _seq.begin();
			}


			/**
			 * Return an iterator for the end of the collection.
			 */
			const_iterator
			end() const {

				ensureSeqSorted();
				return _seq.end();
			}


			/**
			 * Insert another rotation sequence into the
			 * collection.
			 */
			void
			insert(const RotationSequence &rseq) {

				_seq.push_back(rseq);
				_is_modified = true;
			}

		private:

			/**
			 * This member is mutable because its value needs
			 * to be changed in the const member function
			 * 'ensureSeqSorted'.
			 */
			mutable seq_type _seq;


			/**
			 * Whether the collection of rotation sequences has
			 * been modified since it was last sorted.
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
					_seq.sort(compareMRT);
					_is_modified = false;
				}
			}
	};
}

#endif  // _GPLATES_MATHS_ROTATIONHISTORY_H_
