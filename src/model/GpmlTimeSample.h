/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

		boost::intrusive_ptr<const PropertyValue>
		value() const {
			return d_value;
		}

		boost::intrusive_ptr<PropertyValue>
		value() {
			return d_value;
		}

		boost::intrusive_ptr<const GmlTimeInstant>
		valid_time() const {
			return d_valid_time;
		}

		boost::intrusive_ptr<GmlTimeInstant>
		valid_time() {
			return d_valid_time;
		}

		boost::intrusive_ptr<const XsString>
		description() const {
			return d_description;
		}

		boost::intrusive_ptr<XsString>
		description() {
			return d_description;
		}

		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		TemplateTypeParameterType &
		value_type() {
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
