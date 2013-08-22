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

#include <iosfwd>
#include <boost/optional.hpp>

#include "GmlTimeInstant.h"

#include "StructuralType.h"
#include "XsString.h"

#include "model/PropertyValue.h"

#include "utils/QtStreamable.h"


namespace GPlatesPropertyValues
{

	// Since all the members of this class are of type non_null_intrusive_ptr or
	// StructuralType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeSample :
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GpmlTimeSample>
	{

	public:

		GpmlTimeSample(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				GmlTimeInstant::non_null_ptr_type valid_time_,
				boost::optional<XsString::non_null_ptr_type> description_,
				const StructuralType &value_type_,
				bool is_disabled_ = false):
			d_value(value_),
			d_valid_time(valid_time_),
			d_description(description_),
			d_value_type(value_type_),
			d_is_disabled(is_disabled_)
		{  }

		/**
		 * Returns the 'const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return d_value;
		}

		/**
		 * Returns the 'non-const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
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
		valid_time() const
		{
			return d_valid_time;
		}

		/**
		 * Returns the 'non-const' time instant.
		 */
		const GmlTimeInstant::non_null_ptr_type
		valid_time()
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
		const boost::optional<XsString::non_null_ptr_to_const_type>
		description() const
		{
			if (!d_description)
			{
				return boost::none;
			}

			return XsString::non_null_ptr_to_const_type(d_description.get());
		}

		/**
		 * Returns the 'non-const' description.
		 */
		const boost::optional<XsString::non_null_ptr_type>
		description()
		{
			if (!d_description)
			{
				return boost::none;
			}

			return d_description.get();
		}

		void
		set_description(
				boost::optional<XsString::non_null_ptr_type> d)
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

		/**
		 * The description is optional.
		 */
		boost::optional<XsString::non_null_ptr_type> d_description;

		StructuralType d_value_type;

		bool d_is_disabled;
	};

	// operator<< for GpmlTimeSample.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTimeSample &time_sample);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H
