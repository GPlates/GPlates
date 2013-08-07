/* $Id: GmlPoint.h 7942 2010-04-07 07:47:09Z elau $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-11 19:36:59 -0700 (Fri, 11 Jul 2008) $
 * 
 * Copyright (C) 2008, 2009, 2010 California Institute of Technology
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H

#include "model/PropertyValue.h"
#include "utils/UnicodeStringUtils.h"


namespace GPlatesPropertyValues
{

	/**
	 * Base class for topological section derived types.
	 */
	class GpmlTopologicalSection:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalSection>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalSection> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalSection>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalSection> non_null_ptr_to_const_type;


		virtual
		~GpmlTopologicalSection()
		{  }

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalSection>(clone_impl());
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		/**
		 * Construct a GpmlTopologicalSection instance.
		 */
		explicit
		GpmlTopologicalSection(
				const GPlatesModel::PropertyValue::Revision::non_null_ptr_type &revision) :
			PropertyValue(revision)
		{  }

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
