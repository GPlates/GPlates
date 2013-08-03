/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H
#define GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H

#include "GmlTimeInstant.h"

#include "StructuralType.h"
#include "XsString.h"

#include "model/PropertyValue.h"


namespace GPlatesPropertyValues
{

	// Since all the members of this class are of type boost::intrusive_ptr or
	// StructuralType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeSample
	{

	public:

		GpmlTimeSample(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				GmlTimeInstant::non_null_ptr_type valid_time_,
				boost::intrusive_ptr<XsString> description_,
				const StructuralType &value_type_,
				bool is_disabled_ = false):
			d_value(value_),
			d_valid_time(valid_time_),
			d_description(description_),
			d_value_type(value_type_),
			d_is_disabled(is_disabled_)
		{  }

		/**
		 * Copy constructor performs a shallow copy - the internal property values are shared
		 * between the original and copy-constructed objects.
		 */
		GpmlTimeSample(
				const GpmlTimeSample &other) :
			d_value(other.d_value),
			d_valid_time(other.d_valid_time),
			d_description(other.d_description),
			d_value_type(other.d_value_type),
			d_is_disabled(other.d_is_disabled)
		{  }

		/**
		 * Returns a (deep-copy) clone of this GpmlTimeSample.
		 *
		 * Note that the copy constructor of GpmlTimeSample is a shallow copy.
		 */
		const GpmlTimeSample
		clone() const;

		/**
		 * Returns the 'const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		get_value() const
		{
			return d_value;
		}

		/**
		 * Returns the 'non-const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		get_value()
		{
			return d_value;
		}

		void
		set_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v)
		{
			d_value = v;
		}

		/**
		 * Returns the 'const' time instant.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		get_valid_time() const
		{
			return d_valid_time;
		}

		/**
		 * Returns the 'non-const' time instant.
		 */
		const GmlTimeInstant::non_null_ptr_type
		get_valid_time()
		{
			return d_valid_time;
		}

		void
		set_valid_time(
				GmlTimeInstant::non_null_ptr_type vt)
		{
			d_valid_time = vt;
		}

		/**
		 * Returns the 'const' description.
		 */
		const boost::intrusive_ptr<const XsString>
		get_description() const
		{
			return d_description;
		}

		/**
		 * Returns the 'non-const' description.
		 */
		const boost::intrusive_ptr<XsString>
		get_description()
		{
			return d_description;
		}

		void
		set_description(
				boost::intrusive_ptr<XsString> d)
		{
			d_description = d;
		}

		// Note that no "setter" is provided:  The value type of a GpmlTimeSample instance
		// should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		bool
		is_disabled() const
		{
			return d_is_disabled;
		}

		void
		set_disabled(
				bool is_disabled_)
		{
			d_is_disabled = is_disabled_;
		}

		bool
		operator==(
				const GpmlTimeSample &other) const;

	private:

		GPlatesModel::PropertyValue::non_null_ptr_type d_value;

		GmlTimeInstant::non_null_ptr_type d_valid_time;

		// This one is optional.
		boost::intrusive_ptr<XsString> d_description;

		StructuralType d_value_type;

		bool d_is_disabled;
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H
