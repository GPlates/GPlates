/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#ifndef _GPLATES_OPENGL_OPENGLEXCEPTION_H_
#define _GPLATES_OPENGL_OPENGLEXCEPTION_H_

#include "global/GPlatesException.h"


namespace GPlatesOpenGL
{
	/**
	 * A general openGL-error exception.
	 */
	class OpenGLException
		: public GPlatesGlobal::Exception
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * in which the problem occurs.
			 */
			OpenGLException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const char *msg) :
				GPlatesGlobal::Exception(exception_source),
				_msg(msg)
			{  }

		protected:
			virtual const char *
			exception_name() const {

				return "OpenGLException";
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

#endif  // _GPLATES_OPENGL_OPENGLEXCEPTION_H_

