/* $Id$ */

/**
 * \file 
 * Contains the definition of class GpmlOutputVisitor.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLOUTPUTVISITOR_H
#define GPLATES_FILEIO_GPMLOUTPUTVISITOR_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QFile>
#include <QProcess>

#include "FileInfo.h"
#include "XmlWriter.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"

#include "property-values/GpmlTopologicalNetwork.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
	class GpmlTimeSample;
	class GpmlTimeWindow;
}

namespace GPlatesFileIO
{
	class ExternalProgram;


	class GpmlOutputVisitor:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		static
		const ExternalProgram &
		gzip_program();


		/**
		 * Creates a GPML writer for the given file.
		 *
		 * The GpmlOutputVisitor will take care of opening the file for writing,
		 * and is responsible for cleaning up afterwards.
		 *
		 * This constructor can throw a ErrorOpeningFileForWritingException.
		 */
		explicit
		GpmlOutputVisitor(
				const FileInfo &file_info,
				const GPlatesModel::Gpgim &gpgim,
				bool use_gzip);


		/**
		 * Creates a GPML writer for the given QIODevice.
		 *
		 * The GpmlOutputVisitor will write to the QIODevice, but it is the
		 * caller's responsibility for performing any necessary maintainance on the device,
		 * e.g. closing files, sockets, terminating subprocesses.
		 */
		explicit
		GpmlOutputVisitor(
				QIODevice *target,
				const GPlatesModel::Gpgim &gpgim);


		virtual
		~GpmlOutputVisitor();


		/**
		 * Start writing the document (via the XML writer) to the output file or device.
		 */
		static
		void
		start_writing_document(
				XmlWriter &writer,
				const GPlatesModel::Gpgim &gpgim);

	protected:

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_top_level_property_inline(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_data_block(
				const GPlatesPropertyValues::GmlDataBlock &gml_data_block);

		virtual
		void
		visit_gml_file(
				const GPlatesPropertyValues::GmlFile &gml_file);

		virtual
		void
		visit_gml_grid_envelope(
				const GPlatesPropertyValues::GmlGridEnvelope &gml_grid_envelope);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gml_rectified_grid(
				const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_array(
				const GPlatesPropertyValues::GpmlArray &gpml_array);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_feature_reference(
				const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference);

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_hot_spot_trail_mark(
				const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		virtual
		void
		visit_gpml_measure(
				const GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_gpml_polarity_chron_id(
				const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		virtual
		void
		visit_gpml_property_delegate(
				const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate);

		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names);

		virtual
		void
		visit_gpml_revision_id(
				const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id);

		virtual
		void
		visit_gpml_scalar_field_3d_file(
				const GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file);

		virtual
		void
		visit_gpml_string_list(
				const GPlatesPropertyValues::GpmlStringList &gpml_string_list);

		void
		write_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		void
		write_gpml_time_window(
				const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_gpml_topological_network(
				const GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_toplogical_network);

		virtual
		void
		visit_gpml_topological_polygon(
			 	const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon);

		virtual
		void
		visit_gpml_topological_line(
				const GPlatesPropertyValues::GpmlTopologicalLine &gpml_toplogical_line);

		virtual
		void
		visit_gpml_topological_line_section(
				const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section);

		virtual
		void
		visit_gpml_topological_point(
				const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point);

		virtual
		void
		visit_old_version_property_value(
				const GPlatesPropertyValues::OldVersionPropertyValue &old_version_prop_val);

		virtual
		void
		visit_uninterpreted_property_value(
				const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val);

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);
		
		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger &xs_integer);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	private:

		void
		write_gpml_key_value_dictionary_element(
				const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element);

		void
		write_gpml_topological_network_interior(
				const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &gpml_toplogical_network_interior);

		/**
		 * Keeps track of the file currently being written to.
		 *
		 * This shared_ptr will only have been initialised with a QFile *
		 * if the GpmlOutputVisitor(const FileInfo &) constructor
		 * was used. In this case, this class is responsible for opening
		 * the file and closing it afterwards - this gets a little messy
		 * as raw QIODevice pointers are preferred by the QXmlStreamWriter
		 * that this class encapsulates.
		 * 
		 * To address this problem without risking a memory leak or a
		 * double-free, this class is responsible for cleaning up any
		 * QFile objects it may have opened. boost::shared_ptr takes
		 * care of this nicely, and QFile will automatically close itself
		 * when it is destroyed. The sequence of events looks like this:
		 *
		 * ViewportWindow::save_file()
		 *  - Requests FileInfo::get_writer()
		 *     - Creates GpmlOutputVisitor shared ptr,
		 *        - Creates QFile * stored as this d_qfile member,
		 *        - passes that to XmlWriter constructor
		 *           - which passes that to QXmlStreamWriter()
		 *     - Returns GpmlOutputVisitor shared ptr,
		 *  - iterates over feature collection using visitor,
		 *  - end of function, GpmlOutputVisitor goes out of scope.
		 *     - QFile shared ptr goes out of scope.
		 *       - file gets closed.
		 *
		 * If the alternative GpmlOutputVisitor(QIODevice *)
		 * constructor has been used, this is safe as the QFile shared ptr
		 * will be empty.
		 */
		boost::shared_ptr<QFile> d_qfile_ptr;
		
		/**
		 * Keeps track of the process currently being written to.
		 *
		 * This shared_ptr will only have been initialised with a QProcess *
		 * if the GpmlOutputVisitor(const FileInfo &) constructor
		 * was used. In this case, this class is responsible for starting
		 * the process and closing it afterwards, just like the QFile
		 * shared_ptr above.
		 *
		 * boost::shared_ptr takes care of cleaning up the QProcess * when
		 * this parser is destroyed, and QProcess' destructor handles closing
		 * input/output streams and stopping processes.
		 *
		 * If the alternative GpmlOutputVisitor(QIODevice *)
		 * constructor has been used, this is safe as the QFile shared ptr
		 * will be empty.
		 */
		boost::shared_ptr<QProcess> d_qprocess_ptr;
		

		/**
		 * The destination of the the XML data.
		 */
		XmlWriter d_output;
		
		/**
		 * Whether or not we need to perform a gzip after producing uncompressed gpml output.
		 * This should be true when compressed output is requested on a Windows system. 
		 */
		bool d_gzip_afterwards;

		/**
		 * The requested output filename. 
		 */
		QString d_output_filename;
	};
}

#endif  // GPLATES_FILEIO_GPMLOUTPUTVISITOR_H
