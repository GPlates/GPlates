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

#include <map>
#include <iosfwd>

#include "PropertyName.h"
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
			public GPlatesUtils::QtStreamable<TopLevelProperty>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const TopLevelProperty,
		 * 		GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopLevelProperty,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type of the container of XML attributes.
		 */
		typedef std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_type;

		virtual
		~TopLevelProperty()
		{  }

		/**
		 * Construct a TopLevelProperty instance with the given property name.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
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
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 *
		 * This ctor should only be invoked by the @a clone member function (pure virtual
		 * in this class; defined in derived classes), which will create a duplicate
		 * instance and return a new non_null_intrusive_ptr reference to the new duplicate.
		 * Since initially the only reference to the new duplicate will be the one returned
		 * by the @a clone function, *before* the new non_null_intrusive_ptr is created,
		 * the ref-count of the new TopLevelProperty instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		TopLevelProperty(
				const TopLevelProperty &other) :
			GPlatesUtils::ReferenceCount<TopLevelProperty>(),
			d_property_name(other.d_property_name),
			d_xml_attributes(other.d_xml_attributes)
		{  }

		/**
		 * Create a duplicate of this TopLevelProperty instance.
		 *
		 * Note that this will @em not duplicate any property values contained within it.
		 * As a result, the new (duplicate) instance will "contain" (reference by pointer)
		 * the same property values as the original.  (The pointer @em values are copied,
		 * not the pointer @em targets.)  Until the automatic Bubble-Up revisioning system
		 * is fully operational, this is probably @em not what you want, when you're cloning
		 * a feature:  If a property value is modified in the original, the duplicate will
		 * now also contain a modified property value...
		 *
		 * Compare with @a deep_clone, which @em does duplicate any property values
		 * contained within.
		 */
		virtual
		const non_null_ptr_type
		clone() const = 0;

		/**
		 * Create a duplicate of this TopLevelProperty instance, plus any property values
		 * which it might contain.
		 *
		 * Until the automatic Bubble-Up revisioning system is fully operational, this is
		 * the function you should call, when you're cloning a feature.
		 */
		virtual
		const non_null_ptr_type
		deep_clone() const = 0;

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
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the XML attribute map?
		 */
		const xml_attributes_type &
		xml_attributes() const
		{
			return d_xml_attributes;
		}

		/**
		 * Set the XML attributes.
		 */
		void
		set_xml_attributes(
				const xml_attributes_type &xml_attributes)
		{
			d_xml_attributes = xml_attributes;
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

		virtual
		bool
		operator==(
				const TopLevelProperty &other) const = 0;

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
