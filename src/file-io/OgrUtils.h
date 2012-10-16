/* $Id$ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date$
* 
* Copyright (C) 2009, 2010, 2012 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEUTILS_H
#define GPLATES_FILEIO_SHAPEFILEUTILS_H

#include <QFileInfo>
#include <QMap>
#include <QString>

#include "property-values/GpmlKeyValueDictionary.h"
#include "ReconstructionGeometryExportImpl.h"

namespace GPlatesFileIO
{
	namespace OgrUtils
	{
		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


#if 0
		typedef std::map<QString, std::pair<QString,QString> > feature_map_type;
		typedef feature_map_type::const_iterator feature_map_const_iterator;		

		const feature_map_type &
			build_feature_map()
		{

			// The data for the following map has been taken from 
			// 1. (feature-type-to-two-letter-code) The "build_feature_map_type" map in PlatesLineFormatReader.cc 
			// 2. (geometry-type-to-feature-type) The various "create_*" functions in PlatesLinesFormatReader.cc
			//
			// FIXME: we should get this information from a common source, rather than having two independent sources.  
			static feature_map_type map;
			map["AR"] = std::make_pair("AseismicRidge","centerLineOf");
			map["BA"] = std::make_pair("Bathymetry","centerLineOf");
			map["BS"] = std::make_pair("Basin","outlineOf");
			map["CB"] = std::make_pair("PassiveContinentalBoundary","centerLineOf");
			map["CF"] = std::make_pair("ContinentalFragment","outlineOf");
			map["CM"] = std::make_pair("PassiveConinentalBoundary","centerLineOf");
			map["CO"] = std::make_pair("PassiveContinentalBoundary","centerLineOf");
			map["CR"] = std::make_pair("Craton","outlineOf");
			map["CS"] = std::make_pair("Coastline","centerLineOf");
			map["EC"] = std::make_pair("ExtendedContinentalCrust","centerLineOf");
			map["FT"] = std::make_pair("Fault","centerLineOf");
			map["FZ"] = std::make_pair("FractureZone","centerLineOf");
			map["GR"] = std::make_pair("OldPlatesGridMark","centerLineOf");
			map["GV"] = std::make_pair("Gravimetry","outlineOf");
			map["HF"] = std::make_pair("HeatFlow","outlineOf");
			map["HS"] = std::make_pair("HotSpot","position");
			map["HT"] = std::make_pair("HotSpotTrail","unclassifiedGeometry");
			map["IA"] = std::make_pair("IslandArc","outlineOf");
			map["IC"] = std::make_pair("Isochron","centerLineOf");
			map["IM"] = std::make_pair("Isochron","centerLineOf");
			map["IP"] = std::make_pair("SedimentThickness","outlineOf");
			map["IR"] = std::make_pair("IslandArc","centerLineOf");

			// -might- be Ice Shelf, might be Isochron. We don't know.
			// It appears IS covers IC and IM.
			//
			// Update 2012/9/4: Maria Seton requested "IS" result in an isochron instead of unclassified feature.
			map["IS"] = std::make_pair("Isochron","centerLineOf");

			map["LI"] = std::make_pair("GeologicalLineation","centerLineOf");
			map["MA"] = std::make_pair("Magnetics","outlineOf");
			map["NF"] = std::make_pair("gpmlFault","centerLineOf");
			map["OB"] = std::make_pair("OrogenicBelt","centerLineOf");
			map["OP"] = std::make_pair("BasicRockUnit","outlineOf");
			map["OR"] = std::make_pair("OrogenicBelt","centerLineOf");
			map["PB"] = std::make_pair("InferredPaleoBoundary","centerLineOf");
			map["PC"] = std::make_pair("MagneticAnomalyIdentification","position");
			map["PM"] = std::make_pair("MagneticAnomalyIdentification","position");
			map["RA"] = std::make_pair("IslandArc","centerLineOf");
			map["RF"] = std::make_pair("Fault","centerLineOf");
			map["RI"] = std::make_pair("MidOceanRidge","centerLineOf");
			map["SM"] = std::make_pair("Seamount","unclassifiedGeometry");
			map["SS"] = std::make_pair("Fault","centerLineOf");
			map["SU"] = std::make_pair("Suture","centerLineOf");
			map["TB"] = std::make_pair("TerraneBoundary","centerLineOf");
			map["TC"] = std::make_pair("TransitionalCrust","outlineOf");
			map["TF"] = std::make_pair("Transform","centerLineOf");
			map["TH"] = std::make_pair("Fault","centerLineOf");
			map["TO"] = std::make_pair("Topography","outlineOf");
			map["TR"] = std::make_pair("SubductionZone","centerLineOf");
			map["UN"] = std::make_pair("UnclassifiedFeature","unclassifiedGeometry");
			map["VO"] = std::make_pair("Volcano","unclassifiedGeometry");
			map["PL"] = std::make_pair("Pluton","unclassifiedGeometry");
			map["OH"] = std::make_pair("Ophiolite","unclassifiedGeometry");
			map["VP"] = std::make_pair("LargeIgneousProvince","outlineOf");
			map["XR"] = std::make_pair("MidOceanRidge","centerLineOf");
			map["XT"] = std::make_pair("SubductionZone","centerLineOf");

			return map;
		}	
#endif

		typedef QMap<QString, QString > feature_map_type;
		typedef feature_map_type::const_iterator feature_map_const_iterator;		

		
		const feature_map_type &
			build_feature_map();


		/**
		* Given a shapefile name in the form <name>.shp , this will produce a filename of the form
		* <name>.gplates.xml
		*/
		QString
		make_shapefile_xml_filename(
			const QFileInfo &file_info);

		/**
		* Writes the data in the QMap<QString,QString> to an xml file.
		*/
		void
		save_attribute_map_as_xml_file(
			const QString &filename,
			const QMap<QString,QString> &model_to_attribute_map);
	
		void
		create_default_kvd_from_collection(
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> 
				&default_key_value_dictionary);

		void
		add_plate_id_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);

		void
		add_reconstruction_fields_to_kvd(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
			const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
			const double &reconstruction_time);

		void
		add_referenced_files_to_kvd(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
			const referenced_files_collection_type &referenced_files);

		void
		add_standard_properties_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_feature_type_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_begin_and_end_time_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_name_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_description_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_feature_id_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

		void
		add_conjugate_plate_id_to_kvd(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd);	

}



}

#endif // GPLATES_FILEIO_SHAPEFILEUTILS_H
