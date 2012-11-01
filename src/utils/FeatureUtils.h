/* $Id: GenericMapper.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file validate filename template.
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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

#ifndef GPLATES_UTILS_FEATUREUTILS_H
#define GPLATES_UTILS_FEATUREUTILS_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include "maths/Real.h"

#include "model/types.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"

namespace GPlatesPropertyValues
{
	class GpmlPlateId;
}
namespace GPlatesModel
{
	class FeatureHandle;
}

namespace GPlatesUtils
{
	boost::optional<GPlatesModel::integer_plate_id_type>
	get_int_plate_id(
			const GPlatesModel::FeatureHandle* feature_ptr);

	boost::optional<GPlatesMaths::Real>
	get_age(
			const GPlatesModel::FeatureHandle* feature_ptr,
			const GPlatesMaths::Real current_time);

	boost::tuple<
			GPlatesMaths::Real, 
			GPlatesMaths::Real>
	get_start_end_time(
			const GPlatesModel::FeatureHandle* feature_ptr);


	boost::optional<GPlatesModel::PropertyName>
	convert_property_name(
			const QString&);

	
	inline
	boost::optional<QString>
	get_shapefile_attribute(
			const QString& name)
	{
		boost::optional<QString> shape_name = boost::none;
		QRegExp rx("^\\s*(gpml:shapefileAttributes)\\s*:\\s*\\b(\\w+)\\b\\s*"); // gpml:shapefileAttributes
		if(rx.indexIn(name) != -1)
		{
			shape_name = rx.cap(2);
			//qDebug() << "Shapefile attribute name: " << *shape_name;
		}
		return shape_name;
	}


	inline
	QString
	property_value_to_qstring(
			const GPlatesModel::PropertyValue& data)
	{
		std::stringstream ss;
		data.print_to(ss);
		return QString(ss.str().c_str());
	}

}

#endif //GPLATES_UTILS_FEATUREUTILS_H












