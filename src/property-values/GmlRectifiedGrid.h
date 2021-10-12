/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H
#define GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H

#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "Georeferencing.h"
#include "GmlGridEnvelope.h"
#include "GmlPoint.h"
#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlRectifiedGrid, visit_gml_rectified_grid)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:RectifiedGrid".
	 */
	class GmlRectifiedGrid:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlRectifiedGrid>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlRectifiedGrid> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlRectifiedGrid>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlRectifiedGrid> non_null_ptr_to_const_type;

		typedef std::vector<XsString::non_null_ptr_to_const_type> axes_list_type;
		typedef std::vector<double> offset_vector_type;
		typedef std::vector<offset_vector_type> offset_vector_list_type;
		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes_type;

		virtual
		~GmlRectifiedGrid()
		{  }

		/**
		 * Create a GmlRectifiedGrid instance.
		 *
		 * The @a xml_attributes_ could contain srsName, the identifier of a
		 * coordinate reference system, and dimension, which this class assumes to be
		 * 2, but is ignored if present.
		 *
		 * We don't check if the number of dimensions in the axes list or in the
		 * offset vectors list or in the origin specification match up with each other
		 * or with the dimension XML attribute.
		 */
		static
		const non_null_ptr_type
		create(
				const GmlGridEnvelope::non_null_ptr_to_const_type &limits_,
				const axes_list_type &axes_,
				const GmlPoint::non_null_ptr_to_const_type &origin_,
				const offset_vector_list_type &offset_vectors_,
				const xml_attributes_type &xml_attributes_);

		/**
		 * Convenience function for creating a GmlRectifiedGrid from georeferencing
		 * information, and raster width and height.
		 */
		static
		const non_null_ptr_type
		create(
				const Georeferencing::non_null_ptr_to_const_type &georeferencing,
				unsigned int raster_width,
				unsigned int raster_height,
				const xml_attributes_type &xml_attributes_);

		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new GmlRectifiedGrid(*this));
			return dup;
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const GmlGridEnvelope::non_null_ptr_to_const_type &
		limits() const
		{
			return d_limits;
		}

		void
		set_limits(
				const GmlGridEnvelope::non_null_ptr_to_const_type &limits_)
		{
			d_limits = limits_;
			update_instance_id();
		}

		const axes_list_type &
		axes() const
		{
			return d_axes;
		}

		void
		set_axes(
				const axes_list_type &axes_)
		{
			d_axes = axes_;
			update_instance_id();
		}

		const GmlPoint::non_null_ptr_to_const_type &
		origin() const
		{
			return d_origin;
		}

		void
		set_origin(
				const GmlPoint::non_null_ptr_to_const_type &origin_)
		{
			d_origin = origin_;

			// Invalidate the georeferencing cache because that's calculated using the origin.
			d_cached_georeferencing = boost::none;

			update_instance_id();
		}

		const offset_vector_list_type &
		offset_vectors() const
		{
			return d_offset_vectors;
		}

		void
		set_offset_vectors(
				const offset_vector_list_type &offset_vectors_)
		{
			d_offset_vectors = offset_vectors_;

			// Invalidate the georeferencing cache because that's calculated using the
			// offset vectors.
			d_cached_georeferencing = boost::none;

			update_instance_id();
		}

		const xml_attributes_type &
		xml_attributes() const
		{
			return d_xml_attributes;
		}

		void
		set_xml_attributes(
				const xml_attributes_type &xml_attributes_)
		{
			d_xml_attributes = xml_attributes_;
			update_instance_id();
		}

		const boost::optional<Georeferencing::non_null_ptr_to_const_type> &
		convert_to_georeferencing() const;

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
			visitor.visit_gml_rectified_grid(*this);
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
			visitor.visit_gml_rectified_grid(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlRectifiedGrid(
				const GmlGridEnvelope::non_null_ptr_to_const_type &limits_,
				const axes_list_type &axes_,
				const GmlPoint::non_null_ptr_to_const_type &origin_,
				const offset_vector_list_type &offset_vectors_,
				const xml_attributes_type xml_attributes_) :
			PropertyValue(),
			d_limits(limits_),
			d_axes(axes_),
			d_origin(origin_),
			d_offset_vectors(offset_vectors_),
			d_xml_attributes(xml_attributes_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlRectifiedGrid(
				const GmlRectifiedGrid &other) :
			PropertyValue(other), /* share instance id */
			d_limits(other.d_limits),
			d_axes(other.d_axes),
			d_origin(other.d_origin),
			d_offset_vectors(other.d_offset_vectors),
			d_xml_attributes(other.d_xml_attributes),
			d_cached_georeferencing(other.d_cached_georeferencing)
		{  }

	private:

		GmlGridEnvelope::non_null_ptr_to_const_type d_limits;
		axes_list_type d_axes;
		GmlPoint::non_null_ptr_to_const_type d_origin;
		offset_vector_list_type d_offset_vectors;

		xml_attributes_type d_xml_attributes;

		mutable boost::optional<Georeferencing::non_null_ptr_to_const_type> d_cached_georeferencing;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlRectifiedGrid &
		operator=(
				const GmlRectifiedGrid &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLRECTIFIEDGRID_H
