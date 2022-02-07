/* $Id$ */

/**
 * \file .
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <fstream>
#include <utility>
#include <boost/foreach.hpp>

#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "feature-visitors/ShapefileAttributeFinder.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "global/LogException.h"
#include "maths/GeometryDistance.h"
#include "maths/MathsUtils.h"
#include "utils/Earth.h"

#include "CoRegConfigurationTable.h"
#include "Types.h"
#include "OpaqueDataToDouble.h"
#include "DataMiningUtils.h"
#include "GetValueFromPropertyVisitor.h"


using namespace GPlatesUtils;

// Undefine the min and max macros as they can interfere with the min and
// max functions in std::numeric_limits<T>, on Visual Studio.
#if defined(_MSC_VER)
	#undef min
	#undef max
#endif

boost::optional< double > 
GPlatesDataMining::DataMiningUtils::minimum(
		const std::vector< double >& input)
{
	std::vector< double >::const_iterator 
		it = input.begin(), 
		it_end = input.end();
	boost::optional<double> ret=boost::none;
	for(; it != it_end;	it++)
	{
		ret = ret ?  (std::min)(*ret, *it) :  *it;
	}
	return ret;
}


void
GPlatesDataMining::DataMiningUtils::convert_to_double_vector(
		std::vector<OpaqueData>::const_iterator begin,
		std::vector<OpaqueData>::const_iterator end,
		std::vector<double>& result)
{
	for(; begin!=end; begin++)
	{
		if(boost::optional<double> tmp = 
			boost::apply_visitor(ConvertOpaqueDataToDouble(), *begin))
		{
			result.push_back(*tmp);
		}
	}
}


double
GPlatesDataMining::DataMiningUtils::shortest_distance(
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
		const GPlatesAppLogic::ReconstructedFeatureGeometry* geo)
{
	if(seed_geos.empty())
		throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Invalid input");

	// Start with the maximum possible distance (two antipodal points).
	double min_distance = GPlatesMaths::PI * GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS;

	BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* seed, seed_geos)
	{
		const GPlatesMaths::AngularDistance angular_dist_between_geos = minimum_distance(
				*geo->reconstructed_geometry(),
				*seed->reconstructed_geometry(),
				// If either (or both) geometry is a polygon then the distance will be zero
				// if the other geometry overlaps its interior...
				true/*geometry1_interior_is_solid*/,
				true/*geometry2_interior_is_solid*/);

		const double dist_between_geos = angular_dist_between_geos.calculate_angle().dval() *
				GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS;

		if (dist_between_geos < min_distance)
		{
			min_distance = dist_between_geos;
		}
	}

	return min_distance;
}

double
GPlatesDataMining::DataMiningUtils::shortest_distance(
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& first,
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& second)
{
	if(first.empty()||second.empty())
		throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Invalid input");

	// Start with the maximum possible distance (two antipodal points).
	double min_distance = GPlatesMaths::PI * GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS;

	BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* r, second)
	{
		const double shortest_dist = shortest_distance(first, r);

		if (shortest_dist < min_distance)
		{
			min_distance = shortest_dist;
		}
	}
	return min_distance;
}


GPlatesDataMining::OpaqueData
GPlatesDataMining::DataMiningUtils::get_property_value_by_name(
		const GPlatesModel::FeatureHandle* feature_ptr,
		QString name)
{
	using namespace GPlatesModel;
	FeatureHandle::const_iterator it = feature_ptr->begin(), it_end = feature_ptr->end();
	
	//hack for Jo
	if(name == "gpml feature type")
	{
		return feature_ptr->feature_type().get_name().qstring();
	}

	for(; it != it_end; it++)
	{
		if((*it)->get_property_name().get_name() == UnicodeString(name))
		{
			GetValueFromPropertyVisitor visitor;
			(*it)->accept_visitor(visitor);
			std::vector<OpaqueData>& data_vec = visitor.get_data(); 
			if(!data_vec.empty())
			{
				return data_vec.at(0);
			}
		}
	}
	return EmptyData;
}


GPlatesDataMining::OpaqueData
GPlatesDataMining::DataMiningUtils::convert_qvariant_to_Opaque_data(
		const QVariant& data)
{
	switch (data.type())
	{
	case QVariant::Bool:
		return OpaqueData(data.toBool());

	case QVariant::Int:
		return OpaqueData(data.toInt());
		
	case QVariant::Double:
		return OpaqueData(data.toDouble());

	case QVariant::String:
		return OpaqueData(data.toString());
	
	default:
		return EmptyData;
	}
}


GPlatesDataMining::OpaqueData
GPlatesDataMining::DataMiningUtils::get_shape_file_value_by_name(
		const GPlatesModel::FeatureHandle* feature_ptr,
		QString name)
{
	GPlatesModel::FeatureHandle::const_iterator it = feature_ptr->begin(), it_end = feature_ptr->end();
	
	for(; it != it_end; it++)
	{
		if((*it)->get_property_name().get_name() == "shapefileAttributes")
		{
			GPlatesFeatureVisitors::ShapefileAttributeFinder visitor(name);
			(*it)->accept_visitor(visitor);
			if(1 < std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
			{
				qWarning() << "Found more than one shape file attribute with same attribute name.";
				qWarning() << "Since this is a one-to-one mapping only the first value will be used.";
				qWarning() << "Please check your data.";
			}
			if(0 == std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
			{
				continue;
			}
			return convert_qvariant_to_Opaque_data(*visitor.found_qvariants_begin());
		}
	}
	return EmptyData;
}

std::vector<QString>
GPlatesDataMining::DataMiningUtils::load_cfg(
		const QString& cfg_filename,
		const QString& section_name)
{
	std::string str;
	std::vector<QString> ret;
	// FIXME: We should replace usage of std::ifstream with the appropriate Qt class.
	std::ifstream ifs ( cfg_filename.toStdString().c_str() , std::ifstream::in );

	while (ifs.good()) // go to the line starting with section_name.
	{
		std::getline(ifs,str);
		QString s(str.c_str());
		s = s.trimmed().simplified();
		if(s.startsWith(section_name))
			break;
	}

	while (ifs.good())
	{
		std::getline(ifs,str);
		QString s(str.c_str());
		s = s.trimmed().simplified();

		if(s.startsWith("#"))//skip comments
			continue;
		if(s.isEmpty()) //finish at an empty line.
			break;
		ret.push_back(s);
		qDebug() << s;
	}
	return ret;
}






















