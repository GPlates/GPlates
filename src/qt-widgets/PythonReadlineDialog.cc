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
	QDialog(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint)
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

	QString line;
	if (exec() == QDialog::Accepted)
	{
		line = input_lineedit->text();
	}

	d_pos = pos();

	return line + "\n";
}

