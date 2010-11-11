/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>
#include "ExportResolvedTopologyAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ResolvedTopologicalBoundary.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/GMTFormatGeometryExporter.h"
#include "file-io/GMTFormatHeader.h"
#include "file-io/PlatesLineFormatHeaderVisitor.h"


#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/PolygonOnSphere.h"

#include "presentation/ViewState.h"

#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/XsString.h"

#include "utils/ExportTemplateFilenameSequence.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/StringFormattingUtils.h"

namespace
{
	/**
	 *  Sub segment feature type.
	 */
	enum SubSegmentType
	{
		SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT,
		SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT,
		SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN,
		SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING,
		SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH,
		SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE,
		SUB_SEGMENT_TYPE_OTHER
	};


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
	 * Looks for "gpml:depth" property in feature 
	 * otherwise returns false.
	 */
	bool
	get_feature_depth(
			QString &depth,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		// Look for a property with property name "gml:name" and use its value
		// to help generate the header line. If that property doesn't exist
		// then use the geographic description in the old plates header instead.
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gml("depth");
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
	 * Looks for "gpml:dipAngle" property in feature 
	 * otherwise returns false.
	 */
	bool
	get_feature_dip_angle(
			QString &dip_angle,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		// Look for a property with property name "gml:name" and use its value
		// to help generate the header line. If that property doesn't exist
		// then use the geographic description in the old plates header instead.
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gml("dip_angle");
		const GPlatesPropertyValues::XsDouble *property_value = NULL;

		if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
		{
			return false;
		}

		double d = property_value->value();
		QString d_as_str( GPlatesUtils::formatted_double_to_string(d, 3, 1).c_str() );
		dip_angle = d_as_str;

		return true;
	}


	/**
	 * Looks for "gpml:flatLying" property in feature 
	 * otherwise returns false.
	 */
	bool
	get_feature_flat(
			QString &flat,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		// Look for a property with property name "gml:flat" and use its value
		// to help generate the header line. If that property doesn't exist
		// then use the geographic description in the old plates header instead.
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gml("flatLying");
		const GPlatesPropertyValues::XsBoolean *property_value = NULL;

		if (!GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value))
		{
			return false;
		}

		flat = property_value->value() ? QString("True") : QString("False");

		return true;
	}



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
	 * Formats GMT header using GPlates8 format that looks like:
	 *
	 * ">sL    # name: Trenched_on NAP_PAC_1    # polygon: NAM    # use_reverse: no"
	 */
	class GMTReferencePlatePolygonHeader :
			public GMTExportHeader
	{
	public:
		GMTReferencePlatePolygonHeader(
				const GPlatesModel::FeatureHandle::const_weak_ref &source_feature,
				const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature,
				const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
				const SubSegmentType sub_segment_type)
		{
			static const GPlatesModel::PropertyName old_plates_header_property_name =
				GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

			const GPlatesPropertyValues::GpmlOldPlatesHeader *source_feature_old_plates_header = NULL;
			GPlatesFeatureVisitors::get_property_value(
					source_feature,
					old_plates_header_property_name,
					source_feature_old_plates_header);

			// NOTE: 'source_feature_old_plates_header' could still be NULL
			// but 'get_feature_name()' checks for NULL.
			// We do this because we can still get a feature name even when it's NULL.
			QString source_feature_name;
			if (!get_feature_name(
					source_feature_name, source_feature, source_feature_old_plates_header))
			{
				return;
			}

			const GPlatesPropertyValues::GpmlOldPlatesHeader *platepolygon_feature_old_plates_header = NULL;
			GPlatesFeatureVisitors::get_property_value(
					platepolygon_feature,
					old_plates_header_property_name,
					platepolygon_feature_old_plates_header);

			// NOTE: 'platepolygon_feature_old_plates_header' could still be NULL
			// but 'get_feature_name()' checks for NULL.
			// We do this because we can still get a feature name even when it's NULL.
			QString platepolygon_feature_name;
			if (!get_feature_name(
					platepolygon_feature_name, platepolygon_feature,
					platepolygon_feature_old_plates_header))
			{
				return;
			}

			// Get a two-letter PLATES data type code from the subsegment type if
			// it's a subduction zone, otherwise get the data type code from a
			// GpmlOldPlatesHeader if there is one, otherwise get the full gpml
			// feature type.
			const QString feature_type_code = get_feature_type_code(
					source_feature,
					sub_segment_type,
					source_feature_old_plates_header);

			d_header_line =
					feature_type_code +
					"    # name: " +
					source_feature_name +
					"    # polygon: " +
					platepolygon_feature_name +
					"    # use_reverse: " +
					(sub_segment.get_use_reverse() ? "yes" : "no");
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


		/**
		 * Get a two-letter PLATES data type code from the subsegment type if
		 * it's a subduction zone, otherwise get the data type code from a
		 * GpmlOldPlatesHeader if there is one, otherwise get the full gpml
		 * feature type.
		 */
		const QString
		get_feature_type_code(
				const GPlatesModel::FeatureHandle::const_weak_ref &source_feature,
				const SubSegmentType sub_segment_type,
				const GPlatesPropertyValues::GpmlOldPlatesHeader *source_feature_old_plates_header)
		{
			// First determine the PLATES data type code from the subsegment type.
			// We do this because for subduction zones the subsegment type has
			// already accounted for any direction reversal by reversing
			// the subduction type.
			//
			// NOTE: we don't need to handle reverse direction of subsegment
			// because variables of type 'SubSegmentType' already have this
			// information in them.
			if (sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT)
			{
				return "sL";
			}
			if (sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT)
			{
				return "sR";
			}
			// Note: We don't test for SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN.

			// The type is not a subduction left or right so just output the plates
			// data type code if there is an old plates header.
			if (source_feature_old_plates_header)
			{
				return GPlatesUtils::make_qstring_from_icu_string(
						source_feature_old_plates_header->data_type_code());
			}

			// It's not a subduction zone and it doesn't have an old plates header
			// so just return the full gpml feature type.
			// Mark will put in code in his external scripts to check for this.
			return GPlatesUtils::make_qstring_from_icu_string(
					source_feature->feature_type().get_name());
		}
	};


	/**
	 * Formats GMT header for Slab Polygons
	 *
	 * "> FIXME
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

			QString name;
			if ( get_feature_name(name, feature, gpml_old_plates_header.get()) )
			{
				d_header_line.append( name );
 				d_header_line.append( ';');
			}

			QString depth;
			if ( get_feature_depth(depth, feature) )
			{
				d_header_line.append( "; depth =" );
				d_header_line.append( depth );
			}

			QString dip_angle;
			if ( get_feature_dip_angle(dip_angle, feature) )
			{
				d_header_line.append( "; dip_angle =" );
				d_header_line.append( dip_angle );
			}

			QString flat;
			if ( get_feature_flat(flat, feature) )
			{
				d_header_line.append( "; flat =" );
				d_header_line.append( flat );
			}
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


	/**
	 * Determines feature type of subsegment source feature referenced by platepolygon
	 * at a specific reconstruction time.
	 */
	class DetermineSubSegmentFeatureType :
			private GPlatesModel::ConstFeatureVisitor
	{
	public:
		DetermineSubSegmentFeatureType(
				const double &recon_time) :
			d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time))
		{  }

		SubSegmentType
		get_sub_segment_feature_type(
				const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment)
		{
			d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;

			const GPlatesModel::FeatureHandle::const_weak_ref &feature =
					sub_segment.get_feature_ref();

			
			visit_feature(feature);

			// We just visited 'feature' looking for:
			// - a feature type of "SubductionZone",
			// - a property named "subductionPolarity",
			// - a property type of "gpml:SubductionPolarityEnumeration".
			// - an enumeration value other than "Unknown".
			//
			// If we didn't find this information then look for the "sL" and "sR"
			// data type codes in an old plates header if we can find one.
			//
			if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN)
			{
				get_sub_segment_feature_type_from_old_plates_header(feature);
			}

			# if 0
			// NOTE: do not call this function;
			// The sL or sR property is set by the feature, and should not change
			// for any sub-segment
			//
			// Check if the sub_segment is being used in reverse in the polygon boundary
			if (sub_segment.get_use_reverse())
			{
				reverse_orientation();
			}
			#endif

			return d_sub_segment_type;
		}

	private:
		GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		SubSegmentType d_sub_segment_type;


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			static const GPlatesModel::FeatureType subduction_zone_type =
					GPlatesModel::FeatureType::create_gpml("SubductionZone");

			// Only interested in "SubductionZone" features.
			// If something is not a subduction zone then it is considering a ridge/transform.
			if (feature_handle.feature_type() != subduction_zone_type)
			{
				return false;
			}

			// We know it's a subduction zone but need to look at properties to
			// see if a left or right subduction zone.
			d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN;

			return true;
		}


		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &)
		{
			static const GPlatesModel::PropertyName subduction_polarity_property_name =
					GPlatesModel::PropertyName::create_gpml("subductionPolarity");

			// Only interested in detecting the "subductionPolarity" property.
			// If something is not a subduction zone then it is considering a ridge/transform.
			return current_top_level_propname() == subduction_polarity_property_name;
		}


		// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
		{
			std::vector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
				iter = gpml_irregular_sampling.time_samples().begin(),
				end = gpml_irregular_sampling.time_samples().end();
			for ( ; iter != end; ++iter)
			{
				// If time of time sample matches our reconstruction time then visit.
				if (d_recon_time.is_coincident_with(iter->valid_time()->time_position()))
				{
					iter->value()->accept_visitor(*this);
				}
			}
		}


		// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation) 
		{
			std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.time_windows().begin();
			std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.time_windows().end();
			for ( ; iter != end; ++iter)
			{
				// If the time window covers our reconstruction time then visit.
				if (iter->valid_time()->contains(d_recon_time))
				{
					iter->time_dependent_value()->accept_visitor(*this);
				}
			}
		}


		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration)
		{
			static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type(
					"gpml:SubductionPolarityEnumeration");

			if (!subduction_polarity_enumeration_type.is_equal_to(enumeration.type()))
			{
				return;
			}

			static const GPlatesPropertyValues::EnumerationContent unknown("Unknown");
			if (unknown.is_equal_to(enumeration.value()))
			{
				d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN;
				return;
			}

			static const GPlatesPropertyValues::EnumerationContent left("Left");
			d_sub_segment_type = left.is_equal_to(enumeration.value())
					? SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT
					: SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
		}


		void
		get_sub_segment_feature_type_from_old_plates_header(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature)
		{
			static const GPlatesModel::PropertyName old_plates_header_property_name =
				GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header;

			if ( GPlatesFeatureVisitors::get_property_value(
					feature,
					old_plates_header_property_name,
					old_plates_header ) )
			{
				if ( old_plates_header->data_type_code() == "sL" )
				{
					// set the type
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT;
				}

				if ( old_plates_header->data_type_code() == "sR" )
				{
					// set the type
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
				}
			}
		}


		void
		reverse_orientation()
		{
			if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT) 
			{
				// flip the orientation flag
				d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
			}
			else if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT) 
			{
				// flip the orientation flag
				d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT;
			}
		}

	};


	/**
	 * Determines feature type of subsegment source feature referenced by platepolygon.
	 */
	SubSegmentType
	get_sub_segment_type(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const double &recon_time)
	{
		return DetermineSubSegmentFeatureType(recon_time).get_sub_segment_feature_type(sub_segment);
	}


	/**
	 * Determines feature type of subsegment source feature referenced by slab polygon.
	*/
	SubSegmentType
	get_slab_sub_segment_type(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const double &recon_time)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref &feature =
				sub_segment.get_feature_ref();

		static const GPlatesModel::PropertyName property_name =
				GPlatesModel::PropertyName::create_gpml("slabEdgeType");
		const GPlatesPropertyValues::XsString *property_value = NULL;

		if (GPlatesFeatureVisitors::get_property_value(feature, property_name, property_value))
		{
			static const GPlatesPropertyValues::TextContent LEADING("Leading");
			static const GPlatesPropertyValues::TextContent TRENCH("Trench");
			static const GPlatesPropertyValues::TextContent SIDE("Side");

			if (property_value->value() == LEADING)
			{
				return SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING;
			}
			else if (property_value->value() == TRENCH)
			{
				return SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH;
			}
			else if (property_value->value() == SIDE)
			{
				return SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE;
			}
		}

		return SUB_SEGMENT_TYPE_OTHER;
	}
	


	QString
	append_suffix_to_template_filebasename(
			const QFileInfo &original_template_filename,
			QString suffix)
	{
		const QString ext = original_template_filename.suffix();
		if (ext.isEmpty())
		{
			// Shouldn't really happen.
			return original_template_filename.fileName() + suffix;
		}

		// Remove any known file suffix from the template filename.
		const QString template_filebasename = original_template_filename.completeBaseName();

		return template_filebasename + suffix + '.' + ext;
	}


	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}


	const QString
	get_full_output_filename(
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string,
			const QString &placeholder_replacement)
	{
		const QString output_basename = substitute_placeholder(filebasename,
				placeholder_string, placeholder_replacement);

		return target_dir.absoluteFilePath(output_basename);
	}


	void
	export_individual_platepolygon_file(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GMTOldFeatureIdStyleHeader &platepolygon_header,
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &platepolygon_geom,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// We're expecting a plate id as that forms part of the filename.
		if (!resolved_geom.plate_id())
		{
			return;
		}
		const QString plate_id_string = QString::number(*resolved_geom.plate_id());

		GMTFeatureExporter individual_platepolygon_exporter(
			get_full_output_filename(target_dir, filebasename, placeholder_string, plate_id_string));

		individual_platepolygon_exporter.print_gmt_header_and_geometry(
				platepolygon_header, platepolygon_geom);
	}

	void
	export_platepolygon(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			GMTFeatureExporter &all_platepolygons_exporter,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// The geometry stored in the resolved topological geometry should be a PolygonOnSphere.
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type platepolygon_geom =
				resolved_geom.resolved_topology_geometry();
		if (!platepolygon_geom)
		{
			return;
		}

		GMTOldFeatureIdStyleHeader platepolygon_header(platepolygon_feature_ref);

		// Export each platepolygon to a file containing all platepolygons.
		all_platepolygons_exporter.print_gmt_header_and_geometry(
				platepolygon_header, platepolygon_geom);

		// Also export each platepolygon to a separate file.
		export_individual_platepolygon_file(
				resolved_geom,
				platepolygon_header,
				platepolygon_geom,
				target_dir,
				filebasename,
				placeholder_string);
	}

	void
	export_individual_slab_polygon_file(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const SlabPolygonStyleHeader &slab_polygon_header,
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &slab_polygon_geom,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// We're expecting a plate id as that forms part of the filename.
		if (!resolved_geom.plate_id())
		{
			return;
		}

		QString string = "slab_";
		const QString plate_id_string = QString::number(*resolved_geom.plate_id());
		string.append(plate_id_string);

		GMTFeatureExporter individual_slab_polygon_exporter(
			get_full_output_filename(target_dir, filebasename, placeholder_string, string));

		individual_slab_polygon_exporter.print_gmt_header_and_geometry(
				slab_polygon_header, slab_polygon_geom);
	}

	void
	export_slab_polygon(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &slab_polygon_feature_ref,
			GMTFeatureExporter &all_slab_polygons_exporter,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// The geometry stored in the resolved topological geometry should be a PolygonOnSphere.
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type slab_polygon_geom =
				resolved_geom.resolved_topology_geometry();
		if (!slab_polygon_geom)
		{
			return;
		}

		SlabPolygonStyleHeader slab_polygon_header(slab_polygon_feature_ref);

		// Export each polygon to a file containing all polygons.
		all_slab_polygons_exporter.print_gmt_header_and_geometry(
				slab_polygon_header, slab_polygon_geom);

		// Also export each slab polygon to a separate file.
		export_individual_slab_polygon_file(
				resolved_geom,
				slab_polygon_header,
				slab_polygon_geom,
				target_dir,
				filebasename,
				placeholder_string);

	}


	void
	export_sub_segment(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &ridge_transform_exporter,
			GMTFeatureExporter &subduction_exporter,
			GMTFeatureExporter &subduction_left_exporter,
			GMTFeatureExporter &subduction_right_exporter)
	{
		// Determine the feature type of subsegment.
		const SubSegmentType sub_segment_type = get_sub_segment_type(sub_segment, recon_time);

		// The files with specific types of subsegments use a different header format
		// than the file with all subsegments (regardless of type).
		GMTReferencePlatePolygonHeader sub_segment_header(
				sub_segment.get_feature_ref(), platepolygon_feature_ref,
				sub_segment, sub_segment_type);

		// Export subsegment depending on its feature type.
		switch (sub_segment_type)
		{
		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
			subduction_left_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
			subduction_right_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
			// We know it's a subduction zone but don't know if left or right
			// so export to the subduction zone file only.
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_OTHER:
		default:
			ridge_transform_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;
		}
	}

	void
	export_slab_sub_segment(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const GPlatesModel::FeatureHandle::const_weak_ref &slab_polygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &slab_edge_leading_exporter,
			GMTFeatureExporter &slab_edge_trench_exporter,
			GMTFeatureExporter &slab_edge_side_exporter)
	{
		// Determine the feature type of subsegment.
		const SubSegmentType sub_segment_type = get_slab_sub_segment_type(sub_segment, recon_time);

		// The files with specific types of subsegments use a different header format
		// than the file with all subsegments (regardless of type).
		GMTReferencePlatePolygonHeader sub_segment_header(
				sub_segment.get_feature_ref(), slab_polygon_feature_ref,
				sub_segment, sub_segment_type);

		// Export subsegment depending on its feature type.
		switch (sub_segment_type)
		{
		case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING:
			slab_edge_leading_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH:
			slab_edge_trench_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE:
		default:
			slab_edge_side_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;
		}
	}




	void
	export_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &all_sub_segments_exporter,
			GMTFeatureExporter &ridge_transform_exporter,
			GMTFeatureExporter &subduction_exporter,
			GMTFeatureExporter &subduction_left_exporter,
			GMTFeatureExporter &subduction_right_exporter)
	{
		// Iterate over the subsegments contained in the current resolved topological geometry
		// and write each to GMT format file.
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
				resolved_geom.sub_segment_begin();
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
				resolved_geom.sub_segment_end();
		for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment =
					*sub_segment_iter;

			// The file with all subsegments (regardless of type) uses a different
			// header format than the files with specific types of subsegments.
			GMTOldFeatureIdStyleHeader all_sub_segment_header(sub_segment.get_feature_ref());

			// All subsegments get exported to this file.
			all_sub_segments_exporter.print_gmt_header_and_geometry(
					all_sub_segment_header, sub_segment.get_geometry());

			// Also export the sub segment to another file based on its feature type.
			export_sub_segment(
					sub_segment, platepolygon_feature_ref, recon_time,
					ridge_transform_exporter, subduction_exporter,
					subduction_left_exporter, subduction_right_exporter);
		}
	}


	void
	export_slab_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &all_sub_segments_exporter,
			GMTFeatureExporter &slab_edge_leading_exporter,
			GMTFeatureExporter &slab_edge_trench_exporter,
			GMTFeatureExporter &slab_edge_side_exporter)
	{
		// Iterate over the subsegments contained in the current resolved topological geometry
		// and write each to GMT format file.
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
				resolved_geom.sub_segment_begin();
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
				resolved_geom.sub_segment_end();
		for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment =
					*sub_segment_iter;

			// The file with all subsegments (regardless of type) uses a different
			// header format than the files with specific types of subsegments.
			GMTOldFeatureIdStyleHeader all_sub_segment_header(sub_segment.get_feature_ref());

#if 0
			// All subsegments get exported to this file.
			all_sub_segments_exporter.print_gmt_header_and_geometry(
					all_sub_segment_header, sub_segment.get_geometry());
#endif

			// Also export the sub segment to another file based on its feature type.
			export_slab_sub_segment(
					sub_segment, platepolygon_feature_ref, recon_time,
					slab_edge_leading_exporter, 
					slab_edge_trench_exporter,
					slab_edge_side_exporter);
		}
	}

}

const QString 
GPlatesGui::ExportResolvedTopologyAnimationStrategy::
	DEFAULT_RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE
		="Polygons.%P.%d.xy";

const QString 
GPlatesGui::ExportResolvedTopologyAnimationStrategy::
	RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;

const QString GPlatesGui::ExportResolvedTopologyAnimationStrategy::RESOLOVED_TOPOLOGIES_DESC 
		="Export resolved topologies.";

//
// Plate Polygon related data 
//
const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_platepolygons("platepolygons");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_lines("lines");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_ridge_transforms("ridge_transform_boundaries");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_subductions("subduction_boundaries");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_left_subductions("subduction_boundaries_sL");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_right_subductions("subduction_boundaries_sR");

//
// Slab Polygon related data
//
const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_polygons("slab_polygons");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_leading("slab_edges_leading");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_trench("slab_edges_trench");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_side("slab_edges_side");




const GPlatesGui::ExportResolvedTopologyAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportResolvedTopologyAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportResolvedTopologyAnimationStrategy * ptr = 
			new ExportResolvedTopologyAnimationStrategy(
					export_animation_context,
					cfg.filename_template());
		
	ptr->d_class_id = "RESOLVED_TOPOLOGIES_GMT";

	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}


GPlatesGui::ExportResolvedTopologyAnimationStrategy::ExportResolvedTopologyAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
		set_template_filename(filename_template);
}

void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::set_template_filename(
		const QString &filename)
{
	// We want "Polygons" to look like "Polygons.%P.%d" as that
	// is what is expected by the workflow (external to GPlates) that uses
	// this export.
	// The '%P' placeholder string will get replaced for each type of export
	// in 'do_export_iteration()'.
	// The "%d" tells ExportTemplateFilenameSequence to insert the reconstruction time.
#if 0
	//The placeholders, %P and %d", have been taken care by "export" dialog.
	//So there is no need to add them here again.
	const QString suffix =
			"." +
			GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING +
			".%d";

	const QString modified_template_filename =
			append_suffix_to_template_filebasename(filename, suffix);

#endif
	// Call base class method to actually set the template filename.
	//ExportAnimationStrategy::set_template_filename(modified_template_filename);
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportResolvedTopologyAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filebasename = *filename_it++;

	//
	// Here's where we would do the actual exporting of the resolved topologies.
	// The View is already set to the appropriate reconstruction time for
	// this frame; all we have to do is the maths and the file-writing (to @a full_filename)
	//

	GPlatesAppLogic::ApplicationState &application_state =
			d_export_animation_context_ptr->view_state().get_application_state();

	const GPlatesAppLogic::Reconstruction &reconstruction = application_state.get_current_reconstruction();
	const double &reconstruction_time = application_state.get_current_reconstruction_time();

	// Find any ResolvedTopologicalBoundary objects in the reconstruction.
	resolved_geom_seq_type resolved_geom_seq;
	GPlatesAppLogic::Reconstruction::reconstruction_tree_const_iterator reconstruction_trees_iter =
			reconstruction.begin_reconstruction_trees();
	GPlatesAppLogic::Reconstruction::reconstruction_tree_const_iterator reconstruction_trees_end =
			reconstruction.end_reconstruction_trees();
	for ( ; reconstruction_trees_iter != reconstruction_trees_end; ++reconstruction_trees_iter)
	{
		const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree =
				*reconstruction_trees_iter;

		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					reconstruction.begin_reconstruction_geometries(reconstruction_tree),
					reconstruction.end_reconstruction_geometries(reconstruction_tree),
					resolved_geom_seq);
	}

	// Temporarily patch a small bug relating to the 'd_reconstruction_tree_map' member in
	// class Reconstruction.  This bug results in duplicate pointers in 'resolved_geom_seq'.
	std::set<const GPlatesAppLogic::ResolvedTopologicalBoundary *> unique_resolved_geom_set(
			resolved_geom_seq.begin(), resolved_geom_seq.end());
	resolved_geom_seq.assign(unique_resolved_geom_set.begin(), unique_resolved_geom_set.end());

	// Export the various files.
	export_files(resolved_geom_seq, reconstruction_time, output_filebasename);
	
	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::export_files(
		const resolved_geom_seq_type &resolved_geom_seq,
		const double &recon_time,
		const QString &filebasename)
{

	const QDir &target_dir = d_export_animation_context_ptr->target_dir();

	// For exporting all platepolygons to a single file.
	GMTFeatureExporter all_platepolygons_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_platepolygons));

	// For exporting all subsegments of all platepolygons to a single file.
	GMTFeatureExporter all_sub_segments_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_lines));

	// For exporting all ridge/transform subsegments of all platepolygons to a single file.
	GMTFeatureExporter ridge_transform_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_ridge_transforms));

	// For exporting all subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_subductions));

	// For exporting all left subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_left_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_left_subductions));

	// For exporting all right subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_right_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_right_subductions));

    // For exporting all slab polygons to a single file ;
	GMTFeatureExporter all_slab_polygons_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_slab_polygons));

	// For exporting 
	GMTFeatureExporter slab_edge_leading_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_slab_edge_leading));

	// For exporting the 
	GMTFeatureExporter slab_edge_trench_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_slab_edge_trench));

	// For exporting 
	GMTFeatureExporter slab_edge_side_exporter(
			get_full_output_filename(
					target_dir,
					filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					s_placeholder_slab_edge_side));



	// Iterate over the RTGs.
	resolved_geom_seq_type::const_iterator resolved_seq_iter = resolved_geom_seq.begin();
	resolved_geom_seq_type::const_iterator resolved_seq_end = resolved_geom_seq.end();
	for ( ; resolved_seq_iter != resolved_seq_end; ++resolved_seq_iter)
	{
		const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom = *resolved_seq_iter;

		// Feature handle reference to topology feature.
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref =
				resolved_geom->feature_handle_ptr()->reference();

		static const GPlatesModel::FeatureType plate_type = 
			GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");

		static const GPlatesModel::FeatureType slab_type = 
			GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");


		if (feature_ref->feature_type() == plate_type)
		{
#if 0
			std::cout << "TopologicalClosedPlateBoundary\n";
#endif
			export_platepolygon(
					*resolved_geom, feature_ref, all_platepolygons_exporter,
					target_dir, filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING);

			export_sub_segments(
					*resolved_geom, feature_ref, recon_time,
					all_sub_segments_exporter, ridge_transform_exporter, subduction_exporter,
					subduction_left_exporter, subduction_right_exporter);
		}
	
		if (feature_ref->feature_type() == slab_type)
		{
#if 0
			std::cout << "TopologicalSlabBoundary\n";
#endif
			export_slab_polygon( *resolved_geom, feature_ref, all_slab_polygons_exporter,
					target_dir, filebasename,
					GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING);	

			export_slab_sub_segments(
					*resolved_geom, feature_ref, recon_time,
					all_sub_segments_exporter, 
					slab_edge_leading_exporter, 
					slab_edge_trench_exporter,
					slab_edge_side_exporter);
		}
	}
}


const QString&
GPlatesGui::ExportResolvedTopologyAnimationStrategy::get_default_filename_template()
{
	return DEFAULT_RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE;
}

