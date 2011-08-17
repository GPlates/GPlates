/* $Id$ */

/**
 * \file Responsible for managing instances of GPlatesQtWidgets::GPlatesDialog in the application.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "Dialogs.h"

#include <QtGlobal>

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "app-logic/ApplicationState.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GPlatesDialog.h"
#include "qt-widgets/ViewportWindow.h"

////////////////////////////////////////////////
// #includes for all dialogs managed by us.
////////////////////////////////////////////////
#include "qt-widgets/LogDialog.h"


GPlatesGui::Dialogs::Dialogs(
		GPlatesAppLogic::ApplicationState &_application_state,
		GPlatesPresentation::ViewState &_view_state,
		GPlatesQtWidgets::ViewportWindow &_viewport_window,
		QObject *_parent) :
	QObject(_parent),
	d_application_state_ptr(&_application_state),
	d_view_state_ptr(&_view_state),
	d_viewport_window_ptr(&_viewport_window)
{
}


////////////////////////////////////////////////////////////////////////
// Here are all the accessors for dialogs managed by this class.
////////////////////////////////////////////////////////////////////////

GPlatesQtWidgets::LogDialog &
GPlatesGui::Dialogs::log_dialog()
{
	// LogDialog can be lazy-loaded when it is first referenced.
	// Its backend, the app-logic tier log model, is not.
	static QPointer<GPlatesQtWidgets::LogDialog> dialog_ptr = new GPlatesQtWidgets::LogDialog(
			application_state(), &viewport_window());

	// The dialog not existing is a serious error.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			! dialog_ptr.isNull(), GPLATES_ASSERTION_SOURCE);
	
	return *dialog_ptr;
}

void
GPlatesGui::Dialogs::lazy_pop_up_log_dialog()
{
	// The reason this method exists (i.e., log_dialog().pop_up() is not being used directly) is to allow
	// LogDialog to be lazy-loaded not just at app startup, but *even later* when the user first triggers it.
	// Not all dialogs will be able to cope with this as they may have quite tight integration with the
	// rest of the app.
	log_dialog().pop_up();
}




////////////////////////////////////////////////////////////////////////

void
GPlatesGui::Dialogs::close_all_dialogs()
{
	Q_FOREACH(QObject *obj, children())
	{
		QDialog *dialog = dynamic_cast<QDialog *>(obj);
		if (dialog)
		{
			dialog->reject();
		}
	}
}


GPlatesAppLogic::ApplicationState &
GPlatesGui::Dialogs::application_state() const
{
	return *d_application_state_ptr;
}

GPlatesPresentation::ViewState &
GPlatesGui::Dialogs::view_state() const
{
	return *d_view_state_ptr;
}

GPlatesQtWidgets::ViewportWindow &
GPlatesGui::Dialogs::viewport_window() const
{
	return *d_viewport_window_ptr;
}




