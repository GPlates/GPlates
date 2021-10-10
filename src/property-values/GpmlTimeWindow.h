/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H
#define GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H

#include "GmlTimePeriod.h"
#include "model/PropertyValue.h"
#include "TemplateTypeParameterType.h"
#include "model/FeatureVisitor.h"
#include "model/ConstFeatureVisitor.h"


namespace GPlatesPropertyValues {

	// Since all the members of this class are of type boost::intrusive_ptr or
	// TemplateTypeParameterType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeWindow {

	public:

		GpmlTimeWindow(
				GPlatesModel::PropertyValue::non_null_ptr_type time_dependent_value_,
				GmlTimePeriod::non_null_ptr_type valid_time_,
				const TemplateTypeParameterType &value_type_) :
			d_time_dependent_value(time_dependent_value_),
			d_valid_time(valid_time_),
			d_value_type(value_type_)
		{  }

		GpmlTimeWindow(
				const GpmlTimeWindow &other) :
			d_time_dependent_value(other.d_time_dependent_value),
			d_valid_time(other.d_valid_time),
			d_value_type(other.d_value_type)
		{  }

		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		time_dependent_value() const {
			return d_time_dependent_value;
		}

		// Note that, because the copy-assignment operator of PropertyValue is private,
		// the PropertyValue referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the PropertyValue within this GpmlTimeWindow instance.  (This
		// restriction is intentional.)
		//
		// To switch the PropertyValue within this GpmlTimeWindow instance, use the
		// function @a set_time_dependent_value below.
		//
		// (This overload is provided to allow the referenced PropertyValue instance to
		// accept a FeatureVisitor instance.)
		const GPlatesModel::PropertyValue::non_null_ptr_type
		time_dependent_value() {
			return d_time_dependent_value;
		}

		void
		set_time_dependent_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v) {
			d_time_dependent_value = v;
		}

		const GmlTimePeriod::non_null_ptr_to_const_type
		valid_time() const {
			return d_valid_time;
		}

		// Note that, because the copy-assignment operator of GmlTimePeriod is private,
		// the GmlTimePeriod referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GmlTimePeriod within this GpmlTimeWindow instance.  (This
		// restriction is intentional.)
		//
		// To switch the GmlTimePeriod within this GpmlTimeWindow instance, use the
		// function @a set_valid_time below.
		//
		// (This overload is provided to allow the referenced GmlTimePeriod instance to
		// accept a FeatureVisitor instance.)
		const GmlTimePeriod::non_null_ptr_type
		valid_time() {
			return d_valid_time;
		}

		void
		set_valid_time(
				GmlTimePeriod::non_null_ptr_type vt) {
			d_valid_time = vt;
		}

		// Note that no "setter" is provided:  The value type of a GpmlTimeWindow instance
		// should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

	private:

		GPlatesModel::PropertyValue::non_null_ptr_type d_time_dependent_value;

		GmlTimePeriod::non_null_ptr_type d_valid_time;

		TemplateTypeParameterType d_value_type;
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTIMEWINDOW_H
