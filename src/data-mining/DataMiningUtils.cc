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
#include <boost/foreach.hpp>

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "feature-visitors/ShapefileAttributeFinder.h"
#include "file-io/FeatureCollectionReaderWriter.h"

#include "Filter.h"
#include "CoRegConfigurationTable.h"
#include "ReducerTypes.h"
#include "OpaqueDataToDouble.h"
#include "DataMiningUtils.h"
#include "DualGeometryVisitor.h"
#include "GetValueFromPropertyVisitor.h"
#include "IsCloseEnoughChecker.h"

using namespace GPlatesUtils;

boost::optional< double > 
GPlatesDataMining::DataMiningUtils::minimum(
		const std::vector< double >& input)
{
	boost::optional< double >  ret = boost::none;
	std::vector< double >::const_iterator it = input.begin(), it_end = input.end();

	for(; it != it_end; it++)
	{
		if(ret)
			*ret = std::min( (*ret), (*it) );
		else
			ret = *it;
	}
	return ret;
}


void
GPlatesDataMining::DataMiningUtils::convert_to_double_vector(
		std::vector<OpaqueData>::const_iterator begin,
		std::vector<OpaqueData>::const_iterator end,
		std::vector<double>& result)
{
	boost::optional<double> tmp = boost::none;
	for(; begin != end; begin++)
	{
		tmp = boost::apply_visitor(ConvertOpaqueDataToDouble(), *begin);
		if(tmp)
			result.push_back(*tmp);
	}
}


double
GPlatesDataMining::DataMiningUtils::shortest_distance(
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
		const GPlatesAppLogic::ReconstructedFeatureGeometry* geo)
{
	double ret = DEFAULT_RADIUS_OF_EARTH * PI;
	BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* seed, seed_geos)
	{
		//use (DEFAULT_RADIUS_OF_EARTH * PI) as range, so the distance can always be calculated.
		IsCloseEnoughChecker checker((DEFAULT_RADIUS_OF_EARTH * PI), true);
		DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(
				*(geo->reconstructed_geometry()),
				*(seed->reconstructed_geometry()),
				&checker);
		dual_visitor.apply();
		boost::optional<double> tmp = checker.distance();
	
		if(tmp && ret > *tmp)
			ret = *tmp;
	}
	return ret;
}


GPlatesDataMining::OpaqueData
GPlatesDataMining::DataMiningUtils::get_property_value_by_name(
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
		QString name)
{
	using namespace GPlatesModel;
	FeatureHandle::const_iterator it = feature_ref->begin(), it_end = feature_ref->end();
	
	for(; it != it_end; it++)
	{
		if((*it)->property_name().get_name() == UnicodeString(name))
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
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
		QString name)
{
	GPlatesModel::FeatureHandle::const_iterator it = feature_ref->begin(), it_end = feature_ref->end();
	
	for(; it != it_end; it++)
	{
		if((*it)->property_name().get_name() == "shapefileAttributes")
		{
			GPlatesFeatureVisitors::ShapefileAttributeFinder visitor(name);
			(*it)->accept_visitor(visitor);
			if(1 != std::distance(visitor.found_qvariants_begin(),visitor.found_qvariants_end()))
			{
				qDebug() << "More than one property found in shape file attribute.";
				qDebug() << "But this is a one to one mapping. So, only return the first value.";
				qDebug() << "More than one shape file attributes have the same name. Please check you data.";
			}
			return convert_qvariant_to_Opaque_data(*visitor.found_qvariants_begin());
		}
	}
	return EmptyData;
}

std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
GPlatesDataMining::DataMiningUtils::load_files(
		const std::vector<QString>& filenames,
		std::vector<GPlatesFileIO::File::non_null_ptr_type>& files,
		GPlatesFileIO::ReadErrorAccumulation* read_errors)
{
	using namespace GPlatesFileIO;
	GPlatesModel::ModelInterface model;
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> ret;
	ReadErrorAccumulation acc;
	if(!read_errors)
		read_errors = &acc;

	BOOST_FOREACH(const QString& filename, filenames)
	{
		File::non_null_ptr_type file = File::create_file(FileInfo(filename));
		files.push_back(file);
		read_feature_collection(file->get_reference(), model, *read_errors);
		ret.push_back(file->get_reference().get_feature_collection());
	}
	return ret;
}

std::vector<QString>
GPlatesDataMining::DataMiningUtils::load_cfg(
		const QString& cfg_filename,
		const QString& section_name)
{
	std::string str;
	std::vector<QString> ret;
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

















