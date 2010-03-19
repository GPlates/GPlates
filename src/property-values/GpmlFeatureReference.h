/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLFEATUREREFERENCE_H
#define GPLATES_PROPERTYVALUES_GPMLFEATUREREFERENCE_H

#include "TemplateTypeParameterType.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "model/FeatureId.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFeatureReference, visit_gpml_feature_reference)

namespace GPlatesPropertyValues {

	class GpmlFeatureReference:
			public GPlatesModel::PropertyValue {

	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureReference,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureReference,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureReference,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureReference,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		virtual
		~GpmlFeatureReference() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureId &feature_,
				const TemplateTypeParameterType &value_type_) {
			non_null_ptr_type ptr(new GpmlFeatureReference(feature_, value_type_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		const GpmlFeatureReference::non_null_ptr_type
		clone() const {
			GpmlFeatureReference::non_null_ptr_type dup(new GpmlFeatureReference(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		const GpmlFeatureReference::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const GPlatesModel::FeatureId
		feature_id() const {
			return d_feature;
		}

		// Note that, because the copy-assignment operator of PropertyValue is private,
		// the PropertyValue referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the PropertyValue within this GpmlFeatureReference instance.  (This
		// restriction is intentional.)
		//
		// To switch the PropertyValue within this GpmlFeatureReference instance, use the
		// function @a set_value below.
		//
		// (This overload is provided to allow the referenced PropertyValue instance to
		// accept a FeatureVisitor instance.)
		const GPlatesModel::FeatureId
		feature_id() {
			return d_feature;
		}


		// Note that no "setter" is provided:  The value type of a GpmlFeatureReference
		// instance should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
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
			visitor.visit_gpml_feature_reference(*this);
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
			visitor.visit_gpml_feature_reference(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlFeatureReference(
				const GPlatesModel::FeatureId &feature_,
				const TemplateTypeParameterType &value_type_):
			PropertyValue(),
			d_feature(feature_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlFeatureReference(
				const GPlatesModel::FeatureId &feature_,
				const TemplateTypeParameterType &value_type_,
				const UnicodeString &description_):
			PropertyValue(),
			d_feature(feature_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlFeatureReference(
				const GpmlFeatureReference &other) :
			PropertyValue(),
			d_feature(other.d_feature),
			d_value_type(other.d_value_type)
		{  }

	private:

		GPlatesModel::FeatureId d_feature;
		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlFeatureReference &
		operator=(const GpmlFeatureReference &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLFEATUREREFERENCE_H
