/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_GEO_CONCRETEPROPERTY_H
#define GPLATES_GEO_CONCRETEPROPERTY_H

#include "Property.h"
#include <sstream>

namespace GPlatesGeo {

	/**
	 * ConcreteProperty represents the parameterisation of Property
	 * values based on the type of their value.
	 *
	 * The @a value_type must be:
	 *  - copy constructable (see ConcreteProperty)
	 *  - default constructable (see can_be_parsed)
	 *  - extractable to std::string (see get_value_as_string)
	 *  - insertable from std::string ( see set_value_from_string)
	 */
	template< typename value_type >
	class ConcreteProperty : public Property {
		
		public:

			ConcreteProperty(
			 const std::string &name,
			 const value_type &value)
			 : Property(name), m_value(value) {  }

			/**
			 * Modify the value of this ConcreteProperty according
			 * to the @a new_value string.
			 *
			 * The caller should make certain that can_be_parsed 
			 * returns @b true for @a new_value prior to calling
			 * this method.
			 */
			virtual
			void
			set_value_from_string(
			 const std::string &new_value) {

				std::istringstream iss(new_value);
				iss >> m_value;
			}
			
			/**
			 * Obtain a string representation of @a m_value.  This
			 * could then be modified and given back via 
			 * set_value_from_string.
			 */
			virtual
			std::string
			get_value_as_string() const {

				std::ostringstream oss;
				oss << m_value;
				return oss.str();
			}

			/**
			 * can_be_parsed will return true if @a new_value is a
			 * valid string representation of the @a value_type.
			 *
			 * This function should be called, and it's return value
			 * checked, prior to calling set_value_from_string.
			 */
			virtual
			bool
			can_be_parsed(
			 const std::string &new_value) const {

				std::istringstream iss(new_value);
				value_type test_val;
				iss >> test_val;
				return iss;
			}

		private:

			/**
			 * The actual value that this ConcreteProperty represents.
			 */
			value_type m_value;
	};
}

#endif
