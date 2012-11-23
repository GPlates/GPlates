/* $Id$ */

/**
 * \file 
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

#ifndef GPLATESDATAMINING_DATAMININGUTILS_H
#define GPLATESDATAMINING_DATAMININGUTILS_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <QString>
#include <QVariant>
#include <vector>

#include "OpaqueData.h"

#include "app-logic/CoRegistrationLayerProxy.h"
#include "app-logic/ReconstructContext.h"

#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/FeatureHandle.h"

#include <boost/foreach.hpp>

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}
}

namespace GPlatesDataMining
{
	class CoRegConfigurationTable;

	namespace DataMiningUtils
	{
		/*
		* Return the minimum value 
		* Return boost::none if the input vector is empty.
		*/
		boost::optional< double > 
		minimum(
				const std::vector< double >& input);

		/*
		* Convert a vector of OpaqueData to a vector of double.
		*/
		void
		convert_to_double_vector(
				std::vector<OpaqueData>::const_iterator begin,
				std::vector<OpaqueData>::const_iterator end,
				std::vector<double>& result);

		/*
		* Calculate the distances between each two geometries
		* return the shortest distance.
		*/
		double
		shortest_distance(
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
				const GPlatesAppLogic::ReconstructedFeatureGeometry* geo);
		
		double
		shortest_distance(
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& first,
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& second);
		/*
		* Given the feature handle, find a property by the name.
		*/
		OpaqueData
		get_property_value_by_name(
				const GPlatesModel::FeatureHandle* feature_prt,
				QString prop_name);
	
		inline
		OpaqueData
		get_property_value_by_name(
				GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
				QString prop_name)
		{
			return get_property_value_by_name(feature_ref.handle_ptr(), prop_name);
		}

		/*
		* Since the shape file visitor return QVariant,
		* convert QVariant to OpaqueData
		*/
		OpaqueData
		convert_qvariant_to_Opaque_data(
				const QVariant& data);

		/*
		* Fine the shape file attribute from the given feature handle 
		*/
		OpaqueData
		get_shape_file_value_by_name(
				const GPlatesModel::FeatureHandle* feature_ptr,
				QString attr_name);

		inline
		OpaqueData
		get_shape_file_value_by_name(
				GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
				QString attr_name)
		{
			return get_shape_file_value_by_name(feature_ref.handle_ptr(),attr_name);
		}


		std::vector<GPlatesModel::FeatureHandle::weak_ref>
		get_all_seed_features(
				GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type);
		

		GPlatesFileIO::File::non_null_ptr_type 
		load_file(
				const QString fn,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				GPlatesFileIO::ReadErrorAccumulation* read_errors = NULL);
		
		/*
		* Given a list of file names, load all the files and
		* return a vector of weak reference of feature collection handle
		*/
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
		load_files(
				const std::vector<QString>& filenames,
				std::vector<GPlatesFileIO::File::non_null_ptr_type>& files,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				GPlatesFileIO::ReadErrorAccumulation* read_errors = NULL);

		
		inline
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
		load_files(
				const std::vector<const char*>& filenames,
				std::vector<GPlatesFileIO::File::non_null_ptr_type>& files,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
				GPlatesFileIO::ReadErrorAccumulation* read_errors = NULL)
		{
			std::vector<QString> new_filenames;
			BOOST_FOREACH(const char* filename, filenames)
			{
				new_filenames.push_back(QString(filename));
			}
			return load_files(new_filenames,files,file_format_registry);
		}
		
		/*
		* Return particular section of configuration file. 
		*/
		std::vector<QString>
		load_cfg(
				const QString& cfg_filename,
				const QString& section_name);

		/*
		* Convenient function for loading cfg section having only one line.
		*/
		inline
		QString
		load_one_line_cfg(
				const QString& cfg_file,
				const QString& section_name)
		{
			std::vector<QString> cfgs = load_cfg(cfg_file,section_name);
			return cfgs.size() ? cfgs[0] : QString();
		}

		static std::vector<
				boost::tuple<
						std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>,
						const GPlatesAppLogic::ReconstructedFeatureGeometry* > > RFG_INDEX_VECTOR;
	}
}
#endif
