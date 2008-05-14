/* $Id: ShapefileAttributeMapperDialog.h 2681 2008-03-31 20:32:35Z rwatson $ */

/**
 * \file 
 * $Revision: 2681 $
 * $Date: 2008-03-31 22:32:35 +0200 (ma, 31 mar 2008) $ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include <QString>
#include <QStringList>

#include <map>

#include "ShapefileAttributeMapperDialog.h"
#include "ShapefileAttributeRemapperDialog.h"
#include "ShapefilePropertyMapper.h"

bool
GPlatesQtWidgets::ShapefilePropertyMapper::map_properties(
		QString &filename,
		QStringList &field_names,
		QMap<QString,QString> &model_to_attribute_map,
		bool remapping)
{
	if (remapping)
	{
		return (map_remapped_properties(filename,field_names,model_to_attribute_map));
	}
	else{
		return (map_initial_properties(filename,field_names,model_to_attribute_map));
	}
#if 0
	ShapefileAttributeMapperDialog mapper;
	mapper.setup(filename,field_names,model_to_attribute_map,remapping);
	mapper.exec();

	if (mapper.result() == QDialog::Rejected){
		return false;
	}
	else{
		return true;
	}
#endif
}

bool
GPlatesQtWidgets::ShapefilePropertyMapper::map_initial_properties(
		QString &filename,
		QStringList &field_names,
		QMap<QString,QString> &model_to_attribute_map)
{
	ShapefileAttributeMapperDialog mapper;
	mapper.setup(filename,field_names,model_to_attribute_map);
	mapper.exec();

	if (mapper.result() == QDialog::Rejected){
		return false;
	}
	else{
		return true;
	}
}

bool
GPlatesQtWidgets::ShapefilePropertyMapper::map_remapped_properties(
		QString &filename,
		QStringList &field_names,
		QMap<QString,QString> &model_to_attribute_map)
{
	ShapefileAttributeRemapperDialog mapper;
	mapper.setup(filename,field_names,model_to_attribute_map);
	mapper.exec();

	if (mapper.result() == QDialog::Rejected){
		return false;
	}
	else{
		return true;
	}
}
