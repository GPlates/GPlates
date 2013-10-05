/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H
#define GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H

#include <map>

#include "GmlLineString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_revisionable() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlOrientableCurve, visit_gml_orientable_curve)

namespace GPlatesPropertyValues
{

	/**
	 * This class implements the PropertyValue which corresponds to "gml:OrientableCurve".
	 */
	class GmlOrientableCurve:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve> non_null_ptr_to_const_type;


		virtual
		~GmlOrientableCurve()
		{  }

		/**
		 * Create a GmlOrientableCurve instance which contains a "base curve".
		 *
		 * The "base curve" is passed in the parameter @a base_curve_, which is meant to
		 * refer to an object which is substitutable for "gml:_Curve".  (This won't however
		 * be verified at construction-time.)
		 *
		 * This instance will also contain the XML attributes in @a xml_attributes_.
		 */
		static
		const non_null_ptr_type
		create(
				GmlLineString::non_null_ptr_type base_curve_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlOrientableCurve>(clone_impl());
		}

		/**
		 * Access the 'const' PropertyValue which is the "base curve" of this instance.
		 */
		const GmlLineString::non_null_ptr_to_const_type
		base_curve() const
		{
			return get_current_revision<Revision>().base_curve.get_revisionable();
		}

		/**
		 * Access the 'non-const' PropertyValue which is the "base curve" of this instance.
		 */
		const GmlLineString::non_null_ptr_type
		base_curve()
		{
			return get_current_revision<Revision>().base_curve.get_revisionable();
		}

		/**
		 * Set the "base curve" of this instance to @a bc.
		 */
		void
		set_base_curve(
				GmlLineString::non_null_ptr_type bc);

		/**
		 * Return the map of XML attributes contained by this instance.
		 *
		 * This is the overloading of this function for const GmlOrientableCurve instances;
		 * it returns a reference to a const map, which in turn will only allow const
		 * access to its elements.
		 */
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		get_xml_attributes() const
		{
			return get_current_revision<Revision>().xml_attributes;
		}

		/**
		 * Set the map of XML attributes contained by this instance.
		 */
		void
		set_xml_attributes(
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("OrientableCurve");
			return STRUCTURAL_TYPE;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gml_orientable_curve(*this);
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
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gml_orientable_curve(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlOrientableCurve(
				GPlatesModel::ModelTransaction &transaction_,
				GmlLineString::non_null_ptr_type base_curve_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, base_curve_, xml_attributes_)))
		{  }

		//! Constructor used when cloning.
		GmlOrientableCurve(
				const GmlOrientableCurve &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlOrientableCurve(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					GmlLineString::non_null_ptr_type base_curve_,
					const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &xml_attributes_):
				base_curve(
						GPlatesModel::RevisionedReference<GmlLineString>::attach(
								transaction_, child_context_, base_curve_)),
				xml_attributes(xml_attributes_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				base_curve(other_.base_curve),
				xml_attributes(other_.xml_attributes)
			{
				// Clone data members that were not deep copied.
				base_curve.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				base_curve(other_.base_curve),
				xml_attributes(other_.xml_attributes)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *base_curve.get_revisionable() == *other_revision.base_curve.get_revisionable() &&
						xml_attributes == other_revision.xml_attributes &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GmlLineString> base_curve;
			std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H
