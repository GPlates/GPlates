/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#ifndef _GPLATES_OPENGL_VULKANEXCEPTION_H_
#define _GPLATES_OPENGL_VULKANEXCEPTION_H_

#include <string>

#include "global/GPlatesException.h"


namespace GPlatesOpenGL
{
	/**
	 * An exception related to the Vulkan graphics and compute API.
	 */
	class VulkanException :
			public GPlatesGlobal::Exception
	{
		public:

			/**
			 * @param msg is a description of the conditions in which the problem occurs.
			 */
			VulkanException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &msg) :
				GPlatesGlobal::Exception(exception_source),
				d_msg(msg)
			{  }

			~VulkanException() throw()
			{ }

		protected:

			virtual const char *
			exception_name() const
			{
				return "VulkanException";
			}

			virtual
			void
			write_message(
					std::ostream &os) const
			{
				write_string_message(os, d_msg);
			}

		private:
			std::string d_msg;
	};
}

#endif  // _GPLATES_OPENGL_VULKANEXCEPTION_H_

