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

#ifndef GPLATES_UTIL_UNIQUEID_H
#define GPLATES_UTIL_UNIQUEID_H

#include <string>
#include <unicode/unistr.h>

namespace GPlatesUtil
{
	/**
	 * This class provides a static member function to generate a reasonably-unique string
	 * identifier.
	 *
	 * The class itself is a singleton, and client code should not attempt to instantiate it;
	 * just use the static member function @a generate.
	 */
	class UniqueId
	{
	public:
		/**
		 * Generate a unique string identifier.
		 *
		 * To enable the result of this function to serve as XML IDs (which might one day
		 * be useful) without becoming too complicated for us programmers, the resultant
		 * string will conform to the regexp "[A-Za-z_][-A-Za-z_0-9.]*", which is a subset
		 * of the NCName production which defines the set of string values which are valid
		 * for the XML ID type:
		 *  - http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#ID
		 *  - http://www.w3.org/TR/1999/REC-xml-names-19990114/#NT-NCName
		 *
		 * @return A unique string identifier.
		 *
		 * @pre True.
		 *
		 * @post Return-value is a unique string identifier which conforms to the regexp
		 * "[A-Za-z_][-A-Za-z_0-9.]*", or an exception has been thrown.
		 */
		static
		const UnicodeString
		generate();

	private:
		typedef long counter_type;

		static UniqueId *s_instance;

		static
		UniqueId *
		create_instance();

		UniqueId(
				const std::string &time_component,
				const std::string &username_hostname_pid_component) :
			d_counter_component(0),
			d_time_component(time_component),
			d_username_hostname_pid_component(username_hostname_pid_component) {  }


		counter_type
		get_counter_component()
		{
			if (d_counter_component < 0)
			{
				regenerate_time();
				d_counter_component = 0;
			}
			return d_counter_component++;
		}

		const std::string &
		get_time_component()
		{
			return d_time_component;
		}

		const std::string &
		get_username_hostname_pid_component()
		{
			return d_username_hostname_pid_component;
		}

		void
		regenerate_time();

		counter_type d_counter_component;
		std::string d_time_component;
		const std::string d_username_hostname_pid_component;
	};
}

#endif  // GPLATES_UTIL_UNIQUEID_H
