/* $Id: HellingerStatsDialog.cc 240 2012-02-27 16:30:31Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 240 $
 * $Date: 2012-02-27 17:30:31 +0100 (Mon, 27 Feb 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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
#include <QFileDialog>
#include <QTextStream>

#include "HellingerStatsDialog.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerStatsDialog::HellingerStatsDialog(
		const QString &results_file,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_results_file(results_file)
{
	setupUi(this);
	QObject::connect(button_export, SIGNAL(clicked()), this, SLOT(handle_export()));
}

void
GPlatesQtWidgets::HellingerStatsDialog::update()
{
	textEdit->clear();

	QFile dataFile(d_results_file);

	QString line;

	if (dataFile.open(QFile::ReadOnly))
	{
		QTextStream in(&dataFile);
		in.setCodec("UTF-8");
		do
		{
			line = in.readLine();
			textEdit->append(line);
		}
		while (!line.isNull());
	}
}

void
GPlatesQtWidgets::HellingerStatsDialog::handle_export()
{
	QString output_file_name = QFileDialog::getSaveFileName(this, tr("Export File"), "",
													 tr("Text Files (*.txt)"));
	QFile file_out(output_file_name);

	QFile file_in(d_results_file);
	QString line;
	if (file_in.open(QFile::ReadOnly))
	{
		if (file_out.open(QIODevice::WriteOnly))
		{
			QTextStream in(&file_in);
			QTextStream out(&file_out);
			do
			{
				line = in.readLine();
				out << line<< "\n";
			}
			while (!line.isNull());
		}
	}
	file_out.close();
}





