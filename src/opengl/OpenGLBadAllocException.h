/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef _GPLATES_OPENGL_OPENGLBADALLOCEXCEPTION_H_
#define _GPLATES_OPENGL_OPENGLBADALLOCEXCEPTION_H_

#include "global/GPlatesException.h"


namespace GPlatesOpenGL
{
	/**
	 * The Exception thrown by the OpenGL wrappers when OpenGL is unable
	 * to allocate memory for an object.
	 */
	class OpenGLBadAllocException
		: public GPlatesGlobal::Exception
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * in which the problem occurs.
			 */
			OpenGLBadAllocException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const char *msg) :
				GPlatesGlobal::Exception(exception_source),
				_msg(msg)
			{  }

			virtual
			~OpenGLBadAllocException() throw() {  }

		protected:
			virtual const char *
			exception_name() const {

				return "OpenGLBadAllocException";
			}

			virtual
			void
			write_message(
					std::ostream &os) const
			{
				write_string_message(os, _msg);
			}

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_OPENGL_OPENGLBADALLOCEXCEPTION_H_
