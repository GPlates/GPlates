/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREVISITOR_H
#define GPLATES_MODEL_FEATUREVISITOR_H


namespace GPlatesModel {

	// Forward declarations for the member functions.
	class FeatureHandle;
	class GmlLineString;
	class GmlOrientableCurve;
	class GmlPoint;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlConstantValue;
	class GpmlFiniteRotation;
	class GpmlFiniteRotationSlerp;
	class GpmlIrregularSampling;
	class GpmlPlateId;
	class GpmlTimeSample;
	class SingleValuedPropertyContainer;
	class XsString;

	/**
	 * This class defines an abstract interface for a Visitor to visit non-const features.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class FeatureVisitor
	{
	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~FeatureVisitor() = 0;

		virtual
		void
		visit_feature_handle(
				FeatureHandle &feature_handle)
		{  }

		virtual
		void
		visit_gml_line_string(
				GmlLineString &gml_line_string)
		{  }

		virtual
		void
		visit_gml_orientable_curve(
				GmlOrientableCurve &gml_orientable_curve)
		{  }

		virtual
		void
		visit_gml_point(
				GmlPoint &gml_point)
		{  }

		virtual
		void
		visit_gml_time_instant(
				GmlTimeInstant &gml_time_instant)
		{  }

		virtual
		void
		visit_gml_time_period(
				GmlTimePeriod &gml_time_period)
		{  }

		virtual
		void
		visit_gpml_constant_value(
				GpmlConstantValue &gpml_constant_value)
		{  }

		virtual
		void
		visit_gpml_finite_rotation(
				GpmlFiniteRotation &gpml_finite_rotation)
		{  }

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
		{  }

		virtual
		void
		visit_gpml_irregular_sampling(
				GpmlIrregularSampling &gpml_irregular_sampling)
		{  }

		virtual
		void
		visit_gpml_plate_id(
				GpmlPlateId &gpml_plate_id)
		{  }

		virtual
		void
		visit_gpml_time_sample(
				GpmlTimeSample &gpml_time_sample)
		{  }

		virtual
		void
		visit_single_valued_property_container(
				SingleValuedPropertyContainer &single_valued_property_container)
		{  }

		virtual
		void
		visit_xs_string(
				XsString &xs_string)
		{  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		FeatureVisitor &
		operator=(
				const FeatureVisitor &);
	};

}

#endif  // GPLATES_MODEL_FEATUREVISITOR_H
