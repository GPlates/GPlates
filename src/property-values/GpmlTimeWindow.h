/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H
#define GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H

#include <iosfwd>

#include "GmlTimePeriod.h"
#include "StructuralType.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"

#include "utils/CopyOnWrite.h"
#include "utils/QtStreamable.h"


namespace GPlatesPropertyValues
{

	// Since all the members of this class are of type boost::intrusive_ptr or
	// StructuralType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeWindow :
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GpmlTimeWindow>
	{

	public:

		/**
		 * GpmlTimeWindow has value semantics where each @a GpmlTimeWindow instance has its own state.
		 * So if you create a copy and modify the copy's state then it will not modify the state
		 * of the original object.
		 *
		 * The constructor first clones the property values and then copy-on-write is used to allow
		 * multiple @a GpmlTimeWindow objects to share the same state (until the state is modified).
		 */
		GpmlTimeWindow(
				GPlatesModel::PropertyValue::non_null_ptr_type time_dependent_value_,
				GmlTimePeriod::non_null_ptr_type valid_time_,
				const StructuralType &value_type_) :
			d_time_dependent_value(time_dependent_value_),
			d_valid_time(valid_time_),
			d_value_type(value_type_)
		{  }

		/**
		 * Returns the 'const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		get_time_dependent_value() const
		{
			return d_time_dependent_value.get();
		}

		/**
		 * Returns the 'non-const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		get_time_dependent_value()
		{
			return d_time_dependent_value.get();
		}

		void
		set_time_dependent_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v)
		{
			d_time_dependent_value = v;
		}

		/**
		 * Returns the 'const' time period.
		 */
		const GmlTimePeriod::non_null_ptr_to_const_type
		get_valid_time() const
		{
			return d_valid_time.get();
		}

		/**
		 * Returns the 'non-const' time period.
		 */
		const GmlTimePeriod::non_null_ptr_type
		get_valid_time()
		{
			return d_valid_time.get();
		}

		void
		set_valid_time(
				GmlTimePeriod::non_null_ptr_type vt)
		{
			d_valid_time = vt;
		}

		// Note that no "setter" is provided:  The value type of a GpmlTimeWindow instance
		// should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		bool
		operator==(
				const GpmlTimeWindow &other) const;

	private:

		//! Allow sharing of copied values until modification (copy-on-write value semantics).
		GPlatesUtils::CopyOnWrite<GPlatesModel::PropertyValue::non_null_ptr_type> d_time_dependent_value;

		//! Allow sharing of copied values until modification (copy-on-write value semantics).
		GPlatesUtils::CopyOnWrite<GmlTimePeriod::non_null_ptr_type> d_valid_time;

		StructuralType d_value_type;
	};

	// operator<< for GpmlTimeWindow.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTimeWindow &time_window);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H
