/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_PROPERTYVALUENOTSUPPORTEDEXCEPTION_H
#define GPLATES_QTWIDGETS_PROPERTYVALUENOTSUPPORTEDEXCEPTION_H

#include <string>
#include "global/IllegalParametersException.h"

namespace GPlatesQtWidgets
{
	/**
	 * Exception thrown by Edit Widgets when asked to configure themselves
	 * for a property value type which they do not support.
	 *
	 * For example, EditEnumerationWidget will throw this if you ask
	 * it to configure itself for an xs:double.
	 */
	class PropertyValueNotSupportedException:
			public GPlatesGlobal::IllegalParametersException
	{
		public:
			// FIXME: I thought we weren't using strings in exceptions?
			explicit
			PropertyValueNotSupportedException(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				IllegalParametersException(
						exception_source,
						"An edit widget was asked to edit an unsupported property value.")
			{  }
		
		protected:
			
			const char *
			exception_name() const
			{
				return "PropertyValueNotSupportedException";
			}
	};
}

#endif	// GPLATES_QTWIDGETS_PROPERTYVALUENOTSUPPORTEDEXCEPTION_H
