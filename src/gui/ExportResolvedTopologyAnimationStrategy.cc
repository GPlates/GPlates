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

#include "app-logic/AppLogicUtils.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/GMTFormatGeometryExporter.h"
#include "file-io/GMTFormatHeader.h"
#include "file-io/PlatesLineFormatHeaderVisitor.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/PolygonOnSphere.h"

#include "model/ResolvedTopologicalBoundary.h"

#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/XsString.h"

#include "utils/FloatingPointComparisons.h"
#include "utils/UnicodeStringUtils.h"

#include "presentation/ViewState.h"


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


	//! Retrieves PolygonOnSphere from a GeometryOnSphere.
	class GetPlatepolygon :
			private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_polygon_on_sphere(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
		{
			d_polygon_on_sphere = boost::none;

			geometry_on_sphere.accept_visitor(*this);

			return d_polygon_on_sphere;
		}


	private:
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_polygon_on_sphere = polygon_on_sphere;
		}

	private:
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_on_sphere;
	};


	//! Retrieves PolygonOnSphere from a GeometryOnSphere.
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
	get_polygon_on_sphere(
			const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
	{
		return GetPlatepolygon().get_polygon_on_sphere(geometry_on_sphere);
	}


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
				const GPlatesModel::ResolvedTopologicalBoundary::SubSegment &sub_segment,
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
					source_feature->handle_data().feature_type().get_name());
		}
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
				const GPlatesModel::ResolvedTopologicalBoundary::SubSegment &sub_segment)
		{
			d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;

			const GPlatesModel::FeatureHandle::const_weak_ref &feature =
					sub_segment.get_feature_ref();

			visit_feature(feature);

			// We just visited 'feature' looking for:
			// - a feature type of "SubductionZone",
			// - a property named "subductingSlab",
			// - a property type of "gpml:SubductionSideEnumeration".
			// - an enumeration value other than "Unknown".
			//
			// If we didn't find this information then look for the "sL" and "sR"
			// data type codes in an old plates header if we can find one.
			//
			if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN)
			{
				get_sub_segment_feature_type_from_old_plates_header(feature);
			}

			// Check if the sub_segment is being used in reverse in the polygon boundary
			if (sub_segment.get_use_reverse())
			{
				reverse_orientation();
			}

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
			if (feature_handle.handle_data().feature_type() != subduction_zone_type)
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
			static const GPlatesModel::PropertyName subducting_slab_property_name =
					GPlatesModel::PropertyName::create_gpml("subductingSlab");

			// Only interested in detecting the "subductingSlab" property.
			// If something is not a subducting slab then it is considering a ridge/transform.
			return current_top_level_propname() == subducting_slab_property_name;
		}


		// Need this since "SubductionSideEnumeration" is in a time-dependent property value.
		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		// Need this since "SubductionSideEnumeration" is in a time-dependent property value.
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


		// Need this since "SubductionSideEnumeration" is in a time-dependent property value.
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
			static const GPlatesPropertyValues::EnumerationType subduction_side_enumeration_type(
					"gpml:SubductionSideEnumeration");
			if (!subduction_side_enumeration_type.is_equal_to(enumeration.type()))
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
			const GPlatesModel::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const double &recon_time)
	{
		return DetermineSubSegmentFeatureType(recon_time).get_sub_segment_feature_type(sub_segment);
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
			const GPlatesModel::ResolvedTopologicalBoundary &resolved_geom,
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
			const GPlatesModel::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			GMTFeatureExporter &all_platepolygons_exporter,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// The geometry stored in the resolved topological geometry should be a PolygonOnSphere.
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
				platepolygon_geom = get_polygon_on_sphere(*resolved_geom.geometry());
		if (!platepolygon_geom)
		{
			return;
		}

		GMTOldFeatureIdStyleHeader platepolygon_header(platepolygon_feature_ref);

		// Export each platepolygon to a file containing all platepolygons.
		all_platepolygons_exporter.print_gmt_header_and_geometry(
				platepolygon_header, *platepolygon_geom);

		// Also export each platepolygon to a separate file.
		export_individual_platepolygon_file(
				resolved_geom, platepolygon_header, *platepolygon_geom,
				target_dir, filebasename, placeholder_string);
	}


	void
	export_sub_segment(
			const GPlatesModel::ResolvedTopologicalBoundary::SubSegment &sub_segment,
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
	export_sub_segments(
			const GPlatesModel::ResolvedTopologicalBoundary &resolved_geom,
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
		GPlatesModel::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
				resolved_geom.sub_segment_begin();
		GPlatesModel::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
				resolved_geom.sub_segment_end();
		for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
		{
			const GPlatesModel::ResolvedTopologicalBoundary::SubSegment &sub_segment =
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
}


/**
 * This string gets inserted into template filename sequence and later replaced
 * with the different strings used to differentiate the types of export.
 *
 * NOTE: we cannot use "%1", etc as a placeholder since the class
 * ExportTemplateFilenameSequence uses that in its implementation when
 * it expands the various modifiers (like "%d").
 */
const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_string("<placeholder>");

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


const GPlatesGui::ExportResolvedTopologyAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportResolvedTopologyAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context)
{
	return non_null_ptr_type(new ExportResolvedTopologyAnimationStrategy(export_animation_context),
			GPlatesUtils::NullIntrusivePointerHandler());
}


GPlatesGui::ExportResolvedTopologyAnimationStrategy::ExportResolvedTopologyAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context):
	ExportAnimationStrategy(export_animation_context)
{
	// Set the ExportTemplateFilenameSequence name once here, to a sane default.
	// Later, we will let the user configure this.
	// This also sets the iterator to the first filename template.
	set_template_filename(QString("Polygons.xy"));
	
	// Do anything else that we need to do in order to initialise
	// our resolved topology export here...
	
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::set_template_filename(
		const QString &filename)
{
	// We want "Polygons" to look like "Polygons.<placeholder>.<age>" as that
	// is what is expected by the workflow (external to GPlates) that uses
	// this export.
	// The 's_placeholder_string' string will get replaced for each type of export
	// in 'do_export_iteration()'.
	// The "%d" tells ExportTemplateFilenameSequence to insert the reconstruction time.
	const QString suffix = "." + s_placeholder_string + ".%d";

	const QString modified_template_filename =
			append_suffix_to_template_filebasename(filename, suffix);

	// Call base class method to actually set the template filename.
	ExportAnimationStrategy::set_template_filename(modified_template_filename);
}


bool
GPlatesGui::ExportResolvedTopologyAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	// Get the iterator for the next filename.
	if (!d_filename_iterator_opt || !d_filename_sequence_opt) {
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in export iteration - not properly initialised!"));
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = *d_filename_iterator_opt;

	// Doublecheck that the iterator is valid.
	if (filename_it == d_filename_sequence_opt->end()) {
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in filename sequence - not enough filenames supplied!"));
		return false;
	}

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filebasename = *filename_it++;

	//
	// Here's where we would do the actual exporting of the resolved topologies.
	// The View is already set to the appropriate reconstruction time for
	// this frame; all we have to do is the maths and the file-writing (to @a full_filename)
	//

	GPlatesAppLogic::Reconstruct &reconstruct =
			d_export_animation_context_ptr->view_state().get_reconstruct();

	GPlatesModel::Reconstruction &reconstruction = reconstruct.get_current_reconstruction();
	const double &reconstruction_time = reconstruct.get_current_reconstruction_time();

	// Find any ResolvedTopologicalBoundary objects in the reconstruction.
	resolved_geom_seq_type resolved_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction.geometries().begin(),
				reconstruction.geometries().end(),
				resolved_geom_seq);

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
					target_dir, filebasename, s_placeholder_string, s_placeholder_platepolygons));

	// For exporting all subsegments of all platepolygons to a single file.
	GMTFeatureExporter all_sub_segments_exporter(
			get_full_output_filename(
					target_dir, filebasename, s_placeholder_string, s_placeholder_lines));

	// For exporting all ridge/transform subsegments of all platepolygons to a single file.
	GMTFeatureExporter ridge_transform_exporter(
			get_full_output_filename(
					target_dir, filebasename, s_placeholder_string, s_placeholder_ridge_transforms));

	// For exporting all subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_exporter(
			get_full_output_filename(
					target_dir, filebasename, s_placeholder_string, s_placeholder_subductions));

	// For exporting all left subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_left_exporter(
			get_full_output_filename(
					target_dir, filebasename, s_placeholder_string, s_placeholder_left_subductions));

	// For exporting all right subduction zone subsegments of all platepolygons to a single file.
	GMTFeatureExporter subduction_right_exporter(
			get_full_output_filename(
					target_dir, filebasename, s_placeholder_string, s_placeholder_right_subductions));

	// Iterate over the RTGs.
	resolved_geom_seq_type::const_iterator resolved_seq_iter = resolved_geom_seq.begin();
	resolved_geom_seq_type::const_iterator resolved_seq_end = resolved_geom_seq.end();
	for ( ; resolved_seq_iter != resolved_seq_end; ++resolved_seq_iter)
	{
		const GPlatesModel::ResolvedTopologicalBoundary *resolved_geom = *resolved_seq_iter;

		// Feature handle reference to platepolygon feature.
		const GPlatesModel::FeatureHandle::const_weak_ref platepolygon_feature_ref =
				GPlatesModel::FeatureHandle::get_const_weak_ref(
						resolved_geom->feature_handle_ptr()->reference());

		export_platepolygon(
				*resolved_geom, platepolygon_feature_ref, all_platepolygons_exporter,
				target_dir, filebasename, s_placeholder_string);

		export_sub_segments(
				*resolved_geom, platepolygon_feature_ref, recon_time,
				all_sub_segments_exporter, ridge_transform_exporter, subduction_exporter,
				subduction_left_exporter, subduction_right_exporter);
	}
}
