/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GPMLTIMESAMPLE_H
#define GPLATES_MODEL_GPMLTIMESAMPLE_H

#include <boost/intrusive_ptr.hpp>
#include "GmlTimeInstant.h"
#include "PropertyValue.h"
#include "XsString.h"
#include "TemplateTypeParameterType.h"
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

	// Since all the members of this class are of type boost::intrusive_ptr or
	// TemplateTypeParameterType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeSample {

	public:

		GpmlTimeSample(
				boost::intrusive_ptr<PropertyValue> value_,
				boost::intrusive_ptr<GmlTimeInstant> valid_time_,
				boost::intrusive_ptr<XsString> description_,
				const TemplateTypeParameterType &value_type_):
			d_value(value_),
			d_valid_time(valid_time_),
			d_description(description_),
			d_value_type(value_type_)
		{  }

		GpmlTimeSample(
				const GpmlTimeSample &other) :
			d_value(other.d_value),
			d_valid_time(other.d_valid_time),
			d_description(other.d_description),
			d_value_type(other.d_value_type)
		{  }

		const boost::intrusive_ptr<const PropertyValue>
		value() const {
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
		const boost::intrusive_ptr<PropertyValue>
		value() {
			return d_value;
		}

		void
		set_value(
				boost::intrusive_ptr<PropertyValue> v) {
			d_value = v;
		}

		const boost::intrusive_ptr<const GmlTimeInstant>
		valid_time() const {
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
		const boost::intrusive_ptr<GmlTimeInstant>
		valid_time() {
			return d_valid_time;
		}

		void
		set_valid_time(
				boost::intrusive_ptr<GmlTimeInstant> vt) {
			d_valid_time = vt;
		}

		const boost::intrusive_ptr<const XsString>
		description() const {
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
		description() {
			return d_description;
		}

		void
		set_description(
				boost::intrusive_ptr<XsString> d) {
			d_description = d;
		}

		// Note that no "setter" is provided:  The value type of a GpmlTimeSample instance
		// should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_time_sample(*this);
		}

	private:

		// FIXME:  Is it valid for this pointer to be NULL?  I don't think so...
		boost::intrusive_ptr<PropertyValue> d_value;
		// FIXME:  Is it valid for this pointer to be NULL?  I don't think so...
		boost::intrusive_ptr<GmlTimeInstant> d_valid_time;
		// At least I know that *this* one (the description) is optional...
		boost::intrusive_ptr<XsString> d_description;
		TemplateTypeParameterType d_value_type;

	};

}

#endif  // GPLATES_MODEL_GPMLTIMESAMPLE_H
