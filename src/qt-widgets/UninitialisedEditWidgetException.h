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

#ifndef GPLATES_QTWIDGETS_UNINITIALISEDEDITWIDGETEXCEPTION_H
#define GPLATES_QTWIDGETS_UNINITIALISEDEDITWIDGETEXCEPTION_H

#include "global/PreconditionViolationError.h"

namespace GPlatesQtWidgets
{
	/**
	 * Exception thrown by an Edit Widget when update_property_value_from_widget()
	 * is called without a property value to update being previously set via
	 * update_widget_from_xxxx().
	 */
	class UninitialisedEditWidgetException:
			public GPlatesGlobal::PreconditionViolationError
	{
		public:
			explicit
			UninitialisedEditWidgetException(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				GPlatesGlobal::PreconditionViolationError(exception_source)
			{  }
			
			const char *
			exception_name() const
			{
				return "UninitialisedEditWidgetException";
			}
	};
}

#endif	// GPLATES_QTWIDGETS_UNINITIALISEDEDITWIDGETEXCEPTION_H
