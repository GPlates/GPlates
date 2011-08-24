/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#include <boost/scoped_ptr.hpp>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GMTFormatResolvedTopologicalBoundaryExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatGeometryExporter.h"
#include "GMTFormatHeader.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/StringFormattingUtils.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;
using namespace GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl;


namespace GPlatesFileIO
{
	namespace GMTFormatResolvedTopologicalBoundaryExport
	{
		namespace
		{
			/**
			 * Convenience wrapper for opening a text file and attached QTextStream to it.
			 */
			class TextStream
			{
			public:
				TextStream(
						const QFileInfo &file_info,
						bool open_file = true) :
					d_file_info(file_info),
					d_file(file_info.filePath())
				{
					if (open_file)
					{
						open();
					}
				}


				bool
				is_open() const
				{
					return d_text_stream.device();
				}


				void
				open()
				{
					if ( ! d_file.open(QIODevice::WriteOnly | QIODevice::Text) )
					{
						throw GPlatesFileIO::ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
								d_file_info.filePath());
					}

					d_text_stream.setDevice(&d_file);
				}


				/**
				 * Returns text stream (NOTE: must have opened file).
				 */
				QTextStream &
				qtext_stream()
				{
					return d_text_stream;
				}

			private:
				QFileInfo d_file_info;
				QFile d_file;
				QTextStream d_text_stream;
			};


			// 
			// Functions to look for specific property values in a feature
			//


			/**
			 * Looks for "gml:name" property in feature otherwise
			 * looks at GpmlOldPlatesHeader for geographic description (if non-null)
			 * otherwise returns false.
			 */
			bool
			get_feature_name(
					QString &name,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature,
					const GPlatesPropertyValues::GpmlOldPlatesHeader *gpml_old_plates_header)
			{
				// Look for a property with property name "gml:name" and use its value
				// to help generate the header line. If that property doesn't exist
				// then use the geographic description in the old plates header instead.
				static const GPlatesModel::PropertyName name_property_name =
					GPlatesModel::PropertyName::create_gml("name");
				const GPlatesPropertyValues::XsString *feature_name = NULL;
				if (!GPlatesFeatureVisitors::get_property_value(
						feature, name_property_name, feature_name))
				{
					if (gpml_old_plates_header == NULL)
					{
						return false;
					}

					name = GPlatesUtils::make_qstring_from_icu_string(
							gpml_old_plates_header->geographic_description());

					return true;
				}

				name = GPlatesUtils::make_qstring_from_icu_string(feature_name->value().get());

				return true;
			}

			/**
			 * Looks for "gml:name" property in feature
			 * otherwise returns false.
			 */
			bool
			get_feature_name(
					QString &name,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				// Look for a property with property name "gml:name" and use its value
				// to help generate the header line. 
				static const GPlatesModel::PropertyName name_property_name =
					GPlatesModel::PropertyName::create_gml("name");
				const GPlatesPropertyValues::XsString *feature_name = NULL;
				if (!GPlatesFeatureVisitors::get_property_value(
						feature, name_property_name, feature_name))
				{
					name = "Unknown";
					return false;
				}

				name = GPlatesUtils::make_qstring_from_icu_string(feature_name->value().get());
				return true;
			}


			/**
			 * Looks for "gpml:subductionZoneAge" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_sz_age(
					QString &age,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("subductionZoneAge");
				const GPlatesPropertyValues::XsDouble *property_value = NULL;

				if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
				{
					return false;
				}
				double d = property_value->value();
				std::string s = GPlatesUtils::formatted_double_to_string(d, 9, 1);
				QString qs( s.c_str() );
				age = qs;
				return true;
			}

			/**
			 * Looks for "gpml:subductionZoneDeepDip" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_sz_dip(
					QString &dip,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("subductionZoneDeepDip");
				const GPlatesPropertyValues::XsDouble *property_value = NULL;

				if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
				{
					return false;
				}
				double d = property_value->value();
				std::string s = GPlatesUtils::formatted_double_to_string(d, 9, 1);
				QString qs( s.c_str() );
				dip = qs;
				return true;
			}

			/**
			 * Looks for "gpml:subductionZoneDepth" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_sz_depth(
					QString &depth,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("subductionZoneDepth");
				const GPlatesPropertyValues::XsDouble *property_value = NULL;

				if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
				{
					return false;
				}

				double d = property_value->value();
				QString d_as_str( GPlatesUtils::formatted_double_to_string(d, 6, 1).c_str() );
				depth = d_as_str;

				return true;
			}

			/**
			 * Looks for "gpml:rheaFault" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_rhea_fault(
					QString &rhea_fault,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("rheaFault");
				const GPlatesPropertyValues::XsString *property_value = NULL;
				if (!GPlatesFeatureVisitors::get_property_value(
						feature, property_name, property_value))
				{
					rhea_fault = "Unknown";
					return false;
				}

				rhea_fault = GPlatesUtils::make_qstring_from_icu_string(property_value->value().get());
				return true;
			}

			/**
			 * Looks for "gpml:slabFlatLying" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_slab_flat_lying(
					QString &flat,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("slabFlatLying");
				const GPlatesPropertyValues::XsBoolean *property_value = NULL;

				if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
				{
					return false;
				}
				flat = property_value->value() ? QString("True") : QString("False");
				return true;
			}

			/**
			 * Looks for "gpml:slabFlatLyingDepth" property in feature 
			 * otherwise returns false.
			 */
			bool
			get_feature_slab_flat_lying_depth(
					QString &value,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName property_name =
					GPlatesModel::PropertyName::create_gpml("slabFlatLyingDepth");
				const GPlatesPropertyValues::XsDouble *property_value = NULL;

				if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
				{
					return false;
				}
				double d = property_value->value();
				std::string s = GPlatesUtils::formatted_double_to_string(d, 9, 1);
				QString qs( s.c_str() );
				value = qs;
				return true;
			}

			/**
			 * Get a two-letter PLATES data type code from the subsegment type if
			 * it's a subduction zone, otherwise get the data type code from a
			 * GpmlOldPlatesHeader if there is one, otherwise get the full gpml
			 * feature type.
			 */
			const QString
			get_feature_type_code_2chars( const SubSegmentType sub_segment_type )
			{
				switch (sub_segment_type)
				{
				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
				case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT:
					return "sL";
				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
				case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT:
					return "sR";
				case SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH:
					return "SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH";
				case SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE:
					return "SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE";
				default: 
					return "??";
				}
				// Note: We don't test for SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN.
			}

			/**
			 * Get a two-letter PLATES data type code from the subsegment type if
			 * it's a subduction zone, otherwise get the data type code from a
			 * GpmlOldPlatesHeader if there is one, otherwise get the full gpml
			 * feature type.
			 */
			const QString
			get_feature_type_code(
					const GPlatesModel::FeatureHandle::const_weak_ref &source_feature,
					const SubSegmentType sub_segment_type)
			{
				// First check via the sub_segment_type
				QString test = get_feature_type_code_2chars( sub_segment_type ); 

				if (test != "??")
				{
					return test;
				}
				
				// Now check old plates header

				static const GPlatesModel::PropertyName old_plates_header_property_name =
					GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

				const GPlatesPropertyValues::GpmlOldPlatesHeader *source_feature_old_plates_header = NULL;
				GPlatesFeatureVisitors::get_property_value(
						source_feature,
						old_plates_header_property_name,
						source_feature_old_plates_header);

				// The type is not a subduction left or right so just output the plates
				// data type code if there is an old plates header.
				if (source_feature_old_plates_header)
				{
					return GPlatesUtils::make_qstring_from_icu_string(
							source_feature_old_plates_header->data_type_code());
				}

				// It's not a subduction zone and it doesn't have an old plates header
				// so just return the full gpml feature type.
				return GPlatesUtils::make_qstring_from_icu_string(
						source_feature->feature_type().get_name());
			}

			//
			// The Header classes
			//

			/**
			 * Interface for formatting of a GMT feature header.
			 */
			class GMTExportHeader
			{
			public:
				virtual
					~GMTExportHeader()
				{  }

				/**
				 * Format feature into a sequence of header lines (returned as strings).
				 */
				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const = 0;
			};

			/**
			 * Formats GMT header using GPlates8 old feature id style that looks like:
			 *
			 * "> NAM;gplates_00_00_0000_NAM_101_   1.0_-999.0_PP_0001_000_"
			 */
			class GMTOldFeatureIdStyleHeader :
					public GMTExportHeader
			{
			public:
				GMTOldFeatureIdStyleHeader(
						const GPlatesModel::FeatureHandle::const_weak_ref &feature)
				{
					// Get an OldPlatesHeader that contains attributes that are updated
					// with GPlates properties where available.
					GPlatesFileIO::OldPlatesHeader old_plates_header;
					GPlatesFileIO::PlatesLineFormatHeaderVisitor plates_header_visitor;
					plates_header_visitor.get_old_plates_header(
							feature,
							old_plates_header,
							false/*append_feature_id_to_geographic_description*/);

					GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type gpml_old_plates_header =
							old_plates_header.create_gpml_old_plates_header();

					QString name;
					if (!get_feature_name(name, feature, gpml_old_plates_header.get()))
					{
						return;
					}

					const QString old_feature_id =
							QString(gpml_old_plates_header->old_feature_id().c_str());

					d_header_line = ' ' + name + ';' + old_feature_id;
				}


				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const
				{
					header_lines.push_back(d_header_line);
				}

			private:
				QString d_header_line;
			};

			/**
			 * Formats an export GMT header:
			 *
			 * ">sL # name: Trenched_on NAP_PAC_1 # ... # polygon: NAM # use_reverse: no"
			 */
			class PlatePolygonSubSegmentHeader :
					public GMTExportHeader
			{
			public:
				PlatePolygonSubSegmentHeader(
						const GPlatesModel::FeatureHandle::const_weak_ref &feature,
						const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature,
						const GPlatesAppLogic::ResolvedTopologicalBoundarySubSegment &sub_segment,
						const SubSegmentType sub_segment_type)
				{
					d_header_line = "> ";

					QString unk = "Unknown";

					// Feature name
					QString feature_name;
					if (!get_feature_name( feature_name, feature))
					{
						feature_name = unk;
					}

					//qDebug() << "PlatePolygonSubSegmentHeader: name =" << feature_name;

					// Get a two-letter PLATES data type code from the subsegment type 
					const QString feature_type_code = get_feature_type_code(
							feature,
							sub_segment_type);

					// start up the header line
					d_header_line =
							feature_type_code +
							" # name: " +
							feature_name;

					//
					// Continue adding props and values to the header line
					//

					QString age;
					d_header_line.append( " # subductionZoneAge: " );
					if ( get_feature_sz_age(age, feature) )
					{
						d_header_line.append( age );
					}
					else { d_header_line.append( unk ); }


					QString dip;
					d_header_line.append( " # subductionZoneDeepDip: " );
					if ( get_feature_sz_dip(dip, feature) )
					{
						d_header_line.append( dip );
					}
					else { d_header_line.append( unk ); }


					QString depth;
					d_header_line.append( " # subductionZoneDepth: " );
					if ( get_feature_sz_depth(depth, feature) )
					{
						d_header_line.append( depth );
					}
					else { d_header_line.append( unk ); }

					QString rhea_fault;
					d_header_line.append( " # rheaFault: " );
					if ( get_feature_rhea_fault(rhea_fault, feature) )
					{
						d_header_line.append( rhea_fault );
					}
					else { d_header_line.append( unk ); }


					// Plate Polygon name 
					QString platepolygon_feature_name;
					if (!get_feature_name( platepolygon_feature_name, platepolygon_feature))
					{
						platepolygon_feature_name = unk;
					}

					d_header_line.append( " # polygon: ");
					d_header_line.append( platepolygon_feature_name );

					d_header_line.append( " # use_reverse: ");
					d_header_line.append( sub_segment.get_use_reverse() ? "yes" : "no");
				}


				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const
				{
					header_lines.push_back(d_header_line);
				}

			private:
				QString d_header_line;


			};


			/**
			 * Formats GMT header for Slab Polygon Sub Segments
			 */
			class SlabPolygonSubSegmentHeader :
					public GMTExportHeader
			{
			public:
				SlabPolygonSubSegmentHeader(
						const GPlatesModel::FeatureHandle::const_weak_ref &feature,
						const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature,
						const GPlatesAppLogic::ResolvedTopologicalBoundarySubSegment &sub_segment,
						const SubSegmentType sub_segment_type)
				{
					d_header_line = "> ";

					QString unk = "Unknown";

					// Feature name
					QString feature_name;
					if (!get_feature_name( feature_name, feature))
					{
						feature_name = unk;
					}

					//qDebug() << "SlabPolygonSubSegmentHeader: name =" << feature_name;

					// Get a two-letter PLATES data type code from the subsegment type 
					const QString feature_type_code = get_feature_type_code(
							feature,
							sub_segment_type);

					// start up the header line
					d_header_line =
							feature_type_code +
							" # name: " +
							feature_name;

					// continue adding props and values to the header line
					QString dip;
					d_header_line.append( " # subductionZoneDeepDip: " );
					if ( get_feature_sz_dip(dip, feature) )
					{
						d_header_line.append( dip );
					}
					else { d_header_line.append( unk ); }


					QString depth;
					d_header_line.append( " # subductionZoneDepth: " );
					if ( get_feature_sz_depth(depth, feature) )
					{
						d_header_line.append( depth );
					}
					else { d_header_line.append( unk ); }


					QString flat;
					d_header_line.append( " # slabFlatLying: " );
					if ( get_feature_slab_flat_lying(flat, feature) )
					{
						d_header_line.append( flat );
					}
					else { d_header_line.append( unk ); }


					QString flat_lying_depth;
					d_header_line.append( " # slabFlatLyingDepth: " );
					if ( get_feature_slab_flat_lying_depth(flat_lying_depth, feature) )
					{
						d_header_line.append( flat_lying_depth );
					}
					else { d_header_line.append( unk ); }

					// Plate Polygon name 
					QString platepolygon_feature_name;
					if (!get_feature_name( platepolygon_feature_name, platepolygon_feature))
					{
						platepolygon_feature_name = unk;
					}

					d_header_line.append( " # polygon: ");
					d_header_line.append( platepolygon_feature_name );
				}

				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const
				{
					header_lines.push_back(d_header_line);
				}

			private:
				QString d_header_line;
			};

			/**
			 * Formats GMT header for Slab Polygons
			 */
			class SlabPolygonStyleHeader :
					public GMTExportHeader
			{
			public:
				SlabPolygonStyleHeader(
						const GPlatesModel::FeatureHandle::const_weak_ref &feature)
				{

					// Get an OldPlatesHeader that contains attributes that are updated
					// with GPlates properties where available.
					GPlatesFileIO::OldPlatesHeader old_plates_header;
					GPlatesFileIO::PlatesLineFormatHeaderVisitor plates_header_visitor;
					plates_header_visitor.get_old_plates_header(
							feature,
							old_plates_header,
							false/*append_feature_id_to_geographic_description*/);

					GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type gpml_old_plates_header =
							old_plates_header.create_gpml_old_plates_header();

					d_header_line = ' ';

					QString unk = "Unknown";

					QString name;
					if ( get_feature_name(name, feature, gpml_old_plates_header.get()) )
					{
						d_header_line.append( name );
					}
					else { d_header_line.append( unk ); }

					QString flat;
					d_header_line.append( " # slabFlatLying: " );
					if ( get_feature_slab_flat_lying(flat, feature) )
					{
						d_header_line.append( flat );
					}
					else { d_header_line.append( unk ); }


					QString flat_lying_depth;
					d_header_line.append( " # slabFlatLyingDepth: " );
					if ( get_feature_slab_flat_lying_depth(flat_lying_depth, feature) )
					{
						d_header_line.append( flat_lying_depth );
					}
					else { d_header_line.append( unk ); }


				}

				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const
				{
					header_lines.push_back(d_header_line);
				}

			private:
				QString d_header_line;
			};


			/**
			 * Formats GMT header for Network 
			 */
			class NetworkBoundaryStyleHeader :
					public GMTExportHeader
			{
			public:
				NetworkBoundaryStyleHeader(
						const GPlatesModel::FeatureHandle::const_weak_ref &feature)
				{

					// Get an OldPlatesHeader that contains attributes that are updated
					// with GPlates properties where available.
					GPlatesFileIO::OldPlatesHeader old_plates_header;
					GPlatesFileIO::PlatesLineFormatHeaderVisitor plates_header_visitor;
					plates_header_visitor.get_old_plates_header(
							feature,
							old_plates_header,
							false/*append_feature_id_to_geographic_description*/);

					GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type gpml_old_plates_header =
							old_plates_header.create_gpml_old_plates_header();

					d_header_line = ' ';

					QString unk = "Unknown";

					QString name;
					if ( get_feature_name(name, feature, gpml_old_plates_header.get()) )
					{
						d_header_line.append( name );
					}
					else { d_header_line.append( unk ); }
				}

				virtual
				void
				get_feature_header_lines(
						std::vector<QString>& header_lines) const
				{
					header_lines.push_back(d_header_line);
				}

			private:
				QString d_header_line;
			};



			/**
			 * Handles exporting of a feature's geometry and header to GMT format.
			 */
			class GMTFeatureExporter
			{
			public:
				/**
				 * Constructor.
				 * @param filename name of file to export to
				 * @param gmt_header type of GMT header format to use
				 */
				GMTFeatureExporter(
						const QFileInfo &file_info) :
					d_output_stream(file_info, false/*we only open file if we write to it*/)
				{  }


				/**
				 * Write a feature's header and geometry to GMT format.
				 */
				void
				print_gmt_header_and_geometry(
						const GMTExportHeader &gmt_header,
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
				{
					// Open file for writing if we haven't already.
					if (!d_output_stream.is_open())
					{
						d_output_stream.open();
					}

					// Get the header lines.
					std::vector<QString> header_lines;
					gmt_header.get_feature_header_lines(header_lines);

					// Print the header lines.
					// Might be empty if no lines in which case a single '>' character is printed out
					// as is the standard for GMT headers.
					d_gmt_header_printer.print_feature_header_lines(
							d_output_stream.qtext_stream(), header_lines);

					// Write the geometry in GMT format.
					// Note we still output the geometry data even if there's an empty header.
					GPlatesFileIO::GMTFormatGeometryExporter geom_exporter(d_output_stream.qtext_stream());
					geom_exporter.export_geometry(geometry);
				}

			private:
				//! Does writing to file.
				TextStream d_output_stream;

				//! Does the actual printing of GMT header to the output stream.
				GPlatesFileIO::GMTHeaderPrinter d_gmt_header_printer;
			};
		}
	}
}

void
GPlatesFileIO::GMTFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
		const resolved_geom_seq_type &resolved_topological_geometries,
		ResolvedTopologicalBoundaryExportImpl::ResolvedTopologicalBoundaryExportType export_type,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Open the file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.filePath());
	}

	// NOTE: For this particular format we *don't* write out the global header
	// (at the top of the exported file).
	// This is because this format is specifically used as input to CitcomS which expects
	// a certain format.
	//
	// TODO: Keep this CitcomS-specific format separate from a generalised GMT format
	// (which will later be handled by the OGR library - just like Shapefiles).
	//

	// Used to write in GMT format.
	GMTFeatureExporter geom_exporter(output_file);

	// Iterate through the resolved topological geometries and write to output.
	resolved_geom_seq_type::const_iterator resolved_geom_iter;
	for (resolved_geom_iter = resolved_topological_geometries.begin();
		resolved_geom_iter != resolved_topological_geometries.end();
		++resolved_geom_iter)
	{
		const GPlatesAppLogic::ReconstructionGeometry *resolved_geom = *resolved_geom_iter;

		// Get the resolved boundary subsegments.
		boost::optional<const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundarySubSegment> &> boundary_sub_segments =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_sub_segment_sequence(resolved_geom);
		// If not a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork then skip.
		if (!boundary_sub_segments)
		{
			continue;
		}

		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> boundary_polygon =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_polygon(resolved_geom);
		// If not a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork then skip.
		if (!boundary_polygon)
		{
			continue;
		}

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(resolved_geom);
		if (!feature_ref || !feature_ref->is_valid())
		{
			continue;
		}

		// Choose the style of GMT header based on the type of topological polygon export.
		boost::scoped_ptr<GMTExportHeader> gmt_export_header;
		switch (export_type)
		{
		case PLATE_POLYGON_EXPORT_TYPE:
			gmt_export_header.reset(new GMTOldFeatureIdStyleHeader(feature_ref.get()));
			break;
		case SLAB_POLYGON_EXPORT_TYPE:
			gmt_export_header.reset(new SlabPolygonStyleHeader(feature_ref.get()));
			break;
		case NETWORK_POLYGON_EXPORT_TYPE:
			gmt_export_header.reset(new NetworkBoundaryStyleHeader(feature_ref.get()));
			break;
		default:
			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			continue;
		}

		// Write out the resolved topological boundary.
		geom_exporter.print_gmt_header_and_geometry(
				*gmt_export_header,
				boundary_polygon.get());
	}
}


void
GPlatesFileIO::GMTFormatResolvedTopologicalBoundaryExport::export_sub_segments(
		const sub_segment_group_seq_type &sub_segments,
		ResolvedTopologicalBoundaryExportImpl::SubSegmentExportType export_type,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Open the file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.filePath());
	}

	// NOTE: For this particular format we *don't* write out the global header
	// (at the top of the exported file).
	// This is because this format is specifically used as input to CitcomS which expects
	// a certain format.
	//
	// TODO: Keep this CitcomS-specific format separate from a generalised GMT format
	// (which will later be handled by the OGR library - just like Shapefiles).
	//

	// Used to write in GMT format.
	GMTFeatureExporter geom_exporter(output_file);

	// Iterate through the subsegment groups and write them out.
	sub_segment_group_seq_type::const_iterator sub_segment_group_iter;
	for (sub_segment_group_iter = sub_segments.begin();
		sub_segment_group_iter != sub_segments.end();
		++sub_segment_group_iter)
	{
		const SubSegmentGroup &sub_segment_group = *sub_segment_group_iter;

		// The topological geometry feature.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> resolved_geom_feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						sub_segment_group.resolved_topological_geometry);
		if (!resolved_geom_feature_ref || !resolved_geom_feature_ref->is_valid())
		{
			continue;
		}

		// Iterate through the subsegment geometries of the current resolved topological geometry.
		sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
		for (sub_segment_iter = sub_segment_group.sub_segments.begin();
			sub_segment_iter != sub_segment_group.sub_segments.end();
			++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalBoundarySubSegment *sub_segment = *sub_segment_iter;

			// The subsegment feature.
			const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_ref =
					sub_segment->get_feature_ref();
			if (!subsegment_feature_ref.is_valid())
			{
				continue;
			}

			// Choose the style of GMT header based on the type of subsegment type.
			boost::scoped_ptr<GMTExportHeader> gmt_export_header;
			switch (export_type)
			{
			case ALL_SUB_SEGMENTS_EXPORT_TYPE:
				// The file with all subsegments (regardless of type) uses a different
				// header format than the files with specific types of subsegments.
				gmt_export_header.reset(new GMTOldFeatureIdStyleHeader(subsegment_feature_ref));
				break;
			case PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE:
			case NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE:
				gmt_export_header.reset(
						new PlatePolygonSubSegmentHeader(
								subsegment_feature_ref,
								resolved_geom_feature_ref.get(),
								*sub_segment,
								get_sub_segment_type(*sub_segment, reconstruction_time)));
				break;
			case SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE:
				gmt_export_header.reset(
						new SlabPolygonSubSegmentHeader(
								subsegment_feature_ref,
								resolved_geom_feature_ref.get(),
								*sub_segment,
								get_slab_sub_segment_type(*sub_segment, reconstruction_time)));
				break;
			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				continue;
			}

			// Write out the subsegment.
			geom_exporter.print_gmt_header_and_geometry(
					*gmt_export_header,
					sub_segment->get_geometry());
		}
	}
}
