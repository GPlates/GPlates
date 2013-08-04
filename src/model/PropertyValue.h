/* $Id$ */

/**
 * \file 
 * Contains the definition of the class PropertyValue.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_PROPERTYVALUE_H
#define GPLATES_MODEL_PROPERTYVALUE_H

#include <iosfwd>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "property-values/StructuralType.h"

#include "utils/non_null_intrusive_ptr.h"
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
	class TopLevelProperty;

	/**
	 * This class is the abstract base of all property values.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class PropertyValue :
			public GPlatesUtils::ReferenceCount<PropertyValue>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<PropertyValue>,
			public boost::equality_comparable<PropertyValue>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PropertyValue>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyValue> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PropertyValue>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PropertyValue> non_null_ptr_to_const_type;


		virtual
		~PropertyValue()
		{  }

		/**
		 * Create a duplicate of this PropertyValue instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		const non_null_ptr_type
		clone() const
		{
			return clone_impl();
		}

		/**
		 * Returns the structural type associated with the type of the derived property value class.
		 *
		 * NOTE: This is actually a per-class, rather than per-instance, method but
		 * it's most accessible when implemented as a virtual method.
		 * Derived property value classes ideally should return a 'static' variable rather than
		 * an instance variable (data member) in order to reduce memory usage.
		 */
		virtual
		GPlatesPropertyValues::StructuralType
		get_structural_type() const = 0;

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
		 * Prints the contents of this PropertyValue to the stream @a os.
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
				const PropertyValue &other) const;

		/**
		 * Returns a (non-const) reference to the Model to which this property value belongs.
		 *
		 * Returns none if this property value is not currently attached to the model - this can happen
		 * if this property value has no parent (eg, top-level property) or if the parent has no parent, etc.
		 */
		boost::optional<Model &>
		get_model();

		/**
		 * Returns a const reference to the Model to which this property value belongs.
		 *
		 * Returns none if this property value is not currently attached to the model - this can happen
		 * if this property value has no parent (eg, top-level property) or if the parent has no parent, etc.
		 */
		boost::optional<const Model &>
		get_model() const;

	protected:

		/**
		 * Base class inherited by derived revision classes (in derived property values) where
		 * mutable/revisionable property value state is stored so it can be revisioned.
		 */
		class Revision :
				public GPlatesUtils::ReferenceCount<Revision>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<Revision> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Revision> non_null_ptr_to_const_type;

			virtual
			~Revision()
			{  }

			//! Default constructor - is needed since no implicit constructor generated because copy constructor defined.
			Revision()
			{  }

			//! Copy constructor - calls default constructor of ReferenceCount (non-copyable) base class.
			Revision(
					const Revision &other)
			{  }

			/**
			 * Create a duplicate of this Revision instance, including a recursive copy
			 * of any property values this instance might contain.
			 *
			 * This is used when cloning a property value instance because we want an entirely
			 * new property value instance that does not share anything with the original instance.
			 * This is useful when cloning a feature to get an entirely new feature (and not a
			 * revision of the feature).
			 */
			virtual
			non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision());
			}


			/**
			 * Same as @a clone but only clones those sub-objects that can, or will be, modified
			 * in preparation (to be followed by a bubble up through the model hierarchy
			 * potentially reaching the feature store if connected all the way up).
			 *
			 * Any sub-objects that will never be modified are considered immutable from a revisioning
			 * perspective and hence can be shared.
			 *
			 * This defaults to @a clone when not implemented in derived class.
			 */
			virtual
			non_null_ptr_type
			clone_for_bubble_up_modification() const
			{
				return clone();
			}


			/**
			 * Determine if two Revision instances ('this' and 'other') value compare equal.
			 *
			 * This should recursively test for equality as needed.
			 *
			 * A precondition of this method is that the type of 'this' is the same as the type of 'other'.
			 */
			virtual
			bool
			equality(
					const Revision &other) const
			{
				return true; // Terminates derived-to-base recursion.
			}
		};

	public:
		//
		// This public interface is used by the model framework.
		//


		/**
		 * Reference to a property value and one of its revision snapshots.
		 *
		 * Reference can later be used to restore the property value to the revision (eg, undo/redo).
		 */
		class RevisionedReference
		{
		public:
			RevisionedReference(
					const non_null_ptr_type &property_value,
					const Revision::non_null_ptr_to_const_type &revision) :
				d_property_value(property_value),
				d_revision(revision)
			{  }

			non_null_ptr_to_const_type
			get_property_value() const
			{
				return d_property_value;
			}

			non_null_ptr_type
			get_property_value()
			{
				return d_property_value;
			}

			/**
			 * Sets the revision as the current revision of the property value.
			 */
			void
			set_current_revision()
			{
				d_property_value->d_current_revision = d_revision;
			}

		private:
			non_null_ptr_type d_property_value;
			Revision::non_null_ptr_to_const_type d_revision;
		};


		/**
		 * Returns the current revision of this property value.
		 *
		 * The returned revision is immutable since it has already been initialised and
		 * once initialised it cannot be modified. A property value modification involves
		 * creating a new revision object.
		 */
		RevisionedReference
		get_current_revisioned_reference()
		{
			return RevisionedReference(this, d_current_revision);
		}

		/**
		 * Sets current parent to the specified @a TopLevelProperty.
		 *
		 * This method is useful when adding a property value to a top level property.
		 */
		void
		set_parent(
				TopLevelProperty &parent)
		{
			d_current_parent = parent;
		}

		/**
		 * Removes the parent reference (useful when removing a property value from a top level property).
		 */
		void
		unset_parent()
		{
			d_current_parent = boost::none;
		}

	protected:

		/**
		 * A convenience helper class used by derived property value classes in their methods
		 * that modify the property value state.
		 */
		class MutableRevisionHandler :
				private boost::noncopyable
		{
		public:

			explicit
			MutableRevisionHandler(
					const non_null_ptr_type &property_value);

			/**
			 * Returns the new mutable (base class) revision.
			 */
			Revision::non_null_ptr_type
			get_mutable_revision()
			{
				return d_mutable_revision;
			}

			/**
			 * Returns the new mutable revision (cast to specified derived revision type).
			 * Derived property value classes modify the data in the returned derived revision class.
			 */
			template <class RevisionType>
			RevisionType &
			get_mutable_revision()
			{
				return dynamic_cast<RevisionType &>(*d_mutable_revision);
			}

			/**
			 * Handles committing of revision to the model (if attached) and signaling model events.
			 */
			void
			handle_revision_modification();

		private:
			boost::optional<Model &> d_model;
			non_null_ptr_type d_property_value;
			Revision::non_null_ptr_type d_mutable_revision;
		};


		/**
		 * Construct a PropertyValue instance.
		 */
		PropertyValue(
				const Revision::non_null_ptr_type &revision) :
			d_current_revision(revision)
		{  }

		/**
		 * Copy-construct a PropertyValue instance.
		 *
		 * This ctor should only be invoked by the @a clone member function.
		 * We don't copy the parent feature reference because a cloned property value does
		 * not belong to the original feature (so initially it has no parent).
		 */
		PropertyValue(
				const PropertyValue &other) :
			d_current_revision(other.d_current_revision->clone())
		{  }

		/**
		 * Returns the current 'const' revision as the specified derived revision type.
		 *
		 * Revisions are essentially immutable (when attached to model).
		 * Use @a MutableRevisionHandler to modify revisions.
		 */
		template <class RevisionType>
		const RevisionType &
		get_current_revision() const
		{
			return dynamic_cast<const RevisionType &>(*d_current_revision);
		}

		/**
		 * Create a duplicate of this PropertyValue instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		virtual
		const non_null_ptr_type
		clone_impl() const = 0;

		/**
		 * Determine if two property value instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 * Note that the revision testing is done by PropertyValue::equality(), since the revisions
		 * are contained in class PropertyValue, so derived property values classes only need to test
		 * any non-revisioned data that they may contain - and if there is none then this method
		 * does not need to be implemented by that derived property value class.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'object'
		 * so static_cast can be used instead of dynamic_cast.
		 */
		virtual
		bool
		equality(
				const PropertyValue &other) const;

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyValue &
		operator=(
				const PropertyValue &);


		/**
		 * The current revision of this property value.
		 *
		 * The current revision is immutable since it has already been initialised and once
		 * initialised it cannot be modified. A modification involves creating a new revision object.
		 *
		 * The revision also contains the current parent reference such that when a different
		 * revision is swapped in (due to undo/redo) it will automatically reference the correct parent.
		 */
		Revision::non_null_ptr_to_const_type d_current_revision;

		/**
		 * The reference to the owning top level property (is none if not owned/attached).
		 */
		boost::optional<TopLevelProperty &> d_current_parent;
	};


	// operator<< for PropertyValue.
	std::ostream &
	operator<<(
			std::ostream &os,
			const PropertyValue &property_value);

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
