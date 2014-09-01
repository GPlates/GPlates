/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_PROPERTYVALUES_GMLPOINT_H
#define GPLATES_PROPERTYVALUES_GMLPOINT_H

#include <utility>  /* std::pair */
#include <boost/optional.hpp>

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlPoint, visit_gml_point)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:Point".
	 */
	class GmlPoint:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlPoint> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlPoint> non_null_ptr_to_const_type;

		virtual
		~GmlPoint()
		{  }

		/**
		 * In GML 3.0, the whereabouts of a gml:Point can be specified using the "pos"
		 * property or the "coordinates" property.
		 *
		 * There are minor semantic differences between the two so it's probably best
		 * to preserve which property was used originally in the file.
		 *
		 * Examples:
		 *
		 *	<gml:Point>
		 *		<gml:pos>12.3 45.6</gml:pos>
		 *	</gml:Point>
		 *
		 *	<gml:Point>
		 *		<gml:coordinates>2,1</gml:coordinates>
		 *	</gml:Point>
		 */
		enum GmlProperty
		{
			POS,
			COORDINATES
		};

		/**
		 * Create a GmlPoint instance from a (longitude, latitude) coordinate duple.
		 *
		 * This coordinate duple corresponds to the contents of the "gml:pos" property in a
		 * "gml:Point" structural-type.  The first element in the pair is expected to be a
		 * longitude value; the second is expected to be a latitude.  This is the form used
		 * in GML.
		 */
		static
		const non_null_ptr_type
		create_from_lon_lat(
				const std::pair<double/*lon*/, double/*lat*/> &gml_pos,
				GmlProperty gml_property_ = POS)
		{
			return create_from_pos_2d(
					std::make_pair(gml_pos.second/*lat*/, gml_pos.first/*lon*/),
					gml_property_);
		}

		/**
		 * Create a GmlPoint instance from a 2D coordinate duple.
		 *
		 * This coordinate duple corresponds to the contents of the "gml:pos" property in a
		 * "gml:Point" structural-type.  There is no assumption that the position corresponds
		 * to latitude and longitude coordinates.
		 *
		 * This is useful for storing georeferenced coordinates that are not necessarily
		 * in a latitude/longitude coordinate system. For example, the coordinates might be in
		 * a *projection* coordinate system which can be outside valid latitude/longitude ranges.
		 *
		 * NOTE: If @a pos_2d is to be interpreted as latitude and longitude then the order is
		 * (lat, lon) which is the order GPML stores, but is the reverse of the order specified
		 * to @a create_from_lon_lat (which is GML order).
		 */
		static
		const non_null_ptr_type
		create_from_pos_2d(
				const std::pair<double, double> &pos_2d,
				GmlProperty gml_property_ = POS)
		{
			return non_null_ptr_type(new GmlPoint(pos_2d, gml_property_));
		}

		/**
		 * Create a GmlPoint instance from a GPlatesMaths::PointOnSphere instance.
		 */
		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::PointOnSphere &p,
				GmlProperty gml_property_ = POS)
		{
			return create(p.clone_as_point(), gml_property_);
		}

		/**
		 * Create a GmlPoint instance from a non-null-intrusive-pointer to a
		 * GPlatesMaths::PointOnSphere.
		 */
		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &p,
				GmlProperty gml_property_ = POS)
		{
			return non_null_ptr_type(new GmlPoint(p, gml_property_));
		}

		const GmlPoint::non_null_ptr_type
		clone() const
		{
			GmlPoint::non_null_ptr_type dup(new GmlPoint(*this));
			return dup;
		}

		const GmlPoint::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Access the GPlatesMaths::PointOnSphere which encodes the geometry of this instance.
		 *
		 * @throws InvalidLatLonException if the (longitude, latitude) values are out of range.
		 * This can happen if this instance was created using @a create_from_pos_2d with arguments
		 * that are not latitude and longitude - see @a create_from_pos_2d for more details.
		 */
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		point() const;

		/**
		 * Returns the point as a lat-lon point.
		 *
		 * Prefer using this where possible instead of calling point() and then
		 * converting it using GPlatesMaths::make_lat_lon_point. This is because, if
		 * the point was constructed using lat-lon and the lat is 90 or -90, the
		 * longitude information is lost in the conversion. This function, however,
		 * will use the saved longitude where possible.
		 *
		 * @throws InvalidLatLonException if the (longitude, latitude) values are out of range.
		 * This can happen if this instance was created using @a create_from_pos_2d with arguments
		 * that are not latitude and longitude - see @a create_from_pos_2d for more details.
		 */
		GPlatesMaths::LatLonPoint
		point_in_lat_lon() const;

		/**
		 * Returns the point as a 2D (x,y) point.
		 *
		 * NOTE: If the returned point is to be interpreted as latitude and longitude then the order
		 * is (lat, lon) which is the order GPML stores, but is the reverse of the order specified
		 * to @a create_from_lon_lat (which is GML order).
		 *
		 * See @a create_from_pos_2d for more details.
		 */
		const std::pair<double, double> &
		point_2d() const;

		/**
		 * Set the point within this instance to @a p.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_point(
				const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &p);

		GmlProperty
		gml_property() const
		{
			return d_gml_property;
		}

		void
		set_gml_property(
				GmlProperty gml_property_)
		{
			d_gml_property = gml_property_;
			update_instance_id();
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("Point");
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
			visitor.visit_gml_point(*this);
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
			visitor.visit_gml_point(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlPoint(
				const std::pair<double, double> &point_2d_,
				GmlProperty gml_property_) :
			d_gml_property(gml_property_),
			d_point_2d(point_2d_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlPoint(
				const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &point_on_sphere_,
				GmlProperty gml_property_) :
			d_gml_property(gml_property_),
			d_point_on_sphere(point_on_sphere_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlPoint(
				const GmlPoint &other):
			PropertyValue(other), /* share instance id */
			d_gml_property(other.d_gml_property),
			d_point_2d(other.d_point_2d),
			d_point_on_sphere(other.d_point_on_sphere)
		{  }

	private:

		GmlProperty d_gml_property;

		// One of these will always exist depending on how this instance was created.

		mutable boost::optional< std::pair<double, double> > d_point_2d;
		mutable boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> d_point_on_sphere;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlPoint &
		operator=(
				const GmlPoint &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLPOINT_H
