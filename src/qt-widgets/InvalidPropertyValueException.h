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

#ifndef GPLATES_QTWIDGETS_INVALIDPROPERTYVALUEEXCEPTION_H
#define GPLATES_QTWIDGETS_INVALIDPROPERTYVALUEEXCEPTION_H

#include <QString>
#include "global/PreconditionViolationError.h"

namespace GPlatesQtWidgets
{
	/**
	 * Exception thrown by an Edit Widget when create_property_value_from_widget()
	 * is called when the fields of the widget do not contain data that can be
	 * used to construct a valid PropertyValue. For example, the EditGeometryWidget
	 * when there are not enough distinct points to create a PolylineOnSphere.
	 */
	class InvalidPropertyValueException:
			public GPlatesGlobal::PreconditionViolationError
	{
		public:
			/**
			 * @a reason_ is a translated, human-readable description of the specific
			 * details of the failure. This will be presented to the user via a
			 * message box spawned from the AddPropertyDialog.
			 */
			InvalidPropertyValueException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const QString &reason_) :
				GPlatesGlobal::PreconditionViolationError(exception_source),
				d_reason(reason_)
			{  }

			~InvalidPropertyValueException() throw() { }
			
			const QString &
			reason() const
			{
				return d_reason;
			}
			
			const char *
			exception_name() const
			{
				return "InvalidPropertyValueException";
			}
		
		private:
			
			QString d_reason;
	};
}

#endif	// GPLATES_QTWIDGETS_INVALIDPROPERTYVALUEEXCEPTION_H
