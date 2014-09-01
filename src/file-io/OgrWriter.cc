/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2011, 2012 Geological Survey of Norway
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


#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>


#include "OgrException.h"
#include "OgrWriter.h"
#include "OgrUtils.h"
#include "feature-visitors/ToQvariantConverter.h"
#include "maths/LatLonPoint.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"

namespace{

	typedef std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> element_type;
	typedef element_type::const_iterator element_iterator_type;
	typedef std::map<QString, QStringList> file_to_driver_map_type;

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
		gmt_driver << "GMT" << "GMT";
		map["gmt"] = gmt_driver;

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
			return iter->second.at(FORMAT_NAME);
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
		const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type &key_value_dictionary)
	{
		element_type elements = key_value_dictionary->elements();
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
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type key_value_dictionary)
	{
		if ((ogr_layer != NULL) && (ogr_feature != NULL))
		{
			int num_attributes_in_layer = ogr_layer->GetLayerDefn()->GetFieldCount();			
			int num_attributes_in_dictionary = key_value_dictionary->num_elements();

			if (num_attributes_in_layer != num_attributes_in_dictionary)
			{
				// This shouldn't really happen.
				qDebug() << "OGR Writer: Mismatch in number of fields.";
				qDebug() << "Layer has " << num_attributes_in_layer << " fields, kvd has " <<
							num_attributes_in_dictionary << " fields";
			}

			element_iterator_type 
				iter = key_value_dictionary->elements().begin(),
				end = key_value_dictionary->elements().end();

			for (int count = 0; (count < num_attributes_in_layer) && (iter != end) ; count++, ++iter)
			{
				QString model_string = GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());
				QString layer_string = QString::fromStdString(
					ogr_layer->GetLayerDefn()->GetFieldDefn(count)->GetNameRef());

				if (QString::compare(model_string,layer_string) != 0)
				{
					// This shouldn't really happen.
					qDebug() << "Mismatch in field names: model: " << model_string << ", layer : " << layer_string;
				}

				// FIXME: do I really need to put this in variant form first?
				QVariant value_variant = GPlatesFileIO::OgrUtils::get_qvariant_from_kvd_element(*iter);

				OGRFieldType layer_type = ogr_layer->GetLayerDefn()->GetFieldDefn(count)->GetType();	
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
							ogr_feature->SetField(count,value);					
						}
					}
					break;
				case OFTReal:
					{
						double value = value_variant.toDouble(&ok);
						if (ok)
						{
							ogr_feature->SetField(count,value);					
						}
					}
					break;
				case OFTString:
				default:
					ogr_feature->SetField(count,value_variant.toString().toStdString().c_str());
					break;
				}
				if (!ok)
				{
					qWarning() << "The QVariant containing the property value could not be converted to a type.";
				}

			}

		}
	}

	/**
	 * Creates an OGRLayer of type wkb_type and adds it to the OGRDataSource.
	 * Adds any attribute field names provided in key_value_dictionary. 
	 */
	void
	setup_layer(
		OGRDataSource *&ogr_data_source_ptr,
		boost::optional<OGRLayer*>& ogr_layer,
		OGRwkbGeometryType wkb_type,
		const QString &layer_name,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		if (!ogr_data_source_ptr)
		{
			return;
		}
		if (!ogr_layer)
		{
			OGRSpatialReference spatial_reference; 
			spatial_reference.SetWellKnownGeogCS("WGS84");

			ogr_layer.reset(ogr_data_source_ptr->CreateLayer(
				layer_name.toStdString().c_str(),&spatial_reference,wkb_type,0));
			if (*ogr_layer == NULL)
			{
				throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR layer.");
			}
			if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
			{
				set_layer_field_names(*ogr_layer,*key_value_dictionary);
			}
		}
	}
		
	/**
	 * Create an ogr data source.
	 */
	void
	create_data_source(
		OGRSFDriver *&ogr_driver,
		OGRDataSource *&data_source_ptr,
		QString &data_source_name)
	{
		data_source_name = QDir::toNativeSeparators(data_source_name);
		data_source_ptr = ogr_driver->CreateDataSource(data_source_name.toStdString().c_str(), NULL );

		if (data_source_ptr == NULL)
		{
			throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Ogr data source creation failed.");
		}
	}

	void
	remove_OGR_layers(
		OGRSFDriver *&ogr_driver,
		const QString &filename)
	{

		OGRDataSource *ogr_data_source_ptr = ogr_driver->Open(filename.toStdString().c_str(),true /* true to allow updates */);

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

	}

	void
	destroy_ogr_data_source(
		OGRDataSource *&ogr_data_source)
	{
		if (ogr_data_source)
		{
			try{
				OGRDataSource::DestroyDataSource(ogr_data_source);
			}
			catch(...)
			{}
		}
	}


	//! Typedef for a sequence of lat/lon points.
	typedef std::vector<GPlatesMaths::LatLonPoint> lat_lon_points_seq_type;

	//! Typedef for a polyline containing lat/lon points.
	typedef boost::shared_ptr<lat_lon_points_seq_type> lat_lon_polyline_type;

	/**
	 * Typedef for a polygon containing lat/lon points.
	 *
	 * NOTE: This mirrors @a PolygonOnSphere in that the start and end points are *not* the same.
	 * So you may need to explicitly close the polygon by appending the start point (eg, OGR library).
	 */
	typedef boost::shared_ptr<lat_lon_points_seq_type> lat_lon_polygon_type;

	/**
	 * Converts the specified polyline-on-sphere to a lat/lon geometry.
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
	 * Converts the specified polyline-on-sphere geometries to lat/lon geometries with optional dateline wrapping.
	 */
	void
	convert_polylines_to_lat_lon(
			std::vector<lat_lon_polyline_type> &lat_lon_polylines,
			const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines,
			boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper = boost::none)
	{
		typedef std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_seq_type;

		lat_lon_polylines.reserve(polylines.size()); // Though might end up with more if clipped to dateline.

		// Iterate over the polyline-on-sphere's.
		polyline_seq_type::const_iterator polylines_iter = polylines.begin();
		polyline_seq_type::const_iterator polylines_end = polylines.end();
		for ( ; polylines_iter != polylines_end; ++polylines_iter)
		{
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline = *polylines_iter;

			if (dateline_wrapper)
			{
				// Wrap (clip) the current polyline to the dateline.
				// This could turn one polyline into multiple polylines.
				// Note that lat/lon polylines just get appended to 'lat_lon_polylines'.
				dateline_wrapper.get().wrap_to_dateline(polyline, lat_lon_polylines);
			}
			else
			{
				// No dateline wrapping so just straight conversion to lat/lon.
				const lat_lon_polyline_type lat_lon_polyline(new lat_lon_points_seq_type());
				convert_points_to_lat_lon(
						*lat_lon_polyline,
						polyline->vertex_begin(),
						polyline->vertex_end(),
						polyline->number_of_vertices());
				lat_lon_polylines.push_back(lat_lon_polyline);
			}
		}
	}

	/**
	 * Converts the specified polygon-on-sphere geometries to lat/lon geometries with optional dateline wrapping.
	 */
	void
	convert_polygons_to_lat_lon(
			std::vector<lat_lon_polygon_type> &lat_lon_polygons,
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons,
			boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper = boost::none)
	{
		typedef std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_seq_type;

		lat_lon_polygons.reserve(polygons.size()); // Though might end up with more if clipped to dateline.

		// Iterate over the polygon-on-sphere's.
		polygon_seq_type::const_iterator polygons_iter = polygons.begin();
		polygon_seq_type::const_iterator polygons_end = polygons.end();
		for ( ; polygons_iter != polygons_end; ++polygons_iter)
		{
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon = *polygons_iter;

			if (dateline_wrapper)
			{
				// Wrap (clip) the current polygon to the dateline.
				// This could turn one polygon into multiple polygons.
				// Note that lat/lon polygons just get appended to 'lat_lon_polygons'.
				dateline_wrapper.get().wrap_to_dateline(polygon, lat_lon_polygons);
			}
			else
			{
				// No dateline wrapping so just straight conversion to lat/lon.
				const lat_lon_polygon_type lat_lon_polygon(new lat_lon_points_seq_type());
				convert_points_to_lat_lon(
						*lat_lon_polygon,
						polygon->vertex_begin(),
						polygon->vertex_end(),
						polygon->number_of_vertices());
				lat_lon_polygons.push_back(lat_lon_polygon);
			}
		}
	}

	void
	add_polyline_to_ogr_line_string(
			OGRLineString &ogr_line_string,
			const lat_lon_polyline_type &lat_lon_polyline)
	{
		lat_lon_points_seq_type::const_iterator 
			line_iter = lat_lon_polyline->begin(),
			line_end = lat_lon_polyline->end();

		for (; line_iter != line_end ; ++line_iter)
		{
			OGRPoint ogr_point;

			const GPlatesMaths::LatLonPoint &llp = *line_iter;
			ogr_point.setX(llp.longitude());
			ogr_point.setY(llp.latitude());

			ogr_line_string.addPoint(&ogr_point);
		}
	}

	void
	add_multi_polyline_to_ogr_feature(
			OGRFeature &ogr_feature,
			const std::vector<lat_lon_polyline_type> &lat_lon_polylines)
	{
		OGRMultiLineString ogr_multi_line_string;

		std::vector<lat_lon_polyline_type>::const_iterator 
			it = lat_lon_polylines.begin(),
			end = lat_lon_polylines.end();
			
		for (; it != end ; ++it)
		{
			const lat_lon_polyline_type &lat_lon_polyline = *it;

			OGRLineString ogr_line_string;
			add_polyline_to_ogr_line_string(ogr_line_string, lat_lon_polyline);

			ogr_multi_line_string.addGeometry(&ogr_line_string);
		}

		ogr_feature.SetGeometry(&ogr_multi_line_string);
	}

	void
	add_polyline_to_ogr_feature(
			OGRFeature &ogr_feature,
			const lat_lon_polyline_type &lat_lon_polyline)
	{
		OGRLineString ogr_line_string;
		add_polyline_to_ogr_line_string(ogr_line_string, lat_lon_polyline);

		ogr_feature.SetGeometry(&ogr_line_string);
	}

	void
	add_polygon_to_ogr_polygon(
			OGRPolygon &ogr_polygon,
			const lat_lon_polygon_type &lat_lon_polygon)
	{
		OGRLinearRing ogr_linear_ring;

		lat_lon_points_seq_type::const_iterator 
			line_iter = lat_lon_polygon->begin(),
			line_end = lat_lon_polygon->end();

		for (; line_iter != line_end ; ++line_iter)
		{
			OGRPoint ogr_point;

			const GPlatesMaths::LatLonPoint &llp = *line_iter;
			ogr_point.setX(llp.longitude());
			ogr_point.setY(llp.latitude());

			ogr_linear_ring.addPoint(&ogr_point);
		}

		// Close the ring. GPlates stores polygons such that first-point != last-point; the 
		// ESRI shapefile specification says that polygon rings must be closed (first-point == last-point). 
		ogr_linear_ring.closeRings();

		// This will be the external ring. 
		ogr_polygon.addRing(&ogr_linear_ring);
	}

	void
	add_multi_polygon_to_ogr_feature(
			OGRFeature &ogr_feature,
			const std::vector<lat_lon_polygon_type> &lat_lon_polygons)
	{
		OGRMultiPolygon ogr_multi_polygon;

		std::vector<lat_lon_polygon_type>::const_iterator 
			it = lat_lon_polygons.begin(),
			end = lat_lon_polygons.end();
			
		for (; it != end ; ++it)
		{
			const lat_lon_polygon_type &lat_lon_polygon = *it;

			OGRPolygon ogr_polygon;
			add_polygon_to_ogr_polygon(ogr_polygon, lat_lon_polygon);

			ogr_multi_polygon.addGeometry(&ogr_polygon);
		}

		ogr_feature.SetGeometry(&ogr_multi_polygon);
	}

	void
	add_polygon_to_ogr_feature(
			OGRFeature &ogr_feature,
			const lat_lon_polygon_type &lat_lon_polygon)
	{
		OGRPolygon ogr_polygon;
		add_polygon_to_ogr_polygon(ogr_polygon, lat_lon_polygon);

		ogr_feature.SetGeometry(&ogr_polygon);
	}
}

GPlatesFileIO::OgrWriter::OgrWriter(
	QString filename,
	bool multiple_geometry_types,
	bool wrap_to_dateline):
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
	d_dateline_wrapper(GPlatesMaths::DateLineWrapper::create())
{
    OGRRegisterAll();

	QFileInfo q_file_info_original(d_filename);
	d_extension = q_file_info_original.suffix();
	d_extension = d_extension.toLower();

	QString driver_name = get_driver_name_from_file_extension(d_extension);

	d_ogr_driver_ptr = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driver_name.toStdString().c_str());
	if (d_ogr_driver_ptr == NULL)
	{
		throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"OGR driver not available.");
	}


	// Adjust the filename to include a sub-folder if necessary.
	QString path = q_file_info_original.absolutePath();

	QString basename = q_file_info_original.completeBaseName();
	if (d_multiple_geometry_types)
	{
		QString folder_name = path + QDir::separator() + basename;
		QDir qdir(folder_name);
		
		//qDebug() << "Path: " << path;
		//qDebug() << "Basename: " << basename;
		//qDebug() << "Folder name: " << folder_name;
		
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

	// FIXME: Consider saving the file to a temporary name, and rename it once export is complete. 


	QString full_filename = d_filename + "." + d_extension;
	QFileInfo q_file_info_modified(full_filename);
	if (q_file_info_modified.exists())
	{
		// Note: GMT driver does not support DeleteLayer.....
		remove_OGR_layers(d_ogr_driver_ptr,full_filename);
	}


#if 0
	// Experiment with delaying data source creation until we need to create each type of layer. 
	d_ogr_data_source_ptr = ogr_driver->CreateDataSource(d_filename.toStdString().c_str(), NULL );
#endif

#if 0
	if (!q_file_info.exists())
	{
		// Create new file / folder
		d_ogr_data_source_ptr = ogr_driver->CreateDataSource(d_filename.toStdString().c_str(), NULL );
	}
	else
	{
		// File already exists; check if the user wants to overwrite.

		d_ogr_data_source_ptr = ogr_driver->Open(d_filename.toStdString().c_str(),true /* true to allow updates */);
	}
#endif

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
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{

	// Create point data source if it doesn't already exist.
	if (d_ogr_point_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append("_point");
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_point_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_point_data_source_ptr,d_ogr_point_layer,wkbPoint,QString(d_layer_basename + "_point"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_point_layer,ogr_feature,*key_value_dictionary);
	}

	// Create the point feature from the point_on_sphere
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point_on_sphere);

	OGRPoint ogr_point;
	ogr_point.setX(llp.longitude());
	ogr_point.setY(llp.latitude());

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
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
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
			data_source_name.append("_point");
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_point_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_point_data_source_ptr,d_ogr_multi_point_layer,wkbMultiPoint,
		QString(d_layer_basename + "_multi_point"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_multi_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_multi_point_layer,ogr_feature,*key_value_dictionary);
	}

	OGRMultiPoint ogr_multi_point;

	GPlatesMaths::MultiPointOnSphere::const_iterator 
		iter = multi_point_on_sphere->begin(),
		end = multi_point_on_sphere->end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		OGRPoint ogr_point;
		ogr_point.setX(llp.longitude());
		ogr_point.setY(llp.latitude());
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
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{	
	// It's one polyline but if dateline wrapping is enabled it could end up being multiple polylines.
	std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polylines(1, polyline_on_sphere);
	write_single_or_multi_polyline_feature(polylines, key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_multi_polyline_feature(
	const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{
	write_single_or_multi_polyline_feature(polylines, key_value_dictionary);
}

void
GPlatesFileIO::OgrWriter::write_single_or_multi_polyline_feature(
	const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
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
	std::vector<lat_lon_polyline_type> lat_lon_polylines;
	boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper;
	if (d_wrap_to_dateline)
	{
		dateline_wrapper = *d_dateline_wrapper;
	}
	convert_polylines_to_lat_lon(lat_lon_polylines, polylines, dateline_wrapper);

	const bool is_multi_line_string = lat_lon_polylines.size() > 1;

	// Create line data source if it doesn't already exist.
	if (d_ogr_line_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append("_polyline");
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_line_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(
			d_ogr_line_data_source_ptr,
			d_ogr_polyline_layer,
			// Multiple polylines or a single polyline...
			is_multi_line_string ? wkbMultiLineString : wkbLineString,
			QString(d_layer_basename + "_polyline"),
			key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polyline_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polyline_layer,ogr_feature,*key_value_dictionary);
	}

	if (is_multi_line_string)
	{
		add_multi_polyline_to_ogr_feature(*ogr_feature, lat_lon_polylines);
	}
	else
	{
		add_polyline_to_ogr_feature(*ogr_feature, lat_lon_polylines.front());
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
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{	
	// It's one polygon but if dateline wrapping is enabled it could end up being multiple polygons.
	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygons(1, polygon_on_sphere);
	write_single_or_multi_polygon_feature(polygons, key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_multi_polygon_feature(
	const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{
	write_single_or_multi_polygon_feature(polygons, key_value_dictionary);
}


void
GPlatesFileIO::OgrWriter::write_single_or_multi_polygon_feature(
	const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{	
	if (polygons.empty())
	{
		return;
	}

	// Convert the polygons to lat/lon coordinates (with optional dateline wrapping/clipping).
	std::vector<lat_lon_polygon_type> lat_lon_polygons;
	boost::optional<GPlatesMaths::DateLineWrapper &> dateline_wrapper;
	if (d_wrap_to_dateline)
	{
		dateline_wrapper = *d_dateline_wrapper;
	}
	convert_polygons_to_lat_lon(lat_lon_polygons, polygons, dateline_wrapper);

	const bool is_multi_polygon = lat_lon_polygons.size() > 1;

	// Create polygon data source if it doesn't already exist.
	if (d_ogr_polygon_data_source_ptr == NULL)
	{
		QString data_source_name = d_filename;
		if (d_multiple_geometry_types)
		{
			data_source_name.append("_polygon");
		}
		data_source_name.append(".").append(d_extension);

		create_data_source(d_ogr_driver_ptr, d_ogr_polygon_data_source_ptr, data_source_name);
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(
			d_ogr_polygon_data_source_ptr,
			d_ogr_polygon_layer,
			// Multiple polygons or a single polygon...
			is_multi_polygon ? wkbMultiPolygon : wkbPolygon,
			QString(d_layer_basename + "_polygon"),
			key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polygon_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating OGR feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polygon_layer,ogr_feature,*key_value_dictionary);
	}

	if (is_multi_polygon)
	{
		add_multi_polygon_to_ogr_feature(*ogr_feature, lat_lon_polygons);
	}
	else
	{
		add_polygon_to_ogr_feature(*ogr_feature, lat_lon_polygons.front());
	}

	// Add the new feature to the layer.
	if ((*d_ogr_polygon_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create polygon feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

