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

#ifndef GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
#define GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H

#include <vector>

#include "GmlPoint.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "model/PropertyValue.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlMultiPoint, visit_gml_multi_point)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:MultiPoint".
	 */
	class GmlMultiPoint:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint> non_null_ptr_to_const_type;


		/**
		 * A convenience typedef for the internal multipoint representation.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere> multipoint_type;


		virtual
		~GmlMultiPoint()
		{  }

		/**
		 * Create a GmlMultiPoint instance which contains a copy of @a multipoint_.
		 *
		 * All points are presumed to have property gml:pos (as opposed to gml:coordinates).
		 */
		static
		const non_null_ptr_type
		create(
				const multipoint_type &multipoint_)
		{
			return non_null_ptr_type(new GmlMultiPoint(multipoint_));
		}

		/**
		 * Create a GmlMultiPoint instance which contains a copy of @a multipoint_.
		 *
		 * The property with which each point was specified (gml:pos or gml:coordinates)
		 * is specified in @a gml_properties_. The size of @a gml_properties_ must be
		 * the same as @a multipoint_.
		 */
		static
		const non_null_ptr_type
		create(
				const multipoint_type &multipoint_,
				const std::vector<GmlPoint::GmlProperty> &gml_properties_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlMultiPoint>(clone_impl());
		}

		/**
		 * Access the GPlatesMaths::MultiPointOnSphere which encodes the geometry of this
		 * instance.
		 */
		const multipoint_type
		get_multipoint() const
		{
			return get_current_revision<Revision>().multipoint;
		}

		/**
		 * Set the GPlatesMaths::MultiPointOnSphere within this instance to @a p.
		 *
		 * Note: this sets all gml:Points to use gml:pos as their property.
		 */
		void
		set_multipoint(
				const multipoint_type &p);

		const std::vector<GmlPoint::GmlProperty> &
		get_gml_properties() const
		{
			return get_current_revision<Revision>().gml_properties;
		}

		void
		set_gml_properties(
				const std::vector<GmlPoint::GmlProperty> &gml_properties_);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("MultiPoint");
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
			visitor.visit_gml_multi_point(*this);
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
			visitor.visit_gml_multi_point(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlMultiPoint(
				const multipoint_type &multipoint_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(multipoint_)))
		{  }

		GmlMultiPoint(
				const multipoint_type &multipoint_,
				const std::vector<GmlPoint::GmlProperty> &gml_properties_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(multipoint_, gml_properties_)))
		{  }

		//! Constructor used when cloning.
		GmlMultiPoint(
				const GmlMultiPoint &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlMultiPoint(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					const multipoint_type &multipoint_) :
				multipoint(multipoint_)
			{
				fill_gml_properties();
			}

			Revision(
					const multipoint_type &multipoint_,
					const std::vector<GmlPoint::GmlProperty> &gml_properties_) :
				multipoint(multipoint_),
				gml_properties(gml_properties_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				// Note there is no need to distinguish between shallow and deep copying because
				// MultiPointOnSphere is immutable and hence there is never a need to deep copy it...
				multipoint(other_.multipoint),
				gml_properties(other_.gml_properties)
			{  }

			/**
			 * Fills d_gml_properties with multipoint.size() of GmlPoint::POS.
			 */
			void
			fill_gml_properties();

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *multipoint == *other_revision.multipoint &&
						gml_properties == other_revision.gml_properties &&
						PropertyValue::Revision::equality(other);
			}

			multipoint_type multipoint;

			// It's not the nicest OO, but this vector must be of the same size as d_multipoint.
			std::vector<GmlPoint::GmlProperty> gml_properties;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
