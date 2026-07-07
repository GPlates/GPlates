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
#include <boost/optional.hpp>

#include "PropertyName.h"
#include "Revision.h"
#include "Revisionable.h"
#include "types.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"

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
			public Revisionable,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<TopLevelProperty>
	{
	public:

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty>.
		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelProperty> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const TopLevelProperty>.
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
			return GPlatesUtils::dynamic_pointer_cast<TopLevelProperty>(clone_impl());
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
				const xml_attributes_type &xml_attributes_);

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

	protected:

		/**
		 * Construct a TopLevelProperty instance directly.
		 */
		TopLevelProperty(
				const PropertyName &property_name_,
				const Revision::non_null_ptr_type &revision_) :
			Revisionable(revision_),
			d_property_name(property_name_) // immutable/unrevisioned
		{  }

		/**
		 * Construct a TopLevelProperty instance using another TopLevelProperty.
		 */
		TopLevelProperty(
				const TopLevelProperty &other,
				const Revision::non_null_ptr_type &revision_) :
			Revisionable(revision_),
			d_property_name(other.d_property_name) // immutable/unrevisioned
		{  }

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const TopLevelProperty &other_pv = dynamic_cast<const TopLevelProperty &>(other);

			return d_property_name == other_pv.d_property_name &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
		}


		/**
		 * Top-level property data that is mutable/revisionable.
		 */
		class Revision :
				public GPlatesModel::Revision
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<Revision> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Revision> non_null_ptr_to_const_type;

			/**
			 * The type of the container of XML attributes.
			 */
			typedef std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_type;


			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return xml_attributes == other_revision.xml_attributes;
			}


			/**
			 * XML attributes.
			 */
			xml_attributes_type xml_attributes;

		protected:

			/**
			 * Constructor specified optional (parent) context in which this top-level property (revision) is nested.
			 */
			explicit
			Revision(
					const xml_attributes_type &xml_attributes_,
					boost::optional<RevisionContext &> context = boost::none) :
				GPlatesModel::Revision(context),
				xml_attributes(xml_attributes_)
			{  }

			/**
			 * Construct a Revision instance using another Revision.
			 */
			Revision(
					const Revision &other,
					boost::optional<RevisionContext &> context) :
				GPlatesModel::Revision(context),
				xml_attributes(other.xml_attributes)
			{  }

		};

		/**
		 * The property name is not revisioned since it doesn't change - is essentially immutable.
		 *
		 * If it does become mutable then it should be moved into the revision.
		 */
		PropertyName d_property_name;

	};

	std::ostream &
	operator<<(
			std::ostream & os,
			const TopLevelProperty &top_level_prop);

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTY_H
