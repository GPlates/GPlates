/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway.
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

#ifndef GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H
#define GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H

#include <vector>

#include "model/PropertyValue.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"

namespace GPlatesPropertyValues {

	class GpmlKeyValueDictionaryElement;

	class GpmlKeyValueDictionary :
			public GPlatesModel::PropertyValue {

	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionary,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionary,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionary,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionary,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		virtual
		~GpmlKeyValueDictionary() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create() {
			non_null_ptr_type ptr(new GpmlKeyValueDictionary(),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
			const std::vector<GpmlKeyValueDictionaryElement> &elements) {
			non_null_ptr_type ptr(new GpmlKeyValueDictionary(
					elements),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}


		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(
					new GpmlKeyValueDictionary(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		const std::vector<GpmlKeyValueDictionaryElement> &
		elements() const {
				return d_elements;
		}

		std::vector<GpmlKeyValueDictionaryElement> &
		elements() {
				return d_elements;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_key_value_dictionary(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gpml_key_value_dictionary(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionary():
			PropertyValue()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionary(
			const std::vector<GpmlKeyValueDictionaryElement> &elements_):
			PropertyValue(),
			d_elements(elements_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlKeyValueDictionary(
				const GpmlKeyValueDictionary &other) :
			PropertyValue()
		{  }

	private:

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> d_elements;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlKeyValueDictionary &
		operator=(const GpmlKeyValueDictionary &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H
