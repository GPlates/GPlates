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
// FIXME:  This should be done in a platform-independent manner, using Qt functions.

namespace GPlatesUtil
{
	class UniqueId
	{
	public:
		typedef long counter_type;

		static
		const std::string
		generate();

	private:
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
