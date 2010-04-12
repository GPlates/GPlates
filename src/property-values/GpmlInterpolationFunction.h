/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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

#include "TemplateTypeParameterType.h"

#include "model/PropertyValue.h"
#include "utils/UnicodeStringUtils.h"


// This macro is used to define the virtual function 'deep_clone_as_interp_func' inside a class
// which derives from InterpolationFunction.  The function definition is exactly identical in every
// InterpolationFunction derivation, but the function must be defined in each derived class (rather
// than in the base) because it invokes the non-virtual member function 'deep_clone' of that
// specific derived class.
// (This function 'deep_clone' cannot be moved into the base class, because (i) its return type is
// the type of the derived class, and (ii) it must perform different actions in different classes.)
// To define the function, invoke the macro in the class definition.  The macro invocation will
// expand to a definition of the function.
#define DEFINE_FUNCTION_DEEP_CLONE_AS_INTERP_FUNC()  \
		virtual  \
		const GpmlInterpolationFunction::non_null_ptr_type  \
		deep_clone_as_interp_func() const  \
		{  \
			return deep_clone();  \
		}


namespace GPlatesPropertyValues
{

	/**
	 * This is an abstract class, because it derives from class PropertyValue, which contains
	 * the pure virtual member functions @a clone and @a accept_visitor, which this class does
	 * not override with non-pure-virtual definitions.
	 */
	class GpmlInterpolationFunction:
			public GPlatesModel::PropertyValue
	{

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
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlInterpolationFunction>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlInterpolationFunction> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlInterpolationFunction>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlInterpolationFunction> non_null_ptr_to_const_type;

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
			PropertyValue(other), /* share instance id */
			d_value_type(other.d_value_type)
		{  }

		virtual
		~GpmlInterpolationFunction()
		{  }

		virtual
		const GpmlInterpolationFunction::non_null_ptr_type
		deep_clone_as_interp_func() const = 0;

		// Note that no "setter" is provided:  The value type of a
		// GpmlInterpolationFunction instance should never be changed.
		const TemplateTypeParameterType &
		value_type() const
		{
			return d_value_type;
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

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
