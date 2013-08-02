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
			return get_current_revision<Revision>().xml_attributes;
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
		 * Base class inherited by derived revision classes (in derived properties) where
		 * mutable/revisionable property state is stored so it can be revisioned.
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

			/**
			 * Create a duplicate of this Revision instance, including a recursive copy
			 * of any property values in this instance.
			 *
			 * This is used when cloning a property instance because we want an entirely
			 * new property instance that does not share anything with the original instance.
			 * This is useful when cloning a feature to get an entirely new feature (and not a
			 * revision of the feature).
			 */
			virtual
			non_null_ptr_type
			clone() const = 0;


			/**
			 * Same as @a clone but does not clone property values since they are independently revisioned.
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
			 * Changes this revision so that it references the new bubbled-up property value revision.
			 */
			virtual
			void
			reference_bubbled_up_property_value_revision(
					const PropertyValue::RevisionedReference &property_value_revisioned_reference) = 0;


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
				return xml_attributes == other.xml_attributes;
			}

			xml_attributes_type xml_attributes;

		protected:

			explicit
			Revision(
					const xml_attributes_type &xml_attributes_) :
				xml_attributes(xml_attributes_)
			{  }

			Revision(
					const Revision &other) :
				xml_attributes(other.xml_attributes)
			{  }
		};

	public:
		//
		// This public interface is used by the model framework.
		//


		/**
		 * Reference to a top level property and one of its revision snapshots.
		 *
		 * Reference can later be used to restore the property to the revision (eg, undo/redo).
		 */
		class RevisionedReference
		{
		public:
			RevisionedReference(
					const non_null_ptr_type &top_level_property,
					const Revision::non_null_ptr_type &revision) :
				d_top_level_property(top_level_property),
				d_revision(revision)
			{  }

			non_null_ptr_to_const_type
			get_top_level_property() const
			{
				return d_top_level_property;
			}

			non_null_ptr_type
			get_top_level_property()
			{
				return d_top_level_property;
			}

			/**
			 * Sets the revision as the current revision of the property.
			 */
			void
			set_current_revision()
			{
				d_top_level_property->d_current_revision = d_revision;
			}

		private:
			non_null_ptr_type d_top_level_property;
			Revision::non_null_ptr_type d_revision;
		};


		/**
		 * Returns the current revision of this top level property.
		 *
		 * The returned revision is immutable since it has already been initialised and
		 * once initialised it cannot be modified. A top level property modification involves
		 * creating a new revision object.
		 */
		RevisionedReference
		get_current_revisioned_reference()
		{
			return RevisionedReference(this, d_current_revision);
		}

		/**
		 * Sets current parent to the specified @a FeatureHandle.
		 *
		 * This method is useful when adding a top level property to a feature.
		 */
		void
		set_parent(
				FeatureHandle &parent)
		{
			d_current_parent = parent;
		}

		/**
		 * Removes the parent reference (useful when removing a top level property from a feature).
		 */
		void
		unset_parent()
		{
			d_current_parent = boost::none;
		}

		/**
		 * Bubbles up a modification from a child property value (initiated by child).
		 *
		 * The bubble-up mechanism creates a new revision and then bubbles up to our parent and
		 * so on to eventually reach the top of the model hierarchy (feature store) if connected
		 * all the way up.
		 */
		void
		bubble_up_modification(
				const PropertyValue::RevisionedReference &property_value_revisioned_reference,
				ModelTransaction &transaction);

	protected:

		/**
		 * A convenience helper class used by derived top level property classes in their methods
		 * that modify the property state.
		 */
		class MutableRevisionHandler :
				private boost::noncopyable
		{
		public:

			explicit
			MutableRevisionHandler(
					const non_null_ptr_type &top_level_property);

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
			 * Derived top level property classes modify the data in the returned derived revision class.
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
			non_null_ptr_type d_top_level_property;
			Revision::non_null_ptr_type d_mutable_revision;
		};


		/**
		 * Construct a TopLevelProperty instance.
		 */
		TopLevelProperty(
				const PropertyName &property_name_,
				const Revision::non_null_ptr_type &revision_) :
			d_property_name(property_name_), // immutable/unrevisioned
			d_current_revision(revision_)
		{  }

		/**
		 * Construct a TopLevelProperty instance which is a copy of @a other.
		 *
		 * This ctor should only be invoked by the @a clone_impl member function which will create
		 * a duplicate instance and return a new non_null_intrusive_ptr reference to the new duplicate.
		 */
		TopLevelProperty(
				const TopLevelProperty &other) :
			d_property_name(other.d_property_name), // immutable/unrevisioned
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
		 * Returns the current 'non-const' revision as the specified derived revision type.
		 *
		 * NOTE: This should *not* be used to change the data in the current revision.
		 * Use @a MutableRevisionHandler to modify revisions.
		 */
		template <class RevisionType>
		RevisionType &
		get_current_revision()
		{
			return dynamic_cast<RevisionType &>(*d_current_revision);
		}

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

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		TopLevelProperty &
		operator=(
				const TopLevelProperty &);


		/**
		 * The property name is not revisioned since it doesn't change - is essentially immutable.
		 */
		PropertyName d_property_name;

		/**
		 * The current revision of this property value.
		 *
		 * The current revision is immutable since it has already been initialised and once
		 * initialised it cannot be modified. A modification involves creating a new revision object.
		 * However we use a pointer to 'non-const' revision because there are some situations
		 * where we need 'non-const' access to the property values even though we're not
		 * modifying the @a Revision object itself.
		 */
		Revision::non_null_ptr_type d_current_revision;

		/**
		 * The reference to the owning feature (is none if not owned/attached).
		 */
		boost::optional<FeatureHandle &> d_current_parent;

	};

	std::ostream &
	operator<<(
			std::ostream & os,
			const TopLevelProperty &top_level_prop);

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTY_H
