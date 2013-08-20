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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlPoint>.
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
			return non_null_ptr_type(new GmlPoint(p, gml_property_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlPoint>(clone_impl());
		}

		/**
		 * Access the GPlatesMaths::PointOnSphere which encodes the geometry of this instance.
		 */
		const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere>
		get_point() const
		{
			return get_current_revision<Revision>().point;
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
		 */
		void
		set_point(
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p);

		GmlProperty
		gml_property() const
		{
			return get_current_revision<Revision>().gml_property;
		}

		void
		set_gml_property(
				GmlProperty gml_property_);

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
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> point_,
				GmlProperty gml_property_,
				boost::optional<double> original_longitude_ = boost::none) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(point_, gml_property_, original_longitude_)))
		{  }

		//! Constructor used when cloning.
		GmlPoint(
				const GmlPoint &other_,
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlPoint(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			explicit
			Revision(
					const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> &point_,
					GmlProperty gml_property_,
					boost::optional<double> original_longitude_):
				point(point_),
				gml_property(gml_property_),
				original_longitude(original_longitude_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_),
				// Note there is no need to distinguish between shallow and deep copying because
				// PointOnSphere is immutable and hence there is never a need to deep copy it...
				point(other_.point),
				gml_property(other_.gml_property),
				original_longitude(other_.original_longitude)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *point == *other_revision.point &&
						gml_property == other_revision.gml_property &&
						original_longitude == other_revision.original_longitude &&
						PropertyValueRevision::equality(other);
			}

			// PointOnSphere is inherently immutable so we can share it across revisions.
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> point;
			GmlProperty gml_property;

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
			boost::optional<double> original_longitude;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLPOINT_H
