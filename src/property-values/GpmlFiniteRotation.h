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

#ifndef GPLATES_PROPERTYVALUES_GPMLFINITEROTATION_H
#define GPLATES_PROPERTYVALUES_GPMLFINITEROTATION_H

#include <utility>  /* std::pair */
#include <boost/optional.hpp>

#include "GpmlMeasure.h"
#include "GmlPoint.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/FiniteRotation.h"

#include "model/Metadata.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFiniteRotation, visit_gpml_finite_rotation)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:FiniteRotation".
	 */
	class GpmlFiniteRotation:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotation> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotation> non_null_ptr_to_const_type;


		virtual
		~GpmlFiniteRotation()
		{  }

		/**
		 * Create a GpmlFiniteRotation instance from a finite rotation and optional metadata.
		 */
		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::FiniteRotation &finite_rotation,
				boost::optional<const GPlatesModel::MetadataContainer &> metadata_ = boost::none)
		{
			return non_null_ptr_type(new GpmlFiniteRotation(finite_rotation, metadata_));
		}

		/**
		 * Create a GpmlFiniteRotation instance from an Euler pole (longitude, latitude)
		 * and a rotation angle (units-of-measure: degrees).
		 *
		 * This coordinate duple corresponds to the contents of the "gml:pos" property in a
		 * "gml:Point" structural-type.  The first element in the pair is expected to be a
		 * longitude value; the second is expected to be a latitude.  This is the form used
		 * in GML.
		 *
		 * It is assumed that the angle is non-zero (since, technically-speaking, a zero
		 * angle would result in an indeterminate Euler pole).
		 */
		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const std::pair<double, double> &gpml_euler_pole,
				const double &gml_angle_in_degrees,
				boost::optional<const GPlatesModel::MetadataContainer &> metadata_ = boost::none);

		/**
		 * Create a GpmlFiniteRotation instance from an Euler pole (longitude, latitude)
		 * and a rotation angle (units-of-measure: degrees).
		 *
		 * It is assumed that the angle is non-zero (since, technically-speaking, a zero
		 * angle would result in an indeterminate Euler pole).
		 */
		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GmlPoint::non_null_ptr_to_const_type &gpml_euler_pole,
				const GpmlMeasure::non_null_ptr_to_const_type &gml_angle_in_degrees,
				boost::optional<const GPlatesModel::MetadataContainer &> metadata_ = boost::none);

		/**
		 * Create a GpmlFiniteRotation instance which represents a "zero" rotation.
		 *
		 * A "zero" rotation is one in which the angle of rotation is zero (or,
		 * strictly-speaking, an integer multiple of two PI).
		 */
		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create_zero_rotation(
				boost::optional<const GPlatesModel::MetadataContainer &> metadata_ = boost::none);


		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlFiniteRotation>(clone_impl());
		}

		/**
		 * Return whether this GpmlFiniteRotation instance represents a "zero" rotation.
		 *
		 * A "zero" rotation is one in which the angle of rotation is zero (or,
		 * strictly-speaking, an integer multiple of two PI).
		 *
		 * A zero rotation has no determinate Euler pole.  Hence, attempting to invoke the
		 * function @a calculate_euler_pole on a zero rotation GpmlFiniteRotation instance
		 * would result in an exception being thrown back at you.
		 */
		bool
		is_zero_rotation() const;

		/**
		 * Access the GPlatesMaths::FiniteRotation which encodes the finite rotation of
		 * this instance.
		 */
		const GPlatesMaths::FiniteRotation &
		get_finite_rotation() const
		{
			return get_current_revision<Revision>().finite_rotation;
		}

		/**
		 * Set the finite rotation within this instance to @a fr.
		 */
		void
		set_finite_rotation(
				const GPlatesMaths::FiniteRotation &fr);
		
		/**
		 * FIXME: Re-implement MetadataContainer because it's currently possible to modify the
		 * metadata in a 'const' MetadataContainer and this by-passes revisioning.
		 */
		const GPlatesModel::MetadataContainer &
		get_metadata() const
		{
			return get_current_revision<Revision>().metadata;
		}
		
		void
		set_metadata(
				const GPlatesModel::MetadataContainer &metadata);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			return STRUCTURAL_TYPE;
		}

		/**
		 * Static access to the structural type as GpmlFiniteRotation::STRUCTURAL_TYPE.
		 */
		static const StructuralType STRUCTURAL_TYPE;


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
			visitor.visit_gpml_finite_rotation(*this);
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
			visitor.visit_gpml_finite_rotation(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					const GPlatesMaths::FiniteRotation &finite_rotation_,
					boost::optional<const GPlatesModel::MetadataContainer &> metadata_);

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_);

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const;

			GPlatesMaths::FiniteRotation finite_rotation;
			GPlatesModel::MetadataContainer metadata;
		};


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlFiniteRotation(
				const GPlatesMaths::FiniteRotation &finite_rotation_,
				boost::optional<const GPlatesModel::MetadataContainer &> metadata_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(finite_rotation_, metadata_)))
		{  }

		//! Constructor used when cloning.
		GpmlFiniteRotation(
				const GpmlFiniteRotation &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		// This constructor is used by derived classes - seems we need to define after @a Revision.
		explicit
		GpmlFiniteRotation(
				const Revision::non_null_ptr_type &revision_) :
			PropertyValue(revision_)
		{  }

		// This constructor is used by derived classes - seems we need to define after @a Revision.
		explicit
		GpmlFiniteRotation(
				const GpmlFiniteRotation &other_,
				const Revision::non_null_ptr_type &revision_) :
			PropertyValue(revision_)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlFiniteRotation(*this, context));
		}

	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLFINITEROTATION_H
