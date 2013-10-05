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

#ifndef GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H
#define GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "GpmlTimeWindow.h"
#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/RevisionedVector.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPiecewiseAggregation, visit_gpml_piecewise_aggregation)

namespace GPlatesPropertyValues
{

	class GpmlPiecewiseAggregation:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlPiecewiseAggregation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPiecewiseAggregation> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlPiecewiseAggregation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPiecewiseAggregation> non_null_ptr_to_const_type;


		virtual
		~GpmlPiecewiseAggregation()
		{  }

		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTimeWindow::non_null_ptr_type> &time_windows_,
				const StructuralType &value_type_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlPiecewiseAggregation>(clone_impl());
		}

		/**
		 * Returns the 'const' vector of time windows.
		 */
		const GPlatesModel::RevisionedVector<GpmlTimeWindow> &
		time_windows() const
		{
			return *get_current_revision<Revision>().time_windows.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of time windows.
		 */
		GPlatesModel::RevisionedVector<GpmlTimeWindow> &
		time_windows()
		{
			return *get_current_revision<Revision>().time_windows.get_revisionable();
		}

		// Note that no "setter" is provided:  The value type of a GpmlPiecewiseAggregation
		// instance should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("PiecewiseAggregation");
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
			visitor.visit_gpml_piecewise_aggregation(*this);
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
			visitor.visit_gpml_piecewise_aggregation(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlPiecewiseAggregation(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GpmlTimeWindow>::non_null_ptr_type time_windows_,
				const StructuralType &value_type_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, time_windows_))),
			d_value_type(value_type_)
		{  }

		//! Constructor used when cloning.
		GpmlPiecewiseAggregation(
				const GpmlPiecewiseAggregation &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this))),
			d_value_type(other_.d_value_type)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlPiecewiseAggregation(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GpmlPiecewiseAggregation &other_pv = dynamic_cast<const GpmlPiecewiseAggregation &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
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
				public PropertyValue::Revision
		{
			explicit
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					GPlatesModel::RevisionedVector<GpmlTimeWindow>::non_null_ptr_type time_windows_) :
				time_windows(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlTimeWindow> >::attach(
										transaction_, child_context_, time_windows_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				time_windows(other_.time_windows)
			{
				// Clone data members that were not deep copied.
				time_windows.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				time_windows(other_.time_windows)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *time_windows.get_revisionable() == *other_revision.time_windows.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GpmlTimeWindow> > time_windows;
		};


		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H
