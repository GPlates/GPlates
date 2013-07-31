/* $Id$ */

/**
 * \file 
 * Contains the definition of the class TopLevelProperty.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
 *  (under the name "PropertyContainer.h")
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 *  (under the name "TopLevelProperty.h")
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

#ifndef GPLATES_MODEL_TOPLEVELPROPERTY_H
#define GPLATES_MODEL_TOPLEVELPROPERTY_H

#include <iosfwd>
#include <map>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "PropertyName.h"
#include "PropertyValue.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"
#include "types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/QtStreamable.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class FeatureHandle;
	template<class H> class FeatureVisitorBase;
	typedef FeatureVisitorBase<FeatureHandle> FeatureVisitor;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;
	class Model;
	class ModelTransaction;

	/**
	 * This abstract base class (ABC) represents the top-level property of a feature.
	 *
	 * Currently, there is one derivation of this ABC: TopLevelPropertyInline, which contains a
	 * property-value inline.  In the future, there may be a TopLevelPropertyXlink, which uses
	 * a GML Xlink to reference a remote property.
	 */
	class TopLevelProperty:
			public GPlatesUtils::ReferenceCount<TopLevelProperty>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<TopLevelProperty>,
			public boost::equality_comparable<TopLevelProperty>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const TopLevelProperty>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopLevelProperty> non_null_ptr_to_const_type;

		/**
		 * The type of the container of XML attributes.
		 */
		typedef std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_type;


		virtual
		~TopLevelProperty()
		{  }

		/**
		 * Create a duplicate of this TopLevelProperty instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		const non_null_ptr_type
		clone() const
		{
			return clone_impl();
		}

		// Note that no "setter" is provided:  The property name of a TopLevelProperty
		// instance should never be changed.
		const PropertyName &
		property_name() const
		{
			return d_property_name;
		}

		/**
		 * Return the XML attributes.
		 *
		 * There is no setter method since @a TopLevelProperty (and derived classes)
		 * have not implemented revisioning - if they are made mutable (excluding any
		 * contained property values - which already have revisioning) then revisioning will
		 * need to be implemented for @a TopLevelProperty (and derived classes).
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the XML attribute map?  (For consistency with the non-const
		 * overload...)
		 */
		const xml_attributes_type &
		get_xml_attributes() const
		{
			return d_xml_attributes;
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
				ConstFeatureVisitor &visitor) const = 0;

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) = 0;

		/**
		 * Prints the contents of this TopLevelProperty to the stream @a os.
		 *
		 * Note: This function is not called operator<< because operator<< needs to
		 * be a non-member operator, but we would like polymorphic behaviour.
		 */
		virtual
		std::ostream &
		print_to(
				std::ostream &os) const = 0;

		/**
		 * Returns a (non-const) reference to the Model to which this property belongs.
		 *
		 * Returns none if this property is not currently attached to the model - this can happen
		 * if this property has no parent (feature) or if the parent has no parent, etc.
		 */
		boost::optional<Model &>
		get_model();

		/**
		 * Returns a const reference to the Model to which this property belongs.
		 *
		 * Returns none if this property is not currently attached to the model - this can happen
		 * if this property has no parent (feature) or if the parent has no parent, etc.
		 */
		boost::optional<const Model &>
		get_model() const;

		/**
		 * Value equality comparison operator.
		 *
		 * Returns false if the types of @a other and 'this' aren't the same type, otherwise
		 * returns true if their values (tested recursively as needed) compare equal.
		 *
		 * Inequality provided by boost equality_comparable.
		 */
		bool
		operator==(
				const TopLevelProperty &other) const;

	public:
		//
		// This public interface is used by the model framework.
		//


		void
		bubble_up_modification(
				const PropertyValue::RevisionedReference &revision,
				ModelTransaction &transaction);

	protected:

		/**
		 * Construct a TopLevelProperty instance with the given property name.
		 */
		TopLevelProperty(
				const PropertyName &property_name_,
				const xml_attributes_type &xml_attributes_):
			d_property_name(property_name_),
			d_xml_attributes(xml_attributes_)
		{  }

		/**
		 * Construct a TopLevelProperty instance which is a copy of @a other.
		 *
		 * This ctor should only be invoked by the @a clone_impl member function which will create
		 * a duplicate instance and return a new non_null_intrusive_ptr reference to the new duplicate.
		 */
		TopLevelProperty(
				const TopLevelProperty &other) :
			d_property_name(other.d_property_name),
			d_xml_attributes(other.d_xml_attributes)
		{  }

		/**
		 * Create a duplicate of this TopLevelProperty instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		virtual
		const non_null_ptr_type
		clone_impl() const = 0;

		/**
		 * Determine if two property instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'object'
		 * so static_cast can be used instead of dynamic_cast.
		 */
		virtual
		bool
		equality(
				const TopLevelProperty &other) const;

	private:

		PropertyName d_property_name;
		xml_attributes_type d_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		TopLevelProperty &
		operator=(
				const TopLevelProperty &);

	};

	std::ostream &
	operator<<(
			std::ostream & os,
			const TopLevelProperty &top_level_prop);

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTY_H
