/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H
#define GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H

#include <QWidget>
#include "file-io/PropertyMapper.h"

namespace ShapefileAttributes {
	enum ModelProperties {

		PLATEID = 0,	
		FEATURE_TYPE,
		BEGIN,
		END,
		NAME,
		DESCRIPTION
	};

	static const QString model_properties[] = {
		"ReconstructionPlateId",
		"FeatureType",
		"Begin",
		"End",
		"Name",
		"Description"
	};
	
	static const QString default_attributes[] = {
		"PLATEID1",
		"TYPE",
		"FROMAGE",
		"TOAGE",
		"NAME",
		"DESCR"
	};

}

namespace GPlatesQtWidgets
{
	class ShapefilePropertyMapper:
		public GPlatesFileIO::PropertyMapper
	{
	public:
		ShapefilePropertyMapper(
			QWidget *parent_window_ptr):
		d_parent_window_ptr(parent_window_ptr)
		{ }

		~ShapefilePropertyMapper()
		{ }
		
		/**
		 * Fills model_to_attribute_map.
		 */
		bool
		map_properties(
				QString &filename,
				QStringList &field_names,
				QMap<QString,QString> &model_to_attribute_map,
				bool remapping);


	private:
		/**
		 * The Qt window which will be the parent of the dialogs.
		 */
		QWidget *d_parent_window_ptr;

		// Make copy and assignment private.
		ShapefilePropertyMapper(
			const ShapefilePropertyMapper &other);

		ShapefilePropertyMapper &
		operator=(
			const ShapefilePropertyMapper &other);

		/**
		 * Obtains the initial shapefile attribute mapping from the
		 * <name>.shp.gplates.xml file, if it exists. Failing that, the
		 * mapping is obtained from the ShapefileAttributeMapping dialog.  
		 */
		static
		bool
		map_initial_properties(
			QString &filename,
			QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map,
			QWidget *parent_);

		/**
		 * Obtains the shapefile attribute mapping from the <name>.shp.gplates.xml,
		 * and opens the ShapefileAttributeRemapping dialog to allow the user to 
		 * change the mapping. 
		 */
		static
		bool
		map_remapped_properties(
			QString &filename,
			QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map,
			QWidget *parent_);
	};

}

#endif // GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H
