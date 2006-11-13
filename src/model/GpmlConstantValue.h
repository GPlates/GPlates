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

#ifndef GPLATES_MODEL_GPMLCONSTANTVALUE_H
#define GPLATES_MODEL_GPMLCONSTANTVALUE_H

#include <boost/intrusive_ptr.hpp>
#include "PropertyValue.h"
#include "TemplateTypeParameterType.h"
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

	class GpmlConstantValue :
			public PropertyValue {

	public:

		virtual
		~GpmlConstantValue() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GpmlConstantValue>
		create(
				boost::intrusive_ptr<PropertyValue> value_,
				const TemplateTypeParameterType &value_type_) {
			boost::intrusive_ptr<GpmlConstantValue> ptr(new GpmlConstantValue(value_, value_type_));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GpmlConstantValue(*this));
			return dup;
		}

		boost::intrusive_ptr<const PropertyValue>
		value() const {
			return d_value;
		}

		boost::intrusive_ptr<PropertyValue>
		value() {
			return d_value;
		}

		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		TemplateTypeParameterType &
		value_type() {
			return d_value_type;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_constant_value(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlConstantValue(
				boost::intrusive_ptr<PropertyValue> value_,
				const TemplateTypeParameterType &value_type_):
			PropertyValue(),
			d_value(value_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlConstantValue(
				const GpmlConstantValue &other) :
			PropertyValue(),
			d_value(other.d_value),
			d_value_type(other.d_value_type)
		{  }

	private:

		// FIXME:  Is it valid for this pointer to be NULL?  I don't think so...
		boost::intrusive_ptr<PropertyValue> d_value;
		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlConstantValue &
		operator=(const GpmlConstantValue &);

	};

}

#endif  // GPLATES_MODEL_GPMLCONSTANTVALUE_H
