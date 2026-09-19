/* $Id: PythonReadlineDialog.cc 10957 2011-02-09 07:53:12Z elau $ */

/**
 * \file 
 * $Revision: 10957 $
 * $Date: 2011-02-09 18:53:12 +1100 (Wed, 09 Feb 2011) $ 
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

#include "PythonInitFailedDialog.h"

#include "gui/PythonManager.h"


namespace
{
	const char *python_failed_msg =
		"<html> <body> \
		<h2> Python initialisation failed. </h2> \
		GPlates requires Python, and so will not start up. \
		<br /> \
		<h3>Troubleshooting</h3> \
		<h4>If this GPlates was installed from a binary distribution</h4> \
		<font color=\"red\">$PYTHON_NAME</font> is part of the installation, so the \
		installation is most likely incomplete or damaged. Install GPlates again, and contact the \
		GPlates developers if that does not help. \
		<h4>If this GPlates was built from source</h4> \
		Check that $PYTHON_NAME is installed and is the Python that GPlates was built against. \
		</body> </html>"
		;
}


GPlatesQtWidgets::PythonInitFailedDialog::PythonInitFailedDialog(
		QWidget *parent_) :
	QDialog(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint )
{
	setupUi(this);
	setModal(true);
	msg_text_browser->setOpenExternalLinks(true);
	assemble_message();
	msg_text_browser->setHtml(d_html_page);
}


void
GPlatesQtWidgets::PythonInitFailedDialog::assemble_message()
{
	d_html_page = QString(python_failed_msg);
	d_html_page.replace(
			"$PYTHON_NAME",
			QString("Python ") + GPlatesGui::PythonManager::instance()->python_version());
}
