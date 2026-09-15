/* $Id$ */

/**
 * \file 
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

#include <QDebug>

#include "PythonReadlineDialog.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::PythonReadlineDialog::PythonReadlineDialog(
		QWidget *parent_) :
	// Note the window close button: closing the window is a valid response
	// (it means end-of-file).
	QDialog(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
			Qt::WindowCloseButtonHint)
{
	setupUi(this);

	setWindowModality(Qt::ApplicationModal);
	QtWidgetUtils::resize_based_on_size_hint(this);
	setFixedHeight(height());
}


QString
GPlatesQtWidgets::PythonReadlineDialog::get_line(
		const QString &prompt)
{
	static const int MAX_PROMPT_LENGTH = 50;
	prompt_label->setText(prompt.length() <= MAX_PROMPT_LENGTH ? prompt :
			"..." + prompt.right(MAX_PROMPT_LENGTH - 3));
	input_lineedit->setText(QString());
	if (!d_pos.isNull())
	{
		move(d_pos);
	}

	const bool accepted = (exec() == QDialog::Accepted);

	d_pos = pos();

	if (!accepted)
	{
		// The user cancelled (Cancel button, Escape key or window close button).
		//
		// Return an empty string - not even a newline - because that is how Python
		// signals end-of-file on stdin (the equivalent of typing Ctrl-D in a
		// terminal). Without it there is no way out of Python code that keeps
		// reading lines, such as the interactive 'help()' utility, and this dialog
		// just reappears indefinitely.
		return QString();
	}

	return input_lineedit->text() + "\n";
}

