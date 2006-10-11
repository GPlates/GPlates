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

#include <sstream>

// FIXME:  This should be done in a more portable manner, using Qt library functions.
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include "UniqueId.h"


namespace
{
	const std::string
	get_time_component()
	{
		std::ostringstream oss;

		oss << ::time(NULL);
		return oss.str();
	}

	const std::string
	get_username_component()
	{
		char *logname;
		logname = ::getenv("LOGNAME");
		if (logname == NULL)
		{
			return std::string("");
		}
		else
		{
			return std::string(logname);
		}
	}

	const std::string
	get_hostname_component()
	{
		struct utsname u;
		if (::uname(&u) != 0)
		{
			return std::string("");
		}
		else
		{
			return std::string(u.nodename);
		}
	}

	const std::string
	get_pid_component()
	{
		std::ostringstream oss;

		oss << ::getpid();
		return oss.str();
	}
}


const std::string
GPlatesUtil::UniqueId::generate()
{
	if (s_instance == NULL)
	{
		// Instantiate the singleton.
		s_instance = UniqueId::create_instance();
	}

	std::ostringstream oss;

	// Note that the order is important:  The counter should be "gotten" first, to regenerate
	// the time component if necessary.  (Also, the order is important because this is the
	// order in which the components should occur in the ID.)
	oss << s_instance->get_counter_component() << "_";
	oss << s_instance->get_time_component() << "_";
	oss << s_instance->get_username_hostname_pid_component();

	return oss.str();
}


GPlatesUtil::UniqueId *GPlatesUtil::UniqueId::s_instance = NULL;


GPlatesUtil::UniqueId *
GPlatesUtil::UniqueId::create_instance()
{
	std::ostringstream oss;

	oss << ::get_username_component() << "@";
	oss << ::get_hostname_component() << ":";
	oss << ::get_pid_component();

	// FIXME:  We should hash the resulting string in 'oss' to hide any "bad" characters ("bad"
	// characters being characters which make XML sad).
	return new UniqueId(::get_time_component(), oss.str());
}


void
GPlatesUtil::UniqueId::regenerate_time()
{
	d_time_component = ::get_time_component();
}
