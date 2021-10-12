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

#include "GLCompositeDrawable.h"


void
GPlatesOpenGL::GLCompositeDrawable::draw() const
{
	drawable_seq_type::const_iterator drawable_iter = d_drawables.begin();
	drawable_seq_type::const_iterator drawable_end = d_drawables.end();
	for ( ; drawable_iter != drawable_end; ++drawable_iter)
	{
		const GLDrawable::non_null_ptr_to_const_type &drawable = *drawable_iter;

		drawable->bind();
		drawable->draw();
	}
}
