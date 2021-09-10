/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2011, 2012, 2015 Geological Survey of Norway
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


#include <vector>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>


#include "Gdal.h"
#include "GdalUtils.h"
#include "ErrorOpeningFileForWritingException.h"
#include "Ogr.h"
#include "OgrException.h"
#include "OgrUtils.h"
#include "OgrWriter.h"

#include "feature-visitors/ToQvariantConverter.h"

#include "maths/LatLonPoint.h"

#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"


const QString POINT_SUFFIX("_point");
const QString POLYLINE_SUFFIX("_polyline");
const QString POLYGON_SUFFIX("_polygon");

namespace{

	typedef std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> element_type;
	typedef element_type::const_iterator element_iterator_type;
	typedef std::map<QString, QStringList> file_to_driver_map_type;

	bool
	file_type_does_not_support_mixing_single_and_multi_line_strings_in_layer(
			const QString &extension)
	{
		return ((extension == "GMT") || (extension == "gmt") || (extension == "gpkg"));
	}

	bool
	file_type_does_not_support_mixing_single_and_multi_polygons_in_layer(
			const QString &extension)
	{
		return ((extension == "GMT") || (extension == "gmt") || (extension == "gpkg"));
	}

	bool
	file_type_does_not_support_layer_deletion(
			const QString &extension)
	{
		return ((extension == "GMT") || (extension == "gmt"));
	}

	enum ogr_driver_string
	{
		FORMAT_NAME,
		CODE
	};

	/**
	 * Create a map of file extension to OGR driver information.
	 *
	 * The map keys are the lower case file extensions.
	 * The map data are the "Format Name" and "Code" terms from the list of OGR Vector Formats
	 * (http://www.gdal.org/ogr/ogr_formats.html)
	 */
	file_to_driver_map_type
	create_file_to_driver_map()
	{
		static file_to_driver_map_type map;
		QStringList shp_driver;
		shp_driver << "ESRI Shapefile" << "ESRI Shapefile";
		map["shp"] = shp_driver;

		QStringList gmt_driver;
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
		// GDAL2 changed the driver name from "GMT" to "OGR_GMT".
		// This was done when the GDAL/OGR drivers were unified,
		// see https://trac.osgeo.org/gdal/changeset?reponame=&old=27384%40trunk%2Fgdal%2Fogr%2Fogrsf_frmts%2Fgmt%2Fogrgmtdriver.cpp&new=27384%40trunk%2Fgdal%2Fogr%2Fogrsf_frmts%2Fgmt%2Fogrgmtdriver.cpp
		gmt_driver << "GMT" << "OGR_GMT";
#else
		gmt_driver << "GMT" << "GMT";
#endif
		map["gmt"] = gmt_driver;

		QStringList geojson_driver;
		geojson_driver << "GeoJSON" << "GeoJSON";
		map["geojson"] = geojson_driver;
		map["json"] = geojson_driver;

		QStringList gpkg_driver;
		gpkg_driver << "GeoPackage" << "GPKG";
		map["gpkg"] = gpkg_driver;

		return map;
	}

	QString
	get_driver_name_from_file_extension(
		QString file_extension)
	{
		file_extension = file_extension.toLower();
	
		file_to_driver_map_type map = create_file_to_driver_map();
	
		file_to_driver_map_type::const_iterator iter = map.find(file_extension);
		if (iter != map.end())
		{
			return iter->second.at(CODE);
		}
		return QString();
	}


	OGRFieldType
	get_ogr_field_type_from_qvariant(
		QVariant &variant)
	{
		switch (variant.type())
		{
		case QVariant::Int:
			return OFTInteger;
			break;
		case QVariant::Double:
			return OFTReal;
			break;
		case QVariant::String:
			return OFTString;
			break;
		default:
			return OFTString;
		}		
	}

	/**
	 * Sets the Ogr attribute field names and types from the key_value_dictionary elements.
	 */
	void
	set_layer_field_names(
		OGRLayer *ogr_layer,
		const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type &field_names_key_value_dictionary)
	{
		element_type elements = field_names_key_value_dictionary->elements();
		if (elements.empty())
		{
			qDebug() << "No elements in dictionary...";
			return;
		}

		if (ogr_layer != NULL)
		{
			element_iterator_type 
				iter = elements.begin(),
				end = elements.end();

			for ( ; iter != end ; ++iter)
			{
				// FIXME: Handle long field names....or prevent creation of long field names
				// at the appropriate point in the model-to-shapefile-attribute mapping process.
				// 
				// (Shapefile attribute field names are restricted to 10 characters in length. 
				// If the field name came from a shapefile, it'll already be of appropriate length.
				// But if the field name was generated by the user, it may not be...)

				QString key_string = GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());

				QVariant value_variant = GPlatesFileIO::OgrUtils::get_qvariant_from_kvd_element(*iter);
				QString type_string = GPlatesFileIO::OgrUtils::get_type_qstring_from_qvariant(value_variant);

				//qDebug() << "Field name: " << key_string << ", type: " << type_string;

				OGRFieldType ogr_field_type = get_ogr_field_type_from_qvariant(value_variant);

				OGRFieldDefn ogr_field(key_string.toStdString().c_str(),ogr_field_type);

				if (ogr_layer->CreateField(&ogr_field) != OGRERR_NONE)
				{
					qDebug() << "Error creating ogr field. Name: " << key_string << ", type: " << type_string;
					throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating ogr field.");
				}

			}
		}
	}

	/**
	 * Set the Ogr attribute field values from the key_value_dictionary. 
	 */
	void
	set_feature_field_values(
		OGRLayer *ogr_layer,
		OGRFeature *ogr_feature,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type field_values_key_value_dictionary)
	{
		if ((ogr_layer != NULL) && (ogr_feature != NULL))
		{
			// The number of fields created in 'set_layer_field_names()'.
			const int num_attributes_in_layer = ogr_layer->GetLayerDefn()->GetFieldCount();			

			for (int field = 0; field < num_attributes_in_layer; ++field)
			{
				const QString field_name = QString::fromStdString(
						ogr_layer->GetLayerDefn()->GetFieldDefn(field)->GetNameRef());

				// Search the kvd for the attribute with same name as current field name.
				element_iterator_type 
						iter = field_values_key_value_dictionary->elements().begin(),
						end = field_values_key_value_dictionary->elements().end();
				for ( ; iter != end; ++iter)
				{
					const QString attribute_name = GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());

					if (QString::compare(attribute_name, field_name) == 0)
					{
						break;
					}
				}

				// Skip the current feature (and mark it's cell as unset or null) if it does not have
				// an attribute for the current field name.
				if (iter == end)
				{
#if GPLATES_GDAL_VERSION_NUM >= GPLATES_GDAL_COMPUTE_VERSION(2,2,0)
					ogr_feature->SetFieldNull(field);
#else
					ogr_feature->UnsetField(field);
#endif

					continue;
				}

				// FIXME: do I really need to put this in variant form first?
				QVariant value_variant = GPlatesFileIO::OgrUtils::get_qvariant_from_kvd_element(*iter);

				OGRFieldType layer_type = ogr_layer->GetLayerDefn()->GetFieldDefn(field)->GetType();	
				OGRFieldType model_type  = get_ogr_field_type_from_qvariant(value_variant);

				if (layer_type != model_type)
				{
					// This shouldn't really happen.
					qDebug() << "OGR Writer: Mismatch in field types.";
					qDebug() << "Layer type: " << layer_type;
					qDebug() << "Model type: " << model_type;
				}

				bool ok = true;
				switch(layer_type)
				{
				case OFTInteger:
					{			
						int value = value_variant.toInt(&ok);
						if (ok)
						{
							ogr_feature->SetField(field,value);					
						}
					}
					break;
				case OFTReal:
					{
						double value = value_variant.toDouble(&ok);
						if (ok)
						{
							ogr_feature->SetField(field,value);					
						}
					}
					break;
				case OFTString:
				default:
					ogr_feature->SetField(field,value_variant.toString().toStdString().c_str());
					break;
				}

				if (!ok)
				{
					// Mark the current feature's cell as unset or null.
#if GPLATES_GDAL_VERSION_NUM >= GPLATES_GDAL_COMPUTE_VERSION(2,2,0)
					ogr_feature->SetFieldNull(field);
#else
					ogr_feature->UnsetField(field);
#endif

					qWarning() << "The QVariant containing the property value could not be converted to a type.";
				}
			}
		}
	}

	/**
	 * Creates an OGRLayer of type wkb_type and adds it to the GdalUtils::vector_data_source_type.
	 * Adds any attribute field names provided in key_value_dictionary. 
	 */
	void
	setup_layer(
		GPlatesFileIO::GdalUtils::vector_data_source_type *&ogr_data_source_ptr,
		boost::optional<OGRLayer*>& ogr_layer,
		OGRwkbGeometryType wkb_type,
		const QString &layer_name,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
		const boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> &original_srs,
		const GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::OgrSrsWriteBehaviour &ogr_srs_behaviour)
	{
		if (!ogr_data_source_ptr)
		{
			return;
		}
		if (!ogr_layer)
		{
			OGRSpatialReference spatial_reference; 

			if ((ogr_srs_behaviour == GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::WRITE_AS_ORIGINAL_SRS_BEHAVIOUR)  &&
				original_srs)
			{
				spatial_reference = original_srs.get()->get_ogr_srs();
			}
			else
			{
				spatial_reference.SetWellKnownGeogCS("WGS84");
#if GPLATES_GDAL_VERSION_NUM >= GPLATES_GDAL_COMPUTE_VERSION(3,0,0)
				// GDAL >= 3.0 introduced a data-axis-to-CRS-axis mapping (that breaks backward compatibility).
				// We need to set it to behave the same as before GDAL 3.0 (ie, longitude first, latitude second).
				spatial_reference.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
			}

			// We can't just pass the address of 'spatial_reference' into 'GDALDataset::CreateLayer()'
			// because some drivers (such as GeoPackage) increment the reference count instead of clone,
			// but 'spatial_reference' is on the stack and hence its destructor ignores the incremented
			// reference count and just destroys the OGRSpatialReference object leaving the 'GDALDataset'
			// with a dangling reference (which can cause a crash when it attempts to decrement the
			// reference count and delete the stack object).
			//
			// So instead we pass a clone into 'GDALDataset::CreateLayer()' and subsequently
			// 'Release()' the clone (this just decrements its reference count which shouldn't do
			// anything if the reference count was incremented by 'GDALDataset::CreateLayer()', but
			// will destroy object if 'GDALDataset::CreateLayer()' cloned our clone).
			OGRSpatialReference *cloned_spatial_reference = spatial_reference.Clone();

			ogr_layer.reset(ogr_data_source_ptr->CreateLayer(
				// FIXME: Layer name should probably be UTF-8 (ie, "layer_name.toUtf8().constData()")
				// instead of Latin-1 since the latter does not support all character sets.
				// Although it probably doesn't matter currently because the layer name is not
				// really used anyway (it only needs to be unique with the data source)...
				layer_name.toStdString().c_str(),
				cloned_spatial_reference,
				wkb_type,
				0));

			cloned_spatial_reference->Release();

			if (*ogr_layer == NULL)
			{
				// Set to none to avoid NULL pointer dereference if try to write another feature
				// (after catching exception for current feature).
				ogr_layer = boost::none;
				throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR layer.");
			}
			if (field_names_key_value_dictionary && !((*field_names_key_value_dictionary)->is_empty()))
			{
				set_layer_field_names(*ogr_layer, field_names_key_value_dictionary.get());
			}
		}
	}
		
	/**
	 * Create an ogr data source.
	 */
	void
	create_data_source(
		GPlatesFileIO::GdalUtils::vector_data_driver_type *&ogr_driver,
		GPlatesFileIO::GdalUtils::vector_data_source_type *&data_source_ptr,
		QString &data_source_name)
	{
		data_source_name = QDir::toNativeSeparators(data_source_name);
		data_source_ptr = GPlatesFileIO::GdalUtils::create_data_source(ogr_driver, data_source_name, NULL);

		if (data_source_ptr == NULL)
		{
			throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Ogr data source creation failed.");
		}
	}

	void
	destroy_ogr_data_source(
			GPlatesFileIO::GdalUtils::vector_data_source_type *&ogr_data_source)
	{
		if (ogr_data_source)
		{
			try
			{
				GPlatesFileIO::GdalUtils::close_vector(ogr_data_source);
			}
			catch (...)
			{ }
		}
	}

	void
	remove_OGR_layers(
		GPlatesFileIO::GdalUtils::vector_data_driver_type *&ogr_driver,
		const QString &filename)
	{

		GPlatesFileIO::GdalUtils::vector_data_source_type *ogr_data_source_ptr =
				GPlatesFileIO::GdalUtils::open_vector(filename, true/*true to allow updates*/);

		if (ogr_data_source_ptr == NULL)
		{
			throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,
											  "OGR data source creation failed when trying to remove layers.");
		}
		
		int number_of_layers = ogr_data_source_ptr->GetLayerCount();

		for (int count = 0; count < number_of_layers; ++count)
		{
		// After removing a layer, the layers are renumbered, so using index 0 in this loop removes all the layers.
			ogr_data_source_ptr->DeleteLayer(0);
		}

		destroy_ogr_data_source(ogr_data_source_ptr);
	}

	/**
	 * @brief remove_multiple_geometry_type_files
	 *
	 * Shapefiles can only have one geometry type (point, polyline) in the file, hence export of mixed geometry types must
	 * be to separate files. We currently export all these files to a subfolder. For example:
	 *
	 * if a collection contained points and lines, and the collection name and path was
	 * "path-name/collection-name", and .shp export has been requested, then the layers would be
	 * exported to "path-name/collection-name/collection-name_point.shp" and
	 * "path-name/collection-name/collection-name_polyline.shp".
	 *
	 * This function deletes (or removes the layers from) these files.
	 *
	 * @param driver - the OGR driver
	 * @param folder_name - the subfolder name (e.g. path-name/collection-name/ from the example above)
	 * @param basename - the basename (e.g. collection-name from the example above)
	 * @param extension - the extension indicating the file type (e.g. .shp)
	 */
	void
	remove_multiple_geometry_type_files(
			GPlatesFileIO::GdalUtils::vector_data_driver_type *driver,
			const QString &folder_name,
			const QString &basename,
			const QString &extension)
	{

		QString point_name = basename + POINT_SUFFIX + "." + extension;
		QString polygon_name = basename + POLYGON_SUFFIX + "." + extension;
		QString polyline_name = basename + POLYLINE_SUFFIX + "." + extension;

		QStringList filenames;
		filenames << point_name << polygon_name << polyline_name;
		QDir folder(folder_name);
		if (!folder.exists())
		{
			return;
		}


		Q_FOREACH(const QString &filename, filenames)
		{
			if (folder.exists(filename)){
				QString full_name = folder.absoluteFilePath(filename);
				if (file_type_does_not_support_layer_deletion(extension))
				{
					QFile::remove(filename);
				}
				else
				{
					remove_OGR_layers(driver,full_name);
				}
			}
		}
	}


	//! Typedef for a sequence of lat/lon points.
	typedef std::vector<GPlatesMaths::LatLonPoint> lat_lon_points_seq_type;

	/**
	 * A polyline containing a single sequence of points.
	 */
	struct LatLonPolyline
	{
		lat_lon_points_seq_type line;
	};

	/**
	 * A polygon containing an exterior ring and optional interior rings.
	 *
	 * NOTE: This mirrors @a PolygonOnSphere in that the start and end points of each ring are *not* the same.
	 * So you may need to explicitly close each polygon ring by appending the start point (eg, OGR library).
	 */
	struct LatLonPolygon
	{
		lat_lon_points_seq_type exterior_ring;
		std::vector<lat_lon_points_seq_type> interior_rings;
	};


	/**
	 * Converts a sequence of PointOnSphere to LatLonPoint.
	 */
	template <typename PointsForwardIter>
	void
	convert_points_to_lat_lon(
			lat_lon_points_seq_type &lat_lon_points,
			PointsForwardIter const points_begin,
			PointsForwardIter const points_end,
			const unsigned int num_points)
	{
		lat_lon_points.reserve(num_points);

		// Iterate over the vertices of the current polyline or polygon.
		PointsForwardIter line_iter = points_begin;
		PointsForwardIter line_end = points_end;
		for (; line_iter != line_end ; ++line_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *line_iter;
			const GPlatesMaths::LatLonPoint lat_lon_point = GPlatesMaths::make_lat_lon_point(point);
			lat_lon_points.push_back(lat_lon_point);
		}
	}

	/**
	 * Converts the specified PolylineOnSphere to LatLonPolyline.
	 */
	void
	convert_polyline_to_lat_lon(
			LatLonPolyline &lat_lon_polyline,
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline)
	{
		convert_points_to_lat_lon(
				lat_lon_polyline.line,
				polyline->vertex_begin(),
				polyline->vertex_end(),
				polyline->number_of_vertices());
	}

	/**
	 * Converts the specified PolygonOnSphere to LatLonPolygon.
	 */
	void
	convert_polygon_to_lat_lon(
			LatLonPolygon &lat_lon_polygon,
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon)
	{
		convert_points_to_lat_lon(
				lat_lon_polygon.exterior_ring,
				polygon->exterior_ring_vertex_begin(),
				polygon->exterior_ring_vertex_end(),
				polygon->number_of_vertices_in_exterior_ring());

		const unsigned int num_interior_rings = polygon->number_of_interior_rings();
		lat_lon_polygon.interior_rings.resize(num_interior_rings);
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			convert_points_to_lat_lon(
					lat_lon_polygon.interior_rings[interior_ring_index],
					polygon->interior_ring_vertex_begin(interior_ring_index),
					polygon->interior_ring_vertex_end(interior_ring_index),
					polygon->number_of_vertices_in_interior_ring(interior_ring_index));
		}
	}


	/**
	 * Converts the specified polyline-on-sphere geometries to lat/lon geometries with optional dateline wrapping.
	 */
	void
	convert_polylines_to_lat_lon(
			std::vector<LatLonPolyline> &lat_lon_polylines,
			const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines,
			boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper = boost::none)
	{
		typedef std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_seq_type;

		if (dateline_wrapper)
		{
			std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline> wrapped_lat_lon_polylines;

			// Iterate over the polyline-on-sphere's.
			polyline_seq_type::const_iterator polylines_iter = polylines.begin();
			polyline_seq_type::const_iterator polylines_end = polylines.end();
			for ( ; polylines_iter != polylines_end; ++polylines_iter)
			{
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline = *polylines_iter;

				// Wrap (clip) the current polyline to the dateline.
				// This could turn one polyline into multiple polylines.
				dateline_wrapper->wrap_polyline(polyline, wrapped_lat_lon_polylines);
			}

			// Copy the wrapped polylines to the caller's array.
			lat_lon_polylines.reserve(wrapped_lat_lon_polylines.size());
			BOOST_FOREACH(
					const GPlatesMaths::DateLineWrapper::LatLonPolyline &wrapped_lat_lon_polyline,
					wrapped_lat_lon_polylines)
			{
				lat_lon_polylines.push_back(LatLonPolyline());
				LatLonPolyline &lat_lon_polyline = lat_lon_polylines.back();

				lat_lon_polyline.line = wrapped_lat_lon_polyline.get_points();
			}
		}
		else // no dateline wrapping...
		{
			lat_lon_polylines.reserve(polylines.size());

			// Iterate over the polyline-on-sphere's.
			polyline_seq_type::const_iterator polylines_iter = polylines.begin();
			polyline_seq_type::const_iterator polylines_end = polylines.end();
			for ( ; polylines_iter != polylines_end; ++polylines_iter)
			{
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline = *polylines_iter;

				// No dateline wrapping so just straight conversion to lat/lon.
				lat_lon_polylines.push_back(LatLonPolyline());
				convert_polyline_to_lat_lon(lat_lon_polylines.back(), polyline);
			}
		}
	}

	/**
	 * Converts the specified polygon-on-sphere geometries to lat/lon geometries with optional dateline wrapping.
	 */
	void
	convert_polygons_to_lat_lon(
			std::vector<LatLonPolygon> &lat_lon_polygons,
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons,
			boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper = boost::none)
	{
		typedef std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_seq_type;

		if (dateline_wrapper)
		{
			std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon> wrapped_lat_lon_polygons;

			// Iterate over the polygon-on-sphere's.
			polygon_seq_type::const_iterator polygons_iter = polygons.begin();
			polygon_seq_type::const_iterator polygons_end = polygons.end();
			for ( ; polygons_iter != polygons_end; ++polygons_iter)
			{
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon = *polygons_iter;

				// Wrap (clip) the current polygon to the dateline.
				// This could turn one polygon into multiple polygons.
				dateline_wrapper->wrap_polygon(polygon, wrapped_lat_lon_polygons);
			}

			// Copy the wrapped polygons to the caller's array.
			lat_lon_polygons.reserve(wrapped_lat_lon_polygons.size());
			BOOST_FOREACH(
					const GPlatesMaths::DateLineWrapper::LatLonPolygon &wrapped_lat_lon_polygon,
					wrapped_lat_lon_polygons)
			{
				lat_lon_polygons.push_back(LatLonPolygon());
				LatLonPolygon &lat_lon_polygon = lat_lon_polygons.back();

				// Exterior ring.
				lat_lon_polygon.exterior_ring = wrapped_lat_lon_polygon.get_exterior_ring_points();

				// Interior rings.
				const unsigned int num_interior_rings = wrapped_lat_lon_polygon.get_num_interior_rings();
				lat_lon_polygon.interior_rings.resize(num_interior_rings);
				for (unsigned int interior_ring_index = 0;
					interior_ring_index < num_interior_rings;
					++interior_ring_index)
				{
					lat_lon_polygon.interior_rings[interior_ring_index] =
							wrapped_lat_lon_polygon.get_interior_ring_points(interior_ring_index);
				}
			}
		}
		else // no dateline wrapping...
		{
			lat_lon_polygons.reserve(polygons.size());

			// Iterate over the polygon-on-sphere's.
			polygon_seq_type::const_iterator polygons_iter = polygons.begin();
			polygon_seq_type::const_iterator polygons_end = polygons.end();
			for ( ; polygons_iter != polygons_end; ++polygons_iter)
			{
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon = *polygons_iter;

				// No dateline wrapping so just straight conversion to lat/lon.
				lat_lon_polygons.push_back(LatLonPolygon());
				convert_polygon_to_lat_lon(lat_lon_polygons.back(), polygon);
			}
		}
	}

	void
	add_polyline_to_ogr_line_string(
			OGRLineString &ogr_line_string,
			const LatLonPolyline &lat_lon_polyline,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		lat_lon_points_seq_type::const_iterator 
			line_iter = lat_lon_polyline.line.begin(),
			line_end = lat_lon_polyline.line.end();

		for (; line_iter != line_end ; ++line_iter)
		{
			OGRPoint ogr_point;

			const GPlatesMaths::LatLonPoint &llp = *line_iter;
			double x = llp.longitude();
			double y = llp.latitude();
			coordinate_transformation->transform_in_place(&x,&y);
			ogr_point.setX(x);
			ogr_point.setY(y);

			ogr_line_string.addPoint(&ogr_point);
		}
	}

	void
	add_multi_polyline_to_ogr_feature(
			OGRFeature &ogr_feature,
			const std::vector<LatLonPolyline> &lat_lon_polylines,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		OGRMultiLineString ogr_multi_line_string;

		std::vector<LatLonPolyline>::const_iterator 
			it = lat_lon_polylines.begin(),
			end = lat_lon_polylines.end();
			
		for (; it != end ; ++it)
		{
			const LatLonPolyline &lat_lon_polyline = *it;

			OGRLineString ogr_line_string;
			add_polyline_to_ogr_line_string(ogr_line_string, lat_lon_polyline, coordinate_transformation);

			ogr_multi_line_string.addGeometry(&ogr_line_string);
		}

		ogr_feature.SetGeometry(&ogr_multi_line_string);
	}

	void
	add_polyline_to_ogr_feature(
			OGRFeature &ogr_feature,
			const LatLonPolyline &lat_lon_polyline,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		OGRLineString ogr_line_string;
		add_polyline_to_ogr_line_string(ogr_line_string, lat_lon_polyline, coordinate_transformation);

		ogr_feature.SetGeometry(&ogr_line_string);
	}

	void
	add_polygon_ring_to_ogr_polygon(
			OGRPolygon &ogr_polygon,
			const lat_lon_points_seq_type &lat_lon_polygon_ring,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		OGRLinearRing ogr_linear_ring;

		lat_lon_points_seq_type::const_iterator 
			line_iter = lat_lon_polygon_ring.begin(),
			line_end = lat_lon_polygon_ring.end();

		for (; line_iter != line_end ; ++line_iter)
		{
			OGRPoint ogr_point;

			const GPlatesMaths::LatLonPoint &llp = *line_iter;
			double x = llp.longitude();
			double y = llp.latitude();
			coordinate_transformation->transform_in_place(&x,&y);
			ogr_point.setX(x);
			ogr_point.setY(y);

			ogr_linear_ring.addPoint(&ogr_point);
		}

		// Close the ring. GPlates stores polygons such that first-point != last-point; the 
		// ESRI shapefile specification says that polygon rings must be closed (first-point == last-point). 
		ogr_linear_ring.closeRings();

		// This will be the external ring if it's the first to be added (otherwise an interior ring)
		// since according to the OGR docs...
		//
		// "If the polygon has no external ring (it is empty) this will be used as the external ring,
		//  otherwise it is used as an internal ring."
		ogr_polygon.addRing(&ogr_linear_ring);
	}

	void
	add_polygon_to_ogr_polygon(
			OGRPolygon &ogr_polygon,
			const LatLonPolygon &lat_lon_polygon,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		// Add the exterior ring first since according to the OGR docs for OGRPolygon::addRing()...
		//
		// "If the polygon has no external ring (it is empty) this will be used as the external ring,
		//  otherwise it is used as an internal ring."
		add_polygon_ring_to_ogr_polygon(ogr_polygon, lat_lon_polygon.exterior_ring,coordinate_transformation);

		// Add the interior rings (if any).
		const unsigned int num_interior_rings = lat_lon_polygon.interior_rings.size();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			add_polygon_ring_to_ogr_polygon(
					ogr_polygon,
					lat_lon_polygon.interior_rings[interior_ring_index],
					coordinate_transformation);
		}
	}

	void
	add_multi_polygon_to_ogr_feature(
			OGRFeature &ogr_feature,
			const std::vector<LatLonPolygon> &lat_lon_polygons,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		OGRMultiPolygon ogr_multi_polygon;

		std::vector<LatLonPolygon>::const_iterator 
			it = lat_lon_polygons.begin(),
			end = lat_lon_polygons.end();
			
		for (; it != end ; ++it)
		{
			const LatLonPolygon &lat_lon_polygon = *it;

			OGRPolygon ogr_polygon;
			add_polygon_to_ogr_polygon(ogr_polygon, lat_lon_polygon,coordinate_transformation);

			ogr_multi_polygon.addGeometry(&ogr_polygon);
		}

		ogr_feature.SetGeometry(&ogr_multi_polygon);
	}

	void
	add_polygon_to_ogr_feature(
			OGRFeature &ogr_feature,
			const LatLonPolygon &lat_lon_polygon,
			const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation)
	{
		OGRPolygon ogr_polygon;
		add_polygon_to_ogr_polygon(ogr_polygon, lat_lon_polygon, coordinate_transformation);

		ogr_feature.SetGeometry(&ogr_polygon);
	}
}

GPlatesFileIO::OgrWriter::OgrWriter(
	QString filename,
	bool multiple_geometry_types,
	bool wrap_to_dateline,
	boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> original_srs,
	const GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::OgrSrsWriteBehaviour &behaviour):
	d_ogr_driver_ptr(0),
	d_filename(filename),
	d_layer_basename(QString()),
	d_extension(QString()),
	d_multiple_geometry_types(multiple_geometry_types),
	d_wrap_to_dateline(wrap_to_dateline),
	d_ogr_data_source_ptr(0),
	d_ogr_point_data_source_ptr(0),
	d_ogr_line_data_source_ptr(0),
	d_ogr_polygon_data_source_ptr(0),
	d_dateline_wrapper(GPlatesMaths::DateLineWrapper::create()),
	d_original_srs(original_srs),
	d_ogr_srs_write_behaviour(behaviour),
	d_coordinate_transformation(GPlatesPropertyValues::CoordinateTransformation::create())
{
	GdalUtils::register_all_drivers();

	QFileInfo q_file_info_original(d_filename);
	d_extension = q_file_info_original.suffix();
	d_extension = d_extension.toLower();

	QString driver_name = get_driver_name_from_file_extension(d_extension);

	d_ogr_driver_ptr = GdalUtils::get_vector_driver_manager()->GetDriverByName(driver_name.toStdString().c_str());
	if (d_ogr_driver_ptr == NULL)
	{
		throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"OGR driver not available.");
	}


	// Adjust the filename to include a sub-folder if necessary.
	// For multiple geometry types we need to export to separate layers, one for each geometry type.
	// Shapefiles can have only one layer, hence we need to export to separate files. Our current behaviour
	// is to export these files to a new folder. The folder name is taken from the collection name.
	// The individual files in the folder use the collection name with a suffix indicating which
	// geometry type is contained in the file. The suffix is appended later in the process, in
	// functions such as "write_point_feature" etc, where the data source is created.
	//
	// For example if a collection contained points and lines, and the collection name and path was
	// "path-name/collection-name", and .shp export has been requested, then the layers would be
	// exported to "path-name/collection-name/collection-name_point.shp" and
	// "path-name/collection-name/collection-name_polyline.shp".
	QString path = q_file_info_original.absolutePath();

	// If the base level path (directory) does not exist then we cannot open the file(s) for writing.
	// We do create a sub-directory in this path (if needed) when there are multiple geometry types,
	// but we expect the original path (dir) to exist (this mirrors other file writers).
	if (!QDir(path).exists())
	{
		throw ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE,
				q_file_info_original.filePath());
	}

	QString basename = q_file_info_original.completeBaseName();
	if (d_multiple_geometry_types)
	{
		QString folder_name = path + QDir::separator() + basename;
		QDir qdir(folder_name);
		
		if (!qdir.exists())
		{
			if (!QDir(path).mkdir(basename))
			{
				throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create directory for multiple geometry-type files.");
			}
		}
		d_filename = folder_name + QDir::separator() + basename;
	}
	else
	{
		d_filename = path + QDir::separator() + basename;
	}

	d_layer_basename = basename;

	QString full_filename;
	if (!d_multiple_geometry_types)
	{
		full_filename = d_filename + "." + d_extension;
		QFileInfo q_file_info_modified(full_filename);
		if (q_file_info_modified.exists())
		{
			if (file_type_does_not_support_layer_deletion(d_extension))
			{
				QFile::remove(full_filename);
			}
			else
			{
				remove_OGR_layers(d_ogr_driver_ptr,full_filename);
			}
		}
	}
	else
	{
		QString folder_name = path + QDir::separator() + basename;

		QDir dir(folder_name);
		if (dir.exists())
		{
			remove_multiple_geometry_type_files(d_ogr_driver_ptr,folder_name,basename,d_extension);
		}
	}

	// Set up the coordinate transform as required. This may end up being the identity transform.

	if (d_original_srs &&
			d_ogr_srs_write_behaviour == FeatureCollectionFileFormat::OGRConfiguration::WRITE_AS_ORIGINAL_SRS_BEHAVIOUR)
	{
		boost::optional<GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type> transform =
				GPlatesPropertyValues::CoordinateTransformation::create(
					GPlatesPropertyValues::SpatialReferenceSystem::get_WGS84(),
					*d_original_srs);

		if (transform)
		{
			d_coordinate_transformation = transform.get();
		}
	}

}

GPlatesFileIO::OgrWriter::~OgrWriter()
{
	destroy_ogr_data_source(d_ogr_data_source_ptr);
	destroy_ogr_data_source(d_ogr_point_data_source_ptr);
	destroy_ogr_data_source(d_ogr_line_data_source_ptr);
	destroy_ogr_data_source(d_ogr_polygon_data_source_ptr);

}

void
GPlatesFileIO::OgrWriter::write_point_feature(
	const GPlatesMaths::PointOnSphere &point_on_sphere,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{
	// Create point data source if it doesn't already exist.
	if (d_ogr_point_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append(POINT_SUFFIX);
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_point_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names, and set the desired SRS.
	setup_layer(d_ogr_point_data_source_ptr,
				d_ogr_point_layer,
				wkbPoint,
				QString(d_layer_basename + "_point"),
				field_names_key_value_dictionary,
				d_original_srs,
				d_ogr_srs_write_behaviour);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (field_values_key_value_dictionary && !((*field_values_key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_point_layer, ogr_feature, field_values_key_value_dictionary.get());
	}

	// Create the point feature from the point_on_sphere
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point_on_sphere);

	double x = llp.longitude();
	double y = llp.latitude();
	d_coordinate_transformation->transform_in_place(&x,&y);
	OGRPoint ogr_point;
	ogr_point.setX(x);
	ogr_point.setY(y);

	ogr_feature->SetGeometry(&ogr_point);

	// Add the new feature to the layer.
	if ((*d_ogr_point_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create point feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_multi_point_feature(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{
#if 0
	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_multi_point_feature";
		return;
	}
#endif
	// Create point data source if it doesn't already exist.
	if (d_ogr_point_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append(POINT_SUFFIX);
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_point_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_point_data_source_ptr,
				d_ogr_multi_point_layer,
				wkbMultiPoint,
				QString(d_layer_basename + "_multi_point"),
				field_names_key_value_dictionary,
				d_original_srs,
				d_ogr_srs_write_behaviour);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_multi_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (field_values_key_value_dictionary && !((*field_values_key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_multi_point_layer, ogr_feature, field_values_key_value_dictionary.get());
	}

	OGRMultiPoint ogr_multi_point;

	GPlatesMaths::MultiPointOnSphere::const_iterator 
		iter = multi_point_on_sphere->begin(),
		end = multi_point_on_sphere->end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		double x = llp.longitude();
		double y = llp.latitude();
		d_coordinate_transformation->transform_in_place(&x,&y);
		OGRPoint ogr_point;
		ogr_point.setX(x);
		ogr_point.setY(y);
		ogr_multi_point.addGeometry(&ogr_point);
	}

	ogr_feature->SetGeometry(&ogr_multi_point);


	// Add the new feature to the layer.
	if ((*d_ogr_multi_point_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create multi-point feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_polyline_feature(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{	
	// It's one polyline but if dateline wrapping is enabled it could end up being multiple polylines.
	std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polylines(1, polyline_on_sphere);
	write_single_or_multi_polyline_feature(polylines, field_names_key_value_dictionary, field_values_key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_multi_polyline_feature(
	const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{
	write_single_or_multi_polyline_feature(polylines, field_names_key_value_dictionary, field_values_key_value_dictionary);
}

void
GPlatesFileIO::OgrWriter::write_single_or_multi_polyline_feature(
	const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{
#if 0
	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_polyline_feature";
		return;
	}
#endif

	if (polylines.empty())
	{
		return;
	}

	// Convert the polylines to lat/lon coordinates (with optional dateline wrapping/clipping).
	std::vector<LatLonPolyline> lat_lon_polylines;
	boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper;
	if (d_wrap_to_dateline)
	{
		dateline_wrapper = *d_dateline_wrapper;
	}
	convert_polylines_to_lat_lon(lat_lon_polylines, polylines, dateline_wrapper);

	// Multiple polylines or a single polyline...
	//
	// Shapefiles support mixing single/multi line strings per layer but other formats,
	// like GMT and GeoPackage, do not (specifying single line string as layer geom type will
	// result in OGR reader loading only the first line string per feature). Also we don't yet
	// know what the next line string type (single/multi) will be since dateline wrapping can
	// turn a single into a multi. So we just treat them all as multi line strings.
	//
	// FIXME: There's probably a better solution that this such as determining if any
	// multi line strings up front.
	const bool is_multi_line_string = (lat_lon_polylines.size() > 1) ||
			file_type_does_not_support_mixing_single_and_multi_line_strings_in_layer(d_extension);

	// Create line data source if it doesn't already exist.
	if (d_ogr_line_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append(POLYLINE_SUFFIX);
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_line_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(
			d_ogr_line_data_source_ptr,
			d_ogr_polyline_layer,
			(is_multi_line_string ? wkbMultiLineString : wkbLineString),
			QString(d_layer_basename + "_polyline"),
			field_names_key_value_dictionary,
			d_original_srs,
			d_ogr_srs_write_behaviour);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polyline_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (field_values_key_value_dictionary && !((*field_values_key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polyline_layer, ogr_feature, field_values_key_value_dictionary.get());
	}

	if (is_multi_line_string)
	{
		add_multi_polyline_to_ogr_feature(*ogr_feature, lat_lon_polylines, d_coordinate_transformation);
	}
	else
	{
		add_polyline_to_ogr_feature(*ogr_feature, lat_lon_polylines.front(), d_coordinate_transformation);
	}


	// Add the new feature to the layer.
	if ((*d_ogr_polyline_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create multi polyline feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_polygon_feature(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{	
	// It's one polygon but if dateline wrapping is enabled it could end up being multiple polygons.
	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygons(1, polygon_on_sphere);
	write_single_or_multi_polygon_feature(polygons, field_names_key_value_dictionary, field_values_key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_multi_polygon_feature(
	const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{
	write_single_or_multi_polygon_feature(polygons, field_names_key_value_dictionary, field_values_key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_single_or_multi_polygon_feature(
	const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_names_key_value_dictionary,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &field_values_key_value_dictionary)
{	
	if (polygons.empty())
	{
		return;
	}

	// Convert the polygons to lat/lon coordinates (with optional dateline wrapping/clipping).
	std::vector<LatLonPolygon> lat_lon_polygons;
	boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper;
	if (d_wrap_to_dateline)
	{
		dateline_wrapper = *d_dateline_wrapper;
	}
	convert_polygons_to_lat_lon(lat_lon_polygons, polygons, dateline_wrapper);

	// Multiple polygons or a single polygon...
	//
	// Shapefiles support mixing single/multi polygons per layer but other formats,
	// like GMT and GeoPackage, do not (specifying single polygon as layer geom type will
	// result in OGR reader loading only the first polygon per feature). Also we don't yet
	// know what the next polygon type (single/multi) will be since dateline wrapping can
	// turn a single into a multi. So we just treat them all as multi polygons.
	//
	// FIXME: There's probably a better solution that this such as determining if any
	// multi polygons up front.
	const bool is_multi_polygon = (lat_lon_polygons.size() > 1) ||
			file_type_does_not_support_mixing_single_and_multi_polygons_in_layer(d_extension);

	// Create polygon data source if it doesn't already exist.
	if (d_ogr_polygon_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append(POLYGON_SUFFIX);
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_polygon_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(
			d_ogr_polygon_data_source_ptr,
			d_ogr_polygon_layer,
			(is_multi_polygon ? wkbMultiPolygon : wkbPolygon),
			QString(d_layer_basename + "_polygon"),
			field_names_key_value_dictionary,
			d_original_srs,
			d_ogr_srs_write_behaviour);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polygon_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (field_values_key_value_dictionary && !((*field_values_key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polygon_layer, ogr_feature, field_values_key_value_dictionary.get());
	}

	if (is_multi_polygon)
	{
		add_multi_polygon_to_ogr_feature(*ogr_feature, lat_lon_polygons, d_coordinate_transformation);
	}
	else
	{
		add_polygon_to_ogr_feature(*ogr_feature, lat_lon_polygons.front(), d_coordinate_transformation);
	}

	// Add the new feature to the layer.
	if ((*d_ogr_polygon_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create polygon feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

