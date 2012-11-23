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

#include "app-logic/AppLogicFwd.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "feature-visitors/ShapefileAttributeFinder.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "global/LogException.h"

#include "CoRegConfigurationTable.h"
#include "Types.h"
#include "OpaqueDataToDouble.h"
#include "DataMiningUtils.h"
#include "DualGeometryVisitor.h"
#include "GetValueFromPropertyVisitor.h"
#include "IsCloseEnoughChecker.h"

#include <boost/foreach.hpp>

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
		ret = ret ?  std::min(*ret, *it) :  *it;
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

	double dist = -1;
	BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* seed, seed_geos)
	{
		//use (DEFAULT_RADIUS_OF_EARTH_KMS * PI) as range, so the distance can always be calculated.
		IsCloseEnoughChecker checker((DEFAULT_RADIUS_OF_EARTH_KMS * PI), true);
		DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(
				*(geo->reconstructed_geometry()),
				*(seed->reconstructed_geometry()),
				&checker);
		dual_visitor.apply();
		boost::optional<double> tmp = checker.distance();
		if(!tmp)
			continue;
	
		dist = (0 > dist) ? *tmp : (*tmp < dist) ? *tmp : dist;
	}
	return dist;
}

double
GPlatesDataMining::DataMiningUtils::shortest_distance(
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& first,
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& second)
{
	if(first.empty()||second.empty())
		throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Invalid input");

	double dist = -1;
	BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* r, second)
	{
		double tmp = shortest_distance(first,r);
		dist = (0 > dist) ? tmp : (tmp < dist) ? tmp : dist;
	}
	return dist;
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
		const GPlatesModel::FeatureHandle* feature_ptr,
		QString name)
{
	GPlatesModel::FeatureHandle::const_iterator it = feature_ptr->begin(), it_end = feature_ptr->end();
	
	for(; it != it_end; it++)
	{
		if((*it)->property_name().get_name() == "shapefileAttributes")
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

GPlatesFileIO::File::non_null_ptr_type
GPlatesDataMining::DataMiningUtils::load_file(
		const QString fn,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		GPlatesFileIO::ReadErrorAccumulation* read_errors)
{
	using namespace GPlatesFileIO;
	GPlatesModel::ModelInterface model;
	GPlatesModel::FeatureCollectionHandle::weak_ref ret;
	ReadErrorAccumulation acc;
	if(!read_errors)
		read_errors = &acc;

	File::non_null_ptr_type file = File::create_file(FileInfo(fn));
	file_format_registry.read_feature_collection(file->get_reference(), *read_errors);

	return file;	
}

std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
GPlatesDataMining::DataMiningUtils::load_files(
		const std::vector<QString>& filenames,
		std::vector<GPlatesFileIO::File::non_null_ptr_type>& files,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
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
		file_format_registry.read_feature_collection(file->get_reference(), *read_errors);
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


std::vector<GPlatesModel::FeatureHandle::weak_ref>
GPlatesDataMining::DataMiningUtils::get_all_seed_features(
		GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type co_proxy)
{
	using namespace GPlatesAppLogic;
	std::vector<GPlatesModel::FeatureHandle::weak_ref> ret;

	std::vector<reconstruct_layer_proxy_non_null_ptr_type> seed_proxies =
		co_proxy->get_coregistration_seed_layer_proxy();

	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_seed_features;
	BOOST_FOREACH(reconstruct_layer_proxy_non_null_ptr_type p, seed_proxies)
	{
		p->get_reconstructed_features(reconstructed_seed_features);

		BOOST_FOREACH(ReconstructContext::ReconstructedFeature rsf, reconstructed_seed_features)
		{
			const GPlatesModel::FeatureHandle::weak_ref f = rsf.get_feature();
			if(f.is_valid())
				ret.push_back(f);
		}
	}
	return ret;
}





















