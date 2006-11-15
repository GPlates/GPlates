/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_GPMLINTERPOLATIONFUNCTION_H
#define GPLATES_MODEL_GPMLINTERPOLATIONFUNCTION_H

#include "PropertyValue.h"
#include "TemplateTypeParameterType.h"
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

	class GpmlInterpolationFunction :
			public PropertyValue {

	public:

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

		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		TemplateTypeParameterType &
		value_type() {
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

#endif  // GPLATES_MODEL_GPMLINTERPOLATIONFUNCTION_H
