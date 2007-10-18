/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#include "AboutDialog.h"
#include "ViewportWindow.h"
#include "global/Constants.h"  // the copyright string macro


GPlatesQtWidgets::AboutDialog::AboutDialog(
		ViewportWindow &viewport,
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_ptr(&viewport)
{
	setupUi(this);

	QObject::connect(button_License, SIGNAL(clicked()),
			d_viewport_ptr, SLOT(pop_up_license_dialog()));

	QString message(QObject::tr(GPlatesGlobal::HtmlCopyrightString));
	text_Copyright->setHtml(message);
}
