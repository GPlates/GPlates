/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_MATHS_PROXIMITYHITDETAIL_H
#define GPLATES_MATHS_PROXIMITYHITDETAIL_H

#include <boost/intrusive_ptr.hpp>
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class ProximityHitDetailVisitor;


	/**
	 * Derivations of this abstract base class will contain extra information about a proximity
	 * hit -- for example, the specific vertex (point) or segment (GCA) of a polyline which was
	 * hit.
	 */
	class ProximityHitDetail :
			public GPlatesUtils::ReferenceCount<ProximityHitDetail>
	{
	public:
		/**
		 * A convenience typedef for boost::intrusive_ptr<ProximityHitDetail>.
		 */
		typedef boost::intrusive_ptr<ProximityHitDetail> maybe_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ProximityHitDetail,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ProximityHitDetail,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * This is used when there was no proximity hit, and thus no detail.
		 */
		static const maybe_null_ptr_type null;

		/**
		 * Construct a ProximityHitDetail instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		ProximityHitDetail(
				const double &closeness_):
			d_closeness(closeness_)
		{  }

		virtual
		~ProximityHitDetail()
		{  }

		const double &
		closeness() const
		{
			return d_closeness;
		}

		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &) = 0;

	private:
		/**
		 * The "closeness" of the hit.
		 */
		double d_closeness;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		ProximityHitDetail(
				const ProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		ProximityHitDetail &
		operator=(
				const ProximityHitDetail &);
	};


	inline
	ProximityHitDetail::maybe_null_ptr_type
	make_maybe_null_ptr(
			const ProximityHitDetail::non_null_ptr_type &non_null_ptr)
	{
		return ProximityHitDetail::maybe_null_ptr_type(non_null_ptr.get());
	}

}

#endif  // GPLATES_MATHS_PROXIMITYHITDETAIL_H
