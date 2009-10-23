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
 
#include "LicenseDialog.h"

GPlatesQtWidgets::LicenseDialog::LicenseDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint)
{
	setupUi(this);
	setWindowTitle(QObject::tr("License"));

	QString message(QObject::tr(
			"GPlates is free software; you can redistribute it and/or modify it under "
			"the terms of the GNU General Public License, version 2, as published by "
			"the Free Software Foundation.\n"
			"\n"
			"GPlates is distributed in the hope that it will be useful, but WITHOUT "
			"ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
			"FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License "
			"for more details.\n"
			"\n"
			"You should have received a copy of the GNU General Public License along "
			"with this program; if not, write to the Free Software Foundation, Inc., "
			"51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA."));

	text_information->setPlainText(message);

	setWindowModality(Qt::WindowModal);
}
