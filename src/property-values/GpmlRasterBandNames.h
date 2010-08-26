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

#ifndef GPLATES_PROPERTYVALUES_GPMLRASTERBANDNAMES_H
#define GPLATES_PROPERTYVALUES_GPMLRASTERBANDNAMES_H

#include <vector>

#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlRasterBandNames, visit_gpml_raster_band_names)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:RasterBandNames".
	 */
	class GpmlRasterBandNames :
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlRasterBandNames>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlRasterBandNames> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlRasterBandNames>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlRasterBandNames> non_null_ptr_to_const_type;

		virtual
		~GpmlRasterBandNames()
		{  }

		typedef std::vector<XsString::non_null_ptr_to_const_type> band_names_list_type;

		/**
		 * Create a GpmlRasterBandNames instance from a collection of @a band_names_.
		 */
		static
		const non_null_ptr_type
		create(
				const band_names_list_type &band_names_);

		template<typename ForwardIterator>
		static
		const non_null_ptr_type
		create(
				ForwardIterator begin,
				ForwardIterator end)
		{
			return new GpmlRasterBandNames(begin, end);
		}

		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new GpmlRasterBandNames(*this));
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

		const band_names_list_type &
		band_names() const
		{
			return d_band_names;
		}

		void
		set_band_names(
				const band_names_list_type &band_names_)
		{
			d_band_names = band_names_;
			update_instance_id();
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
			visitor.visit_gpml_raster_band_names(*this);
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
			visitor.visit_gpml_raster_band_names(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlRasterBandNames(
				const band_names_list_type &band_names_) :
			PropertyValue(),
			d_band_names(band_names_)
		{  }


		template<typename ForwardIterator>
		GpmlRasterBandNames(
				ForwardIterator begin,
				ForwardIterator end) :
			PropertyValue(),
			d_band_names(begin, end)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlRasterBandNames(
				const GpmlRasterBandNames &other) :
			PropertyValue(other), /* share instance id */
			d_band_names(other.d_band_names)
		{  }

	private:

		band_names_list_type d_band_names;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlRasterBandNames &
		operator=(
				const GpmlRasterBandNames &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
