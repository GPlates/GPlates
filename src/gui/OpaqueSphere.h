/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_OPAQUESPHERE_H_
#define _GPLATES_GUI_OPAQUESPHERE_H_

#include "OpenGL.h"
#include "Colour.h"
#include "Quadrics.h"

namespace GPlatesGui
{
	class OpaqueSphere
	{
		public:
			explicit
			OpaqueSphere(const Colour &colour);

			~OpaqueSphere() {  }

		private:
			/*
			 * These two member functions intentionally declared
			 * private to avoid object copying/assignment.
			 * [Since the data member '_quad' itself cannot be
			 * copied or assigned.]
			 */
			OpaqueSphere(const OpaqueSphere &other);

			OpaqueSphere &operator=(const OpaqueSphere &other);

		public:
			void Paint();

		private:
			Quadrics _quad;

			Colour _colour;
	};
}

#endif /* _GPLATES_GUI_OPAQUESPHERE_H_ */
