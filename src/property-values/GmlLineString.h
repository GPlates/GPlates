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

#ifndef GPLATES_PROPERTYVALUES_GMLLINESTRING_H
#define GPLATES_PROPERTYVALUES_GMLLINESTRING_H

#include <vector>

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "maths/PolylineOnSphere.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlLineString, visit_gml_line_string)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:LineString".
	 */
	class GmlLineString:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlLineString>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlLineString> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlLineString>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlLineString> non_null_ptr_to_const_type;


		/**
		 * A convenience typedef for the internal polyline representation.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> polyline_type;


		virtual
		~GmlLineString()
		{  }

		/**
		 * Create a GmlLineString instance which contains a copy of @a polyline_.
		 */
		static
		const non_null_ptr_type
		create(
				const polyline_type &polyline_)
		{
			// Because PolylineOnSphere can only ever be handled via a non_null_ptr_to_const_type,
			// there is no way a PolylineOnSphere instance can be changed.  Hence, it is safe to store
			// a pointer to the instance which was passed into this 'create' function.
			return non_null_ptr_type(new GmlLineString(polyline_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlLineString>(clone_impl());
		}

		/**
		 * Access the GPlatesMaths::PolylineOnSphere which encodes the geometry of this instance.
		 */
		const polyline_type
		get_polyline() const
		{
			return get_current_revision<Revision>().polyline;
		}

		/**
		 * Set the polyline within this instance to @a p.
		 */
		void
		set_polyline(
				const polyline_type &p);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("LineString");
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
			visitor.visit_gml_line_string(*this);
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
			visitor.visit_gml_line_string(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlLineString(
				const polyline_type &polyline_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(polyline_)))
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GmlLineString(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const polyline_type &polyline_) :
				polyline(polyline_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				// Note that the default copy constructor (shallow copy) is fine because
				// PolylineOnSphere is immutable and hence can be shared without copy-on-write.
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *polyline == *other_revision.polyline &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			polyline_type polyline;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLLINESTRING_H
