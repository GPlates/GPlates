/* $Id: GMTFormatFlowlinesExporter.cc 8290 2010-05-05 20:31:21Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-05-05 22:31:21 +0200 (on, 05 mai 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway
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
#include <QStringList>
#include <QTextStream>

#include "app-logic/MotionPathUtils.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/GMTFormatHeader.h"
#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "model/FeatureVisitor.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsString.h"
#include "utils/StringFormattingUtils.h"
#include "GMTFormatMotionPathExport.h"

namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::GMTFormatMotionPathsExport::referenced_files_collection_type
		referenced_files_collection_type;

	//! Convenience typedef for reconstructed geometries.
	typedef GPlatesFileIO::ReconstructedMotionPathExportImpl::reconstructed_motion_path_seq_type
		reconstructed_motion_path_seq_type;

	/**
	* Adapted from GMTFormatGeometryExporter
	*/
	void
	print_gmt_coordinate_line(
		QTextStream &stream,
		const GPlatesMaths::Real &lat,
		const GPlatesMaths::Real &lon,
		const double &time,
		bool reverse_coordinate_order)
	{
		/*
		* A coordinate in the GMT xy format is written as decimal number that
		* takes up 8 characters excluding sign.
		*/
		static const unsigned GMT_COORDINATE_FIELDWIDTH = 9;

		/*
		* We convert the coordinates to strings first, so that in case an exception
		* is thrown, the ostream is not modified.
		*/
		std::string lat_str, lon_str, time_str;
		try {
			lat_str = GPlatesUtils::formatted_double_to_string(lat.dval(),
				GMT_COORDINATE_FIELDWIDTH);
			lon_str = GPlatesUtils::formatted_double_to_string(lon.dval(),
				GMT_COORDINATE_FIELDWIDTH);
			time_str = GPlatesUtils::formatted_double_to_string(time,
				GMT_COORDINATE_FIELDWIDTH);
		} catch (const GPlatesUtils::InvalidFormattingParametersException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.
			throw;
		}

		// GMT format is by default (lon,lat) which is opposite of PLATES4 line format.
		if (reverse_coordinate_order) {
			// For whatever perverse reason, the user wants to write in (lat,lon) order.
			stream << "  " << lat_str.c_str()
				<< "      " << lon_str.c_str()
				<< "      " << time_str.c_str() << endl;
		} else {
			// Normal GMT (lon,lat) order should be used.
			stream << "  " << lon_str.c_str()
				<< "      " << lat_str.c_str() 
				<< "      " << time_str.c_str() << endl;
		}
	}


	void
	write_seed_point_to_stream(
		QTextStream &text_stream,
		const GPlatesAppLogic::ReconstructedMotionPath &rf)
	{
		GPlatesAppLogic::ReconstructedMotionPath::seed_point_geom_ptr_type seed_point =
			rf.seed_point();

		GPlatesMaths::LatLonPoint llp = make_lat_lon_point(*seed_point);
		text_stream << "> ";
		text_stream << "Seed point: ";
		text_stream << "Lat: ";
		text_stream << llp.latitude();
		text_stream << ", Lon: ";
		text_stream << llp.longitude();
		text_stream << endl;
	}

	void
	write_motion_path_to_stream(
		QTextStream &text_stream,
		const GPlatesAppLogic::ReconstructedMotionPath &rmp,
		const std::vector<double> &times)
	{
		GPlatesAppLogic::ReconstructedMotionPath::motion_path_geom_ptr_type mt = rmp.motion_path_points();

		GPlatesMaths::PolylineOnSphere::vertex_const_iterator 
			line_it = mt->vertex_begin(),
			line_end = mt->vertex_end();

		// Reverse iterate here so that the printed output goes forward in time. This (forward in time) direction
		// is the same sense as the motion track points. 
		std::vector<double>::const_reverse_iterator 
			time_it = times.rbegin(),
			time_end = times.rend(); 


		text_stream << "> Motion path" << endl;
		for (; (line_it != line_end) && (time_it != time_end) ; ++line_it, ++time_it)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*line_it);
			double time = *time_it;
			print_gmt_coordinate_line(text_stream,llp.latitude(),llp.longitude(),time,false /* reverse_coordinate_ord */);
		}

	}

	void
	get_export_times(
		std::vector<double> &export_times,
		const std::vector<double> &times,
		const double &reconstruction_time)
	{
		std::vector<double>::const_iterator 
			it = times.begin(),
			end = times.end();
			
		while ((it != end) && (*it <= reconstruction_time))
		{
			++it;
		}	
		export_times.push_back(reconstruction_time);
		while (it != end)
		{
			export_times.push_back(*it);
			++it;
		}
	}

	
	void
	get_points_from_multipoint(
		std::vector<GPlatesMaths::LatLonPoint> &points,
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
	{
		GPlatesMaths::MultiPointOnSphere::const_iterator 
			it = multi_point_on_sphere->begin(),
			end = multi_point_on_sphere->end();

		for (; it != end ; ++it)
		{
			points.push_back(GPlatesMaths::make_lat_lon_point(*it));
		}		
	}
	
	
	void
	get_global_header_lines(
		std::vector<QString> &global_header_lines,
		const GPlatesFileIO::GMTFormatMotionPathsExport::referenced_files_collection_type referenced_files,
		const GPlatesModel::integer_plate_id_type &anchor_plate_id,
		const double &reconstruction_time)
	{
	
	// Adapted from GMTFormatReconstructedFeatureGeometryExport.cc
	
		// Print the anchor plate id.
		global_header_lines.push_back(
			QString("anchorPlateId ") + QString::number(anchor_plate_id));

		// Print the reconstruction time.
		global_header_lines.push_back(
			QString("reconstructionTime ") + QString::number(reconstruction_time));

		// Print the list of reconstruction filenames that the exported
		// geometries came from.
		QStringList filenames;
		GPlatesFileIO::GMTFormatMotionPathsExport::referenced_files_collection_type::const_iterator file_iter;
		for (file_iter = referenced_files.begin();
			file_iter != referenced_files.end();
			++file_iter)
		{
			const GPlatesFileIO::File::Reference *file = *file_iter;

			// Some files might not actually exist yet if the user created a new
			// feature collection internally and hasn't saved it to file yet.
			if (!GPlatesFileIO::file_exists(file->get_file_info()))
			{
				continue;
			}

			filenames << file->get_file_info().get_display_name(false/*use_absolute_path_name*/);
		}

		global_header_lines.push_back(filenames.join(" "));	
	}

	void
	get_feature_header_lines_from_feature_ref(
		std::vector<QString> &header_lines,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		std::vector<double> &times)
	{

		GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder finder;
		finder.visit_feature(feature_ref);

		if (finder.get_feature_info_string() != QString())
		{
			QString feature_string;
			feature_string += finder.get_feature_info_string();
			header_lines.push_back(feature_string);
		}

		if (finder.get_name() != QString())
		{
			QString name_string(" Feature name: ");
			name_string += finder.get_name();
			header_lines.push_back(name_string);
		}

		if (finder.get_reconstruction_plate_id())
		{

			QString reconstruction_plate_id_string = QString(" Recostruction plate id: ") + QString::number(*finder.get_reconstruction_plate_id());

			header_lines.push_back(reconstruction_plate_id_string);
		}

		if (finder.get_relative_plate_id())
		{

			QString relative_plate_id_string = QString(" Relative plate id: ") + QString::number(*finder.get_relative_plate_id());

			header_lines.push_back(relative_plate_id_string);
		}

		times = finder.get_times();
		if (!times.empty())
		{
			std::vector<double>::const_iterator
				it = times.begin(),
				end = times.end();

			QString times_string;	
			times_string += " ";
			times_string += QString::number(*it);
			++it;
			for( ; it != end ; ++it)
			{
				times_string += QString(",");
				times_string += QString::number(*it);
			}	
			header_lines.push_back(times_string);			
		}	
	}

}

void
GPlatesFileIO::GMTFormatMotionPathsExport::export_motion_paths(
	const motion_path_group_seq_type &motion_path_group_seq,
	const QFileInfo &qfile_info, 
	const referenced_files_collection_type referenced_files, 
	const GPlatesModel::integer_plate_id_type &anchor_plate_id, 
	const double &reconstruction_time)
{
	QFile output_file(qfile_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			qfile_info.filePath());
	}


	QTextStream output_stream(&output_file);

	// We can make use of the GMTFormatHeader class' GMTHeaderPrinter.

	std::vector<QString> global_header_lines;
	get_global_header_lines(global_header_lines,
		referenced_files,anchor_plate_id,reconstruction_time);

	GMTHeaderPrinter gmt_header_printer;
	gmt_header_printer.print_global_header_lines(output_stream,global_header_lines);

	motion_path_group_seq_type::const_iterator
		iter = motion_path_group_seq.begin(),
		end = motion_path_group_seq.end();

	for (; iter != end ; ++iter)
	{
		// Get per-feature stuff: feature info, left/right plates, times.
		const ReconstructedMotionPathExportImpl::MotionPathGroup &motion_path_group =
			*iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			motion_path_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Times as defined in the feature.
		std::vector<double> feature_times;

		// Times appropriate for output. This will not necessarily be the same as the feature times.
		std::vector<double> export_times;

		std::vector<QString> feature_header_lines;
		get_feature_header_lines_from_feature_ref(feature_header_lines,feature_ref,feature_times);

		gmt_header_printer.print_feature_header_lines(output_stream,feature_header_lines);


		get_export_times(export_times,feature_times,reconstruction_time);

		reconstructed_motion_path_seq_type::const_iterator rf_iter;
		for (rf_iter = motion_path_group.recon_motion_paths.begin();
			 rf_iter != motion_path_group.recon_motion_paths.end();
			++rf_iter)
		{
			const GPlatesAppLogic::ReconstructedMotionPath *rf = *rf_iter;

			// Print the seed point
			write_seed_point_to_stream(output_stream,*rf);
			write_motion_path_to_stream(output_stream,*rf,export_times);
		}
		
		

	}

}











