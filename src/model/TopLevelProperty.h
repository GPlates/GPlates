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
#include "TopLevelPropertyRevision.h"
#include "types.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"

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
	class TopLevelPropertyBubbleUpRevisionHandler;

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
		get_property_name() const
		{
			return d_property_name;
		}

		/**
		 * Return the XML attributes.
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the XML attribute map?
		 */
		const xml_attributes_type &
		get_xml_attributes() const
		{
			return get_current_revision<TopLevelPropertyRevision>().xml_attributes;
		}

		/**
		 * Set the XML attributes.
		 */
		void
		set_xml_attributes(
				const xml_attributes_type &xml_attributes);

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

	protected:

		/**
		 * Construct a TopLevelProperty instance directly.
		 */
		TopLevelProperty(
				const PropertyName &property_name_,
				const TopLevelPropertyRevision::non_null_ptr_type &revision_) :
			d_property_name(property_name_), // immutable/unrevisioned
			d_current_revision(revision_)
		{  }

		/**
		 * Construct a TopLevelProperty instance using another TopLevelProperty.
		 */
		TopLevelProperty(
				const TopLevelProperty &other,
				const TopLevelPropertyRevision::non_null_ptr_type &revision_) :
			d_property_name(other.d_property_name), // immutable/unrevisioned
			d_current_revision(revision_)
		{  }

		/**
		 * NOTE: Copy-constructor is intentionally not defined anywhere (not strictly necessary to do
		 * this since base class ReferenceCount is already non-copyable, but makes it more obvious).
		 *
		 * Use the constructor (accepting revision) when cloning a top-level property.
		 * Note that two top-level property instances should not point to the same revision instance as
		 * this will prevent the revisioning system from functioning correctly - this doesn't mean
		 * that two top-level property *revision* instances can't reference the same revision instance though.
		 */
		TopLevelProperty(
				const TopLevelProperty &other);

		/**
		 * Returns the current immutable revision as the base revision type.
		 *
		 * Revisions are immutable - use @a TopLevelPropertyBubbleUpRevisionHandler to modify revisions.
		 */
		TopLevelPropertyRevision::non_null_ptr_to_const_type
		get_current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Returns the current immutable revision as the specified derived revision type.
		 *
		 * Revisions are immutable - use @a TopLevelPropertyBubbleUpRevisionHandler to modify revisions.
		 */
		template <class RevisionType>
		const RevisionType &
		get_current_revision() const
		{
			return dynamic_cast<const RevisionType &>(*d_current_revision);
		}

		/**
		 * Create a new bubble-up revision by delegating to the (parent) revision context
		 * if there is one, otherwise create a new revision without any context.
		 */
		TopLevelPropertyRevision::non_null_ptr_type
		create_bubble_up_revision(
				ModelTransaction &transaction) const;

		/**
		 * Same as the other overload of @a create_bubble_up_revision but downcasts to
		 * the specified derived revision type.
		 */
		template <class RevisionType>
		RevisionType &
		create_bubble_up_revision(
				ModelTransaction &transaction) const
		{
			// The returned revision is kept alive by either the model transaction (if uncommitted),
			// or 'this' top-level property (if committed).
			return dynamic_cast<RevisionType &>(*create_bubble_up_revision(transaction));
		}

		/**
		 * Create a duplicate of this TopLevelProperty instance, including a recursive copy
		 * of any property values this instance contains.
		 *
		 * @a context is non-null if this top-level property is nested within a parent feature handle.
		 */
		virtual
		const non_null_ptr_type
		clone_impl(
				boost::optional<FeatureHandle &> context = boost::none) const = 0;

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

		/**
		 * The property name is not revisioned since it doesn't change - is essentially immutable.
		 *
		 * If it does become mutable then it should be moved into the revision.
		 */
		PropertyName d_property_name;

		/**
		 * The current revision of this property.
		 *
		 * The current revision is immutable since it has already been initialised and once
		 * initialised it cannot be modified. A modification involves creating a new revision object.
		 * Keeping the current revision 'const' prevents inadvertent modifications by derived
		 * top-level property classes.
		 *
		 * The revision also contains the current parent reference such that when a different
		 * revision is swapped in (due to undo/redo) it will automatically reference the correct parent.
		 *
		 * The pointer is declared 'mutable' so that revisions can be swapped in 'const' top-level properties.
		 */
		mutable TopLevelPropertyRevision::non_null_ptr_to_const_type d_current_revision;


		friend class ModelTransaction;
		friend class TopLevelPropertyBubbleUpRevisionHandler;

	};

	std::ostream &
	operator<<(
			std::ostream & os,
			const TopLevelProperty &top_level_prop);

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTY_H
