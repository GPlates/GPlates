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


	class RotationHistory
	{
		public:

			typedef std::list< RotationSequence > seq_type;

			typedef seq_type::const_iterator const_iterator;


			RotationHistory(const RotationSequence &rseq)
			 : _is_modified(true) {

				_seq.push_back(rseq);
				_most_recent_time = rseq.mostRecentTime();
				_most_distant_time = rseq.mostDistantTime();
			}


			bool
			isDefinedAtTime(real_t t) const {

				return (_most_recent_time <= t &&
				        t <= _most_distant_time);
			}


			/**
			 * Insert another rotation sequence into the list.
			 */
			void insert(const RotationSequence &rseq);


			// returns first match
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
