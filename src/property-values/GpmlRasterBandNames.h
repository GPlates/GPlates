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

		//! Typedef for a sequence of band names.
		typedef std::vector<XsString::non_null_ptr_to_const_type> band_names_list_type;


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
			return new GpmlRasterBandNames(begin, end);
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlRasterBandNames>(clone_impl());
		}

		const band_names_list_type &
		get_band_names() const
		{
			return get_current_revision<Revision>().band_names;
		}

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


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlRasterBandNames(
				const GpmlRasterBandNames &other) :
			PropertyValue(other)
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
					ForwardIterator end_,
					bool deep_copy = true)
			{
				if (deep_copy)
				{
					set_cloned_band_names(begin_, end_);
				}
				else
				{
					band_names.insert(band_names.end(), begin_, end_);
				}
			}

			Revision(
					const Revision &other);

			// To keep our revision state immutable we clone the band names so that the client
			// can no longer modify them indirectly...
			template<typename ForwardIterator>
			void
			set_cloned_band_names(
					ForwardIterator begin_,
					ForwardIterator end_)
			{
				band_names.clear();
				ForwardIterator band_names_iter_ = begin_;
				for ( ; band_names_iter_ != end_; ++band_names_iter_)
				{
					const XsString::non_null_ptr_to_const_type &band_name_ = *band_names_iter_;
					band_names.push_back(band_name_->clone());
				}
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone_for_bubble_up_modification() const
			{
				// Don't clone the band name property values.
				return non_null_ptr_type(new Revision(band_names.begin(), band_names.end(), false/*deep_copy*/));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const;

			band_names_list_type band_names;
		};


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
