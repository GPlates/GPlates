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
#include <boost/operators.hpp>

#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

#include "utils/CopyOnWrite.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
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


		/**
		 * Raster band name.
		 */
		class BandName :
				public boost::equality_comparable<BandName>
		{
		public:

			/**
			 * BandName has value semantics where each @a BandName instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property value and then copy-on-write is used to allow
			 * multiple @a BandName objects to share the same state (until the state is modified).
			 */
			BandName(
					XsString::non_null_ptr_type name) :
				d_name(name)
			{  }

			/**
			 * Returns the 'const' band name.
			 */
			const XsString::non_null_ptr_to_const_type
			get_name() const
			{
				return d_name.get();
			}

			/**
			 * Returns the 'non-const' band name.
			 */
			const XsString::non_null_ptr_type
			get_name()
			{
				return d_name.get();
			}

			void
			set_name(
					XsString::non_null_ptr_type name)
			{
				d_name = name;
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const BandName &other) const
			{
				return *d_name.get_const() == *other.d_name.get_const();
			}

		private:
			GPlatesUtils::CopyOnWrite<XsString::non_null_ptr_type> d_name;
		};

		//! Typedef for a sequence of band names.
		typedef std::vector<BandName> band_names_list_type;


		virtual
		~GpmlRasterBandNames()
		{  }

		/**
		 * Create a GpmlRasterBandNames instance from a collection of @a band_names_.
		 */
		static
		const non_null_ptr_type
		create(
				const band_names_list_type &band_names_)
		{
			return create(band_names_.begin(), band_names_.end());
		}

		template<typename ForwardIterator>
		static
		const non_null_ptr_type
		create(
				ForwardIterator begin,
				ForwardIterator end)
		{
			return non_null_ptr_type(new GpmlRasterBandNames(begin, end));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlRasterBandNames>(clone_impl());
		}

		/**
		 * Returns the band names.
		 *
		 * To modify any band names:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_band_names to set them.
		 *
		 * The returned band names implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const band_names_list_type &
		get_band_names() const
		{
			return get_current_revision<Revision>().band_names;
		}

		/**
		 * Sets the internal band names.
		 */
		void
		set_band_names(
				const band_names_list_type &band_names_);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("RasterBandNames");
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
		template<typename ForwardIterator>
		GpmlRasterBandNames(
				ForwardIterator begin,
				ForwardIterator end) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(begin, end)))
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlRasterBandNames(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			template<typename ForwardIterator>
			Revision(
					ForwardIterator begin_,
					ForwardIterator end_) :
				band_names(begin_, end_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			// Don't need 'clone_for_bubble_up_modification()' since we're using CopyOnWrite.

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const;

			band_names_list_type band_names;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
