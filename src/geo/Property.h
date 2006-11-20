/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_GEO_PROPERTY_H
#define GPLATES_GEO_PROPERTY_H

#include <string>

namespace GPlatesGeo {

	/**
	 * Property represents a (name, value) pair.
	 *
	 * The type of value is a parameter of the child class 
	 * ConcreteProperty.
	 */
	class Property {
		
		public:

			Property(
			 const std::string &name,
			 const std::string &units = "")
			 : m_name(name), m_units(units) {  }

			virtual
			~Property() {  }

			/**
			 * Return the descriptive name of this Property as a 
			 * std::string.
			 */
			std::string
			get_name() const {

				return m_name;
			}

			/**
			 * Return the description of the units of this Property 
			 * as a std::string.
			 */
			std::string
			get_units() const {

				return m_units;
			}
			
			virtual
			void
			set_value_from_string(
			 const std::string &new_value) = 0;

			virtual
			std::string
			get_value_as_string() const = 0;

			virtual
			bool
			can_be_parsed(
			 const std::string &new_value) const = 0;

		private:

			/**
			 * The name of this Property.
			 */
			const std::string m_name;

			/**
			 * The units of measurement of this Property.
			 *
			 * @todo This should probably be expanded into a 
			 *  comprehensive system, with conversions to and from
			 *  different units, rather than just storing a string
			 *  representation.
			 */
			const std::string m_units;
	};
}

#endif
