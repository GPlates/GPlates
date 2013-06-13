/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2012-02-29 17:05:33 +0100 (Wed, 29 Feb 2012) $
 *
 * Copyright (C) 2012 Geological Survey of Norway
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

#include "qt-widgets/HellingerModel.h"

#include "HellingerWriter.h"


void
GPlatesFileIO::HellingerWriter::write_pick_file(
	const QString &filename,
	GPlatesQtWidgets::HellingerModel &hellinger_model)
{
	QFile file(filename);
	QTextStream out(&file);

	if (file.open(QIODevice::WriteOnly))
	{
		QStringList load_data = hellinger_model.get_data();
		QString pick_state;
		if (!load_data.isEmpty())
		{
			int a = 0;
			for (int i=0; i < (load_data.size()/6); i++)
			{
				bool ok;
				if (!load_data.at(a+5).toInt(&ok))
				{
					if (load_data.at(a+1).toInt() == GPlatesQtWidgets::MOVING_SEGMENT_TYPE)
					{

						pick_state = QString("%1").arg(GPlatesQtWidgets::DISABLED_MOVING_SEGMENT_TYPE);

					}
					else if (load_data.at(a+1).toInt() == GPlatesQtWidgets::FIXED_SEGMENT_TYPE)
					{

						pick_state = QString("%1").arg(GPlatesQtWidgets::DISABLED_FIXED_SEGMENT_TYPE);
					}
				}
				else if (load_data.at(a+5).toInt(&ok))
				{
					pick_state = load_data.at(a+1);
				}
				out << pick_state<<" "<<load_data.at(a)<<" "<<load_data.at(a+2)
					<<" "<<load_data.at(a+3)<<" "<<load_data.at(a+4)<<"\n";
				a=a+6;
			}
		}
	}
}
