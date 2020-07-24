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

#include <QtGlobal>

#include "PythonInitFailedDialog.h"

#include "gui/PythonManager.h"


namespace
{
	const char* python_failed_msg =
		"<html> <body> \
		<h2> Python initialization failed. </h2> \
		GPlates will not start up. \
		<br /> \
		<h3>Troubleshooting</h3> \
		If this version of GPlates was installed via a binary distribution then please contact the GPlates developers. \
		<br /> \
		Otherwise check that <font color=\"red\">$PYTHON_NAME</font> has been installed. \
		If it has been installed at an unusual location, set the \"python/python_home\" variable in the \
		GPlates Python preferences \"Edit->Preference->Python\" (using a working version of GPlates) and then try again. \
		<h3>Install Python</h3> \
		$INSTALL_INSTRUCTION \
		</body> </html>"
		;
#ifdef Q_OS_WIN
	const char* python_install_instructions_win =
		"<p><a href=\"http://www.python.org/download\">Click here to download a Python installer for Windows</a></p>" \
		;
#endif
#ifdef Q_OS_MACOS
	const char* python_install_instructions_mac =
		"<h4>Type in \"sudo port install python<version>\" in the terminal to install python (replacing \"<version>\" with the Python version above).</h4>" \
		;
		;
#endif
#ifdef Q_OS_LINUX
	const char* python_install_instructions_linux =
		"<h4>Type in \"sudo apt-get install python<version>\" in terminal to install python (replacing \"<version>\" with the Python version above).</h4>" \
		;
#endif
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
	QString python_version = GPlatesGui::PythonManager::instance()->python_version();
	d_html_page.replace("$PYTHON_NAME", QString("Python") + python_version);
#ifdef Q_OS_MACOS   
	d_html_page.replace("$INSTALL_INSTRUCTION", python_install_instructions_mac);
#elif defined Q_OS_LINUX 
	d_html_page.replace("$INSTALL_INSTRUCTION", python_install_instructions_linux);
#elif defined Q_OS_WIN
	d_html_page.replace("$INSTALL_INSTRUCTION", python_install_instructions_win);
#endif
}




