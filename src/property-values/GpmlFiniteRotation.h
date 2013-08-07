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

#include "GmlPoint.h"
#include "GpmlMeasure.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/FiniteRotation.h"

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
		 * Create a GpmlFiniteRotation instance from a finite rotation.
		 */
		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::FiniteRotation &finite_rotation)
		{
			return non_null_ptr_type(new GpmlFiniteRotation(finite_rotation));
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
				const double &gml_angle_in_degrees);

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
				const GmlPoint::non_null_ptr_type &gpml_euler_pole,
				const GpmlMeasure::non_null_ptr_type &gml_angle_in_degrees);

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
		create_zero_rotation();


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
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("FiniteRotation");
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

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlFiniteRotation(
				const GPlatesMaths::FiniteRotation &finite_rotation_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(finite_rotation_)))
		{  }

		// This constructor is used by derived classes.
		explicit
		GpmlFiniteRotation(
				const Revision::non_null_ptr_type &revision_):
			PropertyValue(revision_)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlFiniteRotation(*this));
		}

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const GPlatesMaths::FiniteRotation &finite_rotation_) :
				finite_rotation(finite_rotation_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return finite_rotation == other_revision.finite_rotation &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GPlatesMaths::FiniteRotation finite_rotation;
		};

	private:

	};


	/**
	 * Calculate the GmlPoint of the Euler pole of the GpmlFiniteRotation instance @a fr.
	 *
	 * The instance @a fr should @em not represent a zero rotation.  Use the member function
	 * @a is_zero_rotation to determine whether a GpmlFiniteRotation instance represents a zero
	 * rotation.
	 *
	 * Note that the GmlPoint is calculated on-the-fly, rather than being stored inside the
	 * GpmlFiniteRotation instance.  Thus, modifying the target of the return-value will have
	 * no effect upon the internals of the GpmlFiniteRotation instance.
	 */
	const GmlPoint::non_null_ptr_type
	calculate_euler_pole(
			const GpmlFiniteRotation &fr);


	/**
	 * Calculate the angle of rotation (in degrees) of this finite rotation.
	 *
	 * The return-value of this function is suitable as the value of the "gml:angle"
	 * property.
	 */
	const GPlatesMaths::real_t
	calculate_angle(
			const GpmlFiniteRotation &fr);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLFINITEROTATION_H
