/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
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
			 * Create a rotation history, initialising the history
			 * with a rotation sequence.
			 *
			 * Since a rotation sequence must be provided to this
			 * constructor, it will be assumed that a rotation
			 * history can never be empty of rotation sequences.
			 */
			RotationHistory(const RotationSequence &rseq)
			 : _is_modified(true) {

				_seq.push_back(rseq);
				_most_recent_time = rseq.mostRecentTime();
				_most_distant_time = rseq.mostDistantTime();
			}


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
			void insert(const RotationSequence &rseq);


			/**
			 * If this rotation history is defined at time 't',
			 * return a const iterator which points to a rotation
			 * sequence which can be used to rotate the moving
			 * plate back to its location at time 't'.
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
			 * modified since it was last sorted -- if it has
			 * even been sorted yet at all.
			 *
			 * This member is mutable because its value needs
			 * to be changed in the const member function
			 * 'ensureSeqSorted'.
			 */
			mutable bool _is_modified;

			real_t _most_recent_time;  // Millions of years ago
			real_t _most_distant_time;  // Millions of years ago


			void
			ensureSeqSorted() const {

				if (_is_modified) {

					/*
					 * The sequence has been modified
					 * since last sort (or has not yet
					 * been sorted at all).
					 */
					_seq.sort();
					_is_modified = false;
				}
			}
	};
}

#endif  // _GPLATES_MATHS_ROTATIONHISTORY_H_
