/**
 * \file
 * $Revision: 15361 $
 * $Date: 2014-07-10 16:30:58 +0200 (Thu, 10 Jul 2014) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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
#include <QFile>
#include <QStringList>


#include "AgeModelReader.h"


#include "ErrorOpeningFileForReadingException.h"
#include "app-logic/AgeModelCollection.h"

QString GPlatesFileIO::AgeModelReader::s_delimiter = QString("\t");
QString GPlatesFileIO::AgeModelReader::s_comment_marker = QString("@C");
QString GPlatesFileIO::AgeModelReader::s_geotimescale_marker = QString("@GEOTIMESCALE");

namespace
{
	void
	parse_geotimescale(
			const QString &line,
			GPlatesAppLogic::AgeModelCollection &model,
			const QString &geotimescale_marker)
	{
		// Hardcoded for now...13 is the length of @GEOTIMESCALE"
		QString temp = line;
		temp.remove(0,geotimescale_marker.length());
		temp = temp.split("|").first();

		// Strip leading quotation mark.
		temp = temp.remove(0,1);

		// Create new AgeModel, add to collection
		GPlatesAppLogic::AgeModel age_model(temp);
		model.add_age_model(age_model);
	}

	void
	parse_chron_line(
			const QString &line,
			GPlatesAppLogic::AgeModelCollection &model,
			const QString &delimiter,
			const QString &comment_marker)
	{
		// If there's a comment, grab it first, as it may contain the delimiter symbol and hence mess up string splitting.
		QStringList split_at_comment = line.split(comment_marker);
		QString comment;
		if (split_at_comment.length()>1)
		{
			comment = split_at_comment.at(1);
		}
		qDebug() << "Comment: " << comment;

		QStringList list = split_at_comment.first().split(delimiter,QString::SkipEmptyParts);


		// From here we assume that the first term in "list" is the Chron string, and subsequent terms are the ages (or NULL marker)
		// for each of the models in @a model.
		QString chron = list.first();
		qDebug() << "Chron: " << chron;


		if (list.length() < 2)
		{
			qWarning() << "No ages found for chron " << chron;
			return;
		}

		// Check number of fields vs number of models. If we don't have matching numbers, give a warning, but do the best we can.
		if (list.length() != model.number_of_age_models() + 1)
		{
			qWarning() << "Chron line does not contain the correct number of model ages; there are " << model.number_of_age_models() << " models and " << list.length()-1 << " ages.";
		}

		for (int count = 0; count < model.number_of_age_models() ; ++count)
		{
			if (count > list.length())
			{
				return;
			}

			bool ok;
			double age = list.at(count+1).toDouble(&ok);

			if (ok)
			{
				model.add_chron_to_model(count,chron,age);
			}
		}
		model.add_chron_metadata(chron,comment);

	}

	void
	parse_line(
			const QString &line,
			GPlatesAppLogic::AgeModelCollection &model,
			const QString &delimiter,
			const QString &geotimescale_marker,
			const QString &comment_marker)
	{
		if (line.isEmpty())
		{
			return;
		}

		if (line.startsWith("#"))
		{
			return;
		}

		qDebug() << line;

		if (line.startsWith(geotimescale_marker))
		{
			parse_geotimescale(line,model,geotimescale_marker);
		}
		else
		{
			parse_chron_line(line,model,delimiter,comment_marker);
		}
	}

}

void
GPlatesFileIO::AgeModelReader::read_file(
		const QString &filename,
		GPlatesAppLogic::AgeModelCollection &model)
{
	qDebug() << "Filename: " << filename;
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	QTextStream input(&file);

	model.clear();
	while(!input.atEnd())
	{
		parse_line(input.readLine(),model,s_delimiter,s_geotimescale_marker,s_comment_marker);
	}

	model.set_filename(filename);
}
