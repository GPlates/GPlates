/* $Id$ */

/**
 * \file
 * Contains the definitions of the member functions of the class CallStack.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2009 The University of Sydney, Australia
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

#include <iostream>
#include "CallStackTracker.h"


void
GPlatesUtils::CallStack::push(
		const Trace &trace)
{
	d_call_stack.push_back(trace);
}


void
GPlatesUtils::CallStack::pop()
{
	d_call_stack.pop_back();
}


void
GPlatesUtils::CallStack::write_call_stack_trace(
		std::ostream &output)
{
	output << "Call stack trace:" << std::endl;

	GPlatesUtils::CallStack::trace_const_iterator trace_iter;
	for (trace_iter = call_stack_begin();
		trace_iter != call_stack_end();
		++trace_iter)
	{
		const GPlatesUtils::CallStack::Trace &trace = *trace_iter;

		output
			<< '('
			<< trace.get_filename()
			<< ", "
			<< trace.get_line_num()
			<< ')'
			<< std::endl;
	}
}
