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
 
#ifndef GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H
#define GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H

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
		ShapefilePropertyMapper()
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
		bool
		map_initial_properties(
			QString &filename,
			QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map);

		/**
		 * Obtains the shapefile attribute mapping from the <name>.shp.gplates.xml,
		 * and opens the ShapefileAttributeRemapping dialog to allow the user to 
		 * change the mapping. 
		 */
		bool
		map_remapped_properties(
			QString &filename,
			QStringList &field_names,
			QMap<QString,QString> &model_to_attribute_map);
	};

}

#endif // GPLATES_QTWIDGETS_SHAPEFILEPROPERTYMAPPER_H
