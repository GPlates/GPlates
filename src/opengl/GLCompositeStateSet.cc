/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "GLCompositeStateSet.h"


void
GPlatesOpenGL::GLCompositeStateSet::enter_state_set() const
{
	state_set_seq_type::const_iterator state_set_iter = d_state_sets.begin();
	state_set_seq_type::const_iterator state_set_end = d_state_sets.end();
	for ( ; state_set_iter != state_set_end; ++state_set_iter)
	{
		(*state_set_iter)->enter_state_set();
	}
}


void
GPlatesOpenGL::GLCompositeStateSet::leave_state_set() const
{
	state_set_seq_type::const_reverse_iterator state_set_iter = d_state_sets.rbegin();
	state_set_seq_type::const_reverse_iterator state_set_end = d_state_sets.rend();
	for ( ; state_set_iter != state_set_end; ++state_set_iter)
	{
		(*state_set_iter)->leave_state_set();
	}
}
