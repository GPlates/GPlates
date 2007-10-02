/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H
#define GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H

#include "model/PropertyValue.h"
#include "TemplateTypeParameterType.h"


namespace GPlatesPropertyValues {

	class GpmlInterpolationFunction :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for boost::intrusive_ptr<GpmlInterpolationFunction>.
		 */
		typedef boost::intrusive_ptr<GpmlInterpolationFunction>
				maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const GpmlInterpolationFunction>.
		 */
		typedef boost::intrusive_ptr<const GpmlInterpolationFunction>
				maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<GpmlInterpolationFunction>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<GpmlInterpolationFunction>
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const GpmlInterpolationFunction>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const GpmlInterpolationFunction>
				non_null_ptr_to_const_type;

		/**
		 * Construct a GpmlInterpolationFunction instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		GpmlInterpolationFunction(
				const TemplateTypeParameterType &value_type_):
			PropertyValue(),
			d_value_type(value_type_)
		{  }

		/**
		 * Construct a GpmlInterpolationFunction instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		GpmlInterpolationFunction(
				const GpmlInterpolationFunction &other) :
			PropertyValue(),
			d_value_type(other.d_value_type)
		{  }

		virtual
		~GpmlInterpolationFunction() {  }

		// Note that no "setter" is provided:  The value type of a
		// GpmlInterpolationFunction instance should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

	private:

		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlInterpolationFunction &
		operator=(const GpmlInterpolationFunction &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H
