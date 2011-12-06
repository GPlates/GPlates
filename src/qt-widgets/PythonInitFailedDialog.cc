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

#ifndef GPLATES_NO_PYTHON

namespace
{
	const char* python_failed_msg =
		"<html> <body> \
		<h2> Python initialization failed. <br /> \
		GPlates will start up without python support.</h2> \
		<br /> \
		<h3>Troubleshooting</h3> \
		<h4>Check if <font color=\"red\">$PYTHON_NAME</font> has been installed.</h4> \
		<h4>If $PYTHON_NAME has been installed at an unusual location, \
		set \"python/python_home\" variable in GPlates preference \"Edit->Preference->Advanced Settings\" \
		and restart GPlates. </h4> \
		<h3>Install Python</h3> \
		$INSTALL_INSTRUCTION \
		</body> </html>"
		;
#ifdef __WINDOWS__
	const char* python26_install_instructions_win =
		"<p><a href=\"http://www.python.org/download/releases/2.6.6/\">Click here to download Python installer for Windows</a></p>" \
		;

	const char* python27_install_instructions_win =
		"<p><a href=\"http://www.python.org/download/releases/2.7.2/\">Click here to download Python installer for Windows</a></p>" \
		;
#endif

#ifdef __APPLE__
	const char* python26_install_instructions_mac =
		"<h4>Type in \"sudo port install python26\" in terminal to install python.</h4>" \
		;
	const char* python27_install_instructions_mac =
		"<h4>Type in \"sudo port install python27\" in terminal to install python.</h4>" \
		;
#endif

#ifdef __LINUX__
	const char* python26_install_instructions_linux =
		"<h4>Type in \"sudo apt-get install python2.6\" in terminal to install python.</h4>" \
		;

	const char* python27_install_instructions_linux =
		"<h4>Type in \"sudo apt-get install python2.7\" in terminal to install python.</h4>" \
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
#ifdef __APPLE__
	if("2.7" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python27_install_instructions_mac);
	else if("2.6" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python26_install_instructions_mac);
#elif defined __LINUX__
	if("2.7" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python27_install_instructions_linux);
	else if("2.6" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python26_install_instructions_linux);
#elif defined __WINDOWS__
	if("2.7" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python27_install_instructions_win);
	else if("2.6" == python_version)
		d_html_page.replace("$INSTALL_INSTRUCTION", python26_install_instructions_win);
#endif
}

#endif


