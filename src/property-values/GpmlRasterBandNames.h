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

#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"


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
			public GPlatesModel::PropertyValue,
			public GPlatesModel::PropertyValueRevisionContext
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
				return d_name;
			}

			/**
			 * Returns the 'non-const' band name.
			 */
			const XsString::non_null_ptr_type
			get_name()
			{
				return d_name;
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
				return *d_name == *other.d_name;
			}

		private:
			XsString::non_null_ptr_type d_name;
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
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(new GpmlRasterBandNames(transaction, begin, end));
			transaction.commit();
			return ptr;
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
				GPlatesModel::ModelTransaction &transaction_,
				ForwardIterator begin,
				ForwardIterator end) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, begin, end)))
		{  }

		//! Constructor used when cloning.
		GpmlRasterBandNames(
				const GpmlRasterBandNames &other_,
				boost::optional<PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlRasterBandNames(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		GPlatesModel::PropertyValueRevision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const PropertyValue::non_null_ptr_to_const_type &child_property_value);

		/**
		 * Inherited from @a PropertyValueRevisionContext.
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
				public GPlatesModel::PropertyValueRevision
		{
			template<typename ForwardIterator>
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					PropertyValueRevisionContext &child_context_,
					ForwardIterator begin_,
					ForwardIterator end_) :
				band_names(begin_, end_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_,
					PropertyValueRevisionContext &child_context_) :
				PropertyValueRevision(context_),
				band_names(other_.band_names)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_),
				band_names(other_.band_names)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<PropertyValueRevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const;

			band_names_list_type band_names;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
