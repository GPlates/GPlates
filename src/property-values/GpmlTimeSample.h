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
#include "model/PropertyValue.h"
#include "XsString.h"
#include "StructuralType.h"
#include "model/FeatureVisitor.h"


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

		GpmlTimeSample(
				const GpmlTimeSample &other) :
			d_value(other.d_value),
			d_valid_time(other.d_valid_time),
			d_description(other.d_description),
			d_value_type(other.d_value_type),
			d_is_disabled(other.d_is_disabled)
		{  }

		const GpmlTimeSample
		deep_clone() const;

		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return d_value;
		}

		// Note that, because the copy-assignment operator of PropertyValue is private,
		// the PropertyValue referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the PropertyValue within this GpmlTimeSample instance.  (This
		// restriction is intentional.)
		//
		// To switch the PropertyValue within this GpmlTimeSample instance, use the
		// function @a set_value below.
		//
		// (This overload is provided to allow the referenced PropertyValue instance to
		// accept a FeatureVisitor instance.)
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

		const GmlTimeInstant::non_null_ptr_to_const_type
		valid_time() const
		{
			return d_valid_time;
		}

		// Note that, because the copy-assignment operator of GmlTimeInstant is private,
		// the GmlTimeInstant referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GmlTimeInstant within this GpmlTimeSample instance.  (This
		// restriction is intentional.)
		//
		// To switch the GmlTimeInstant within this GpmlTimeSample instance, use the
		// function @a set_valid_time below.
		//
		// (This overload is provided to allow the referenced GmlTimeInstant instance to
		// accept a FeatureVisitor instance.)
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

		const boost::intrusive_ptr<const XsString>
		description() const
		{
			return d_description;
		}

		// Note that, because the copy-assignment operator of XsString is private, the
		// XsString referenced by the return-value of this function cannot be assigned-to,
		// which means that this function does not provide a means to directly switch the
		// XsString within this GpmlTimeSample instance.  (This restriction is
		// intentional.)
		//
		// To switch the XsString within this GpmlTimeSample instance, use the function
		// @a set_value below.
		//
		// (This overload is provided to allow the referenced XsString instance to accept a
		// FeatureVisitor instance.)
		const boost::intrusive_ptr<XsString>
		description()
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
		value_type() const
		{
			return d_value_type;
		}

		bool
		is_disabled() const
		{
			return d_is_disabled;
		}

		bool
		is_disabled()
		{
			return d_is_disabled;
		}

		void
		set_is_disabled(
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
