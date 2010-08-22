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


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
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
		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const std::pair<double, double> &gml_pos,
				GmlProperty gml_property_ = POS);

		/**
		 * Create a GmlPoint instance from a GPlatesMaths::PointOnSphere instance.
		 */
		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::PointOnSphere &p,
				GmlProperty gml_property_ = POS);

		/**
		 * Create a GmlPoint instance from a non-null-intrusive-pointer to a
		 * GPlatesMaths::PointOnSphere.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p,
				GmlProperty gml_property_ = POS)
		{
			GmlPoint::non_null_ptr_type point_ptr(
					new GmlPoint(p, gml_property_));
			return point_ptr;
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
		 * Access the GPlatesMaths::PointOnSphere which encodes the geometry of this
		 * instance.
		 *
		 * Note that there is no accessor provided which returns a boost::intrusive_ptr to
		 * a non-const GPlatesMaths::PointOnSphere.  The GPlatesMaths::PointOnSphere within
		 * this instance should not be modified directly; to alter the
		 * GPlatesMaths::PointOnSphere within this instance, set a new value using the
		 * function @a set_point below.
		 */
		const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere>
		point() const
		{
			return d_point;
		}

		/**
		 * Returns the point as a lat-lon point.
		 *
		 * Prefer using this where possible instead of calling point() and then
		 * converting it using GPlatesMaths::make_lat_lon_point. This is because, if
		 * the point was constructed using lat-lon and the lat is 90 or -90, the
		 * longitude information is lost in the conversion. This function, however,
		 * will use the saved longitude where possible.
		 */
		GPlatesMaths::LatLonPoint
		point_in_lat_lon() const;

		/**
		 * Set the point within this instance to @a p.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_point(
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p)
		{
			d_point = p;
			d_original_longitude = boost::none;
			update_instance_id();
		}

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
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> point_,
				GmlProperty gml_property_):
			PropertyValue(),
			d_point(point_),
			d_gml_property(gml_property_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlPoint(
				const GmlPoint &other):
			PropertyValue(other), /* share instance id */
			d_point(other.d_point),
			d_gml_property(other.d_gml_property)
		{  }

	private:

		GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> d_point;
		GmlProperty d_gml_property;

		/**
		 * This is a hack to remember the original longitude that we were given in the
		 * version of create() that takes a std::pair<double, double>.
		 *
		 * This is necessary when the latitude is 90 or -90, because we lose longitude
		 * information when the lat-lon gets converted to a 3D vector (all points with
		 * latitude of 90 are the exact same point on the 3D sphere, the north pole).
		 * While this might not matter in many cases, there are times when we care
		 * about the original longitude, in particular in storing the origin of a
		 * georeferenced raster.
		 */
		boost::optional<double> d_original_longitude;

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
