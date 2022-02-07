/* $Id$ */

/**
 * \file 
 * Implementation of the Plates4 rotation writer.
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

#include "PlatesRotationFormatWriter.h"

#include <ostream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "global/unicode.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsString.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/Real.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitQuaternion3D.h"

#include "utils/StringFormattingUtils.h"


namespace
{
	/**
	 * Print out a line that does not contain a rotation pole.
	 *
	 * If using the old-style PLATES4 format then also comment out the line using a commented rotation pole.
	 *
	 * Note: Lines containing a rotation pole should be commented out by changing the moving plate ID to 999.
	 */
	void
	print_non_rotation_pole_line(
			QTextStream &os,
			const QString &line,
			bool grot_format)
	{
		if (!grot_format)
		{
			os << "999 0.0 0.0 0.0 0.0 999 !";
		}

		os << line << endl;
	}


	/**
	 * Print the rotation pole data (with no newline).
	 */
	void
	print_rotation_pole(
			QTextStream &os,
			const GPlatesMaths::FiniteRotation &finite_rotation,
			int moving_plate_id,
			int fixed_plate_id,
			const double &time)
	{
		double latitude;
		double longitude;
		double angle;

		const GPlatesMaths::UnitQuaternion3D &quat = finite_rotation.unit_quat();
		if (!GPlatesMaths::represents_identity_rotation(quat)) 
		{
			const GPlatesMaths::UnitQuaternion3D::RotationParams rot_params =
					quat.get_rotation_params(finite_rotation.axis_hint());

			const GPlatesMaths::LatLonPoint pole =
					GPlatesMaths::make_lat_lon_point(
							GPlatesMaths::PointOnSphere(rot_params.axis));

			latitude = pole.latitude();
			longitude = pole.longitude();
			angle = GPlatesMaths::convert_rad_to_deg(rot_params.angle.dval());
		}
		else // identity rotation...
		{
			if (moving_plate_id == 999 &&
				(fixed_plate_id == 0 || fixed_plate_id == 999) &&
				GPlatesMaths::are_almost_exactly_equal(time, 0.0))
			{
				// For rotations "999 0.0 0.0 0.0 0.0 999" or "999 0.0 0.0 0.0 0.0 000"
				// leave latitude as zero (instead of 90 for North pole).
				// There are various documents that suggesting these lines are general comments.
				// So we probably shouldn't change that these lines case it messes up some software's parser.
				latitude = 0.0;
			}
			else
			{
				// Note that we use the North pole as the axis for zero-angle (identity) rotations since
				// most of the time, when dealing with palaeomag and Euler pole situations, users think of a
				// zero rotation about a pole situated at the north pole, not at the equator.
				latitude = 90.0;
			}
			longitude = 0;
			angle = 0;
		}

		/*
		 * A coordinate in the PLATES4 format is written as decimal number with
		 * 4 digits precision after the decimal point, and it must take up 9
		 * characters altogether (i.e. including the decimal point and maybe a sign).
		 */
		static const unsigned PLATES_COORDINATE_FIELDWIDTH = 9;
		static const unsigned PLATES_COORDINATE_PRECISION = 4;

		os << GPlatesUtils::formatted_int_to_string(moving_plate_id, 3, '0').c_str()
			<< " "
			<< GPlatesUtils::formatted_double_to_string(time, 5, 2, true).c_str()
			<< " "
			<< GPlatesUtils::formatted_double_to_string(latitude, PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION, true).c_str()
			<< " "
			<< GPlatesUtils::formatted_double_to_string(longitude, PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION, true).c_str()
			<< " "
			<< GPlatesUtils::formatted_double_to_string(angle, PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION, true).c_str()
			<< "  "
			<< GPlatesUtils::formatted_int_to_string(fixed_plate_id, 3, '0').c_str();
	}
}


GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatWriter(
		const FileInfo &file_info,
		bool grot_format) :
	d_grot_format(grot_format)
{
	// Open the file.
	d_output_file.reset( new QFile(file_info.get_qfileinfo().filePath()) );
	if ( ! d_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.get_qfileinfo().filePath());
	}

	d_output_stream.reset( new QTextStream(d_output_file.get()) );

	// Write output to text file as UTF8 encoded (which includes the ASCII character set).
	// If we don't specify this then (in Qt4) QTextCodec::codecForLocale() will get used
	// during encoding, which is likely to not be UTF8.
	d_output_stream->setCodec("UTF-8");
}


void
GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatAccumulator::print_rotations(
		QTextStream &os,
		bool grot_format)
{
	std::list<ReconstructionPoleData>::const_iterator reconstruction_pole_data_iter = reconstruction_poles.begin();
	std::list<ReconstructionPoleData>::const_iterator reconstruction_pole_data_end = reconstruction_poles.end();
	for ( ; reconstruction_pole_data_iter != reconstruction_pole_data_end; ++reconstruction_pole_data_iter) 
	{
		const ReconstructionPoleData &reconstruction_pole_data = *reconstruction_pole_data_iter;

		if (reconstruction_pole_data.have_sufficient_info_for_output())
		{
			print_rotation(os, reconstruction_pole_data, grot_format);
		}
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatAccumulator::print_rotation(
		QTextStream &os,
		const ReconstructionPoleData &reconstruction_pole_data,
		bool grot_format)
{
	// Separate metadata attributes into single and multi-line.
	std::vector<GPlatesModel::Metadata::shared_ptr_to_const_type> multi_line_attributes;
	std::vector<GPlatesModel::Metadata::shared_ptr_to_const_type> single_line_attributes;
	if (reconstruction_pole_data.metadata)
	{
		BOOST_FOREACH(
				const GPlatesModel::Metadata::shared_ptr_to_const_type &attribute,
				reconstruction_pole_data.metadata.get())
		{
			if (attribute->get_content().contains("\n"))
			{
				multi_line_attributes.push_back(attribute);
			}
			else
			{
				single_line_attributes.push_back(attribute);
			}
		}
	}

	// Output multi-line metadata attributes first.
	BOOST_FOREACH(const GPlatesModel::Metadata::shared_ptr_to_const_type &attribute, multi_line_attributes)
	{
		const QStringList attribute_content_lines = attribute->get_content().split("\n");

		// If any newline ending is not escaped then use multi-line attribute content quotes (triple '"').
		QString attribute_content_quote = "\"";
		for (int i = 0; i < attribute_content_lines.size() - 1; ++i)
		{
			if (!attribute_content_lines[i].simplified().endsWith("\\"))
			{
				attribute_content_quote = "\"\"\"";
				break;
			}
		}

		// Print the '@NAME"""' part.
		print_non_rotation_pole_line(
				os,
				QString("@") + attribute->get_name() + attribute_content_quote,
				grot_format);

		// Print the individual content lines.
		for (int n = 0; n < attribute_content_lines.size(); ++n)
		{
			print_non_rotation_pole_line(
					os,
					attribute_content_lines[n],
					grot_format);
		}

		// Print the closing quote '"""' part.
		print_non_rotation_pole_line(
				os,
				attribute_content_quote,
				grot_format);
	}

	int moving_plate_id_or_comment = moving_plate_id.get();

	// If the 'is_disabled' flag is set, this rotation is considered "commented out". 
	if (reconstruction_pole_data.is_disabled &&
		reconstruction_pole_data.is_disabled.get())
	{
		if (grot_format)
		{
			// For new-style GROT format this is represented by a line beginning with the
			// '#' character (and *not* changing the moving plate ID to 999).
			os << '#';
		}
		else
		{
			// For old-style PLATES4 format this is represented by giving the rotation a
			// moving plate id of 999.
			//
			// Note: We don't print a line comment ('999 0.0 0.0 0.0 0.0 999') because that's only
			// for lines not containing a rotation pole.
			moving_plate_id_or_comment = 999;
		}
	}

	print_rotation_pole(
			os,
			reconstruction_pole_data.finite_rotation.get(),
			moving_plate_id_or_comment,
			fixed_plate_id.get(),
			reconstruction_pole_data.time.get());

	// Always print a comment marker for old-style PLATES4 format (even if no comment).
	if (!grot_format)
	{
		os << " !";
	}

	if (reconstruction_pole_data.metadata &&
		!reconstruction_pole_data.metadata->empty())
	{
		BOOST_FOREACH(const GPlatesModel::Metadata::shared_ptr_to_const_type& data, single_line_attributes)
		{
			os << " @"
				<< data->get_name()
				<< "\""
				<< data->get_content()
				<< "\"";
		}
	}
	else if (reconstruction_pole_data.comment)
	{
		os << reconstruction_pole_data.comment->qstring();
	}


	os << endl;
}


bool 
GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatAccumulator::have_sufficient_info_for_output() const 
{
	// The only thing we can do without is the comment.
	return moving_plate_id && fixed_plate_id && ! reconstruction_poles.empty();
}


bool
GPlatesFileIO::PlatesRotationFormatWriter::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	static const GPlatesModel::FeatureType TOTAL_RECONSTRUCTION_SEQUENCE = 
			GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence");
	static const GPlatesModel::FeatureType ABSOLUTE_REFERENCE_FRAME =
			GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame");

	if (feature_handle.feature_type() != TOTAL_RECONSTRUCTION_SEQUENCE &&
		feature_handle.feature_type() != ABSOLUTE_REFERENCE_FRAME)
	{
		// These are not the features you're looking for.
		return false;
	}

	// Reset the accumulator.
	d_accum = PlatesRotationFormatAccumulator();

	return true;
}


void
GPlatesFileIO::PlatesRotationFormatWriter::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Print reconstruction poles when we can.
	if (d_accum.have_sufficient_info_for_output())
	{
		d_accum.print_rotations(*d_output_stream, d_grot_format);
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{
	d_accum.current_pole().metadata = std::vector<GPlatesModel::Metadata::shared_ptr_to_const_type>();
	const std::vector< boost::shared_ptr<GPlatesModel::Metadata> > &metadata = gpml_finite_rotation.get_metadata();
	for (std::size_t i = 0; i < metadata.size(); ++i)
	{
		d_accum.current_pole().metadata->push_back(metadata[i]);
	}

	d_accum.current_pole().finite_rotation = gpml_finite_rotation.get_finite_rotation();
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	// Iterate through the time samples (hence finite rotations) in the current
	// total reconstruction sequence.
	GPlatesModel::RevisionedVector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
		iter = gpml_irregular_sampling.time_samples().begin(),
		end = gpml_irregular_sampling.time_samples().end();
	for ( ; iter != end; ++iter)
	{
		write_gpml_time_sample(*iter);
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName FIXED_REFERENCE_FRAME =
			GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	static const GPlatesModel::PropertyName MOVING_REFERENCE_FRAME =
				GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");

	if (*current_top_level_propname() == FIXED_REFERENCE_FRAME)
	{
		d_accum.fixed_plate_id = gpml_plate_id.get_value();
	}
	else if (*current_top_level_propname() == MOVING_REFERENCE_FRAME)
	{
		d_accum.moving_plate_id = gpml_plate_id.get_value();
	}
	// else do nothing: the plate id must not be associated to a finite rotation.
}


void
GPlatesFileIO::PlatesRotationFormatWriter::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type &gpml_time_sample)
{
	// Start a new reconstruction pole
	d_accum.reconstruction_poles.push_back(PlatesRotationFormatAccumulator::ReconstructionPoleData());

	d_accum.current_pole().time = gpml_time_sample->valid_time()->get_time_position().value();
	d_accum.current_pole().is_disabled = gpml_time_sample->is_disabled();
	
	d_accum.d_is_expecting_a_time_sample = true;

	// Visit the finite rotation inside this time sample.
	gpml_time_sample->value()->accept_visitor(*this);

	// Visit the comment.
	if (gpml_time_sample->description())
	{
		gpml_time_sample->description().get()->accept_visitor(*this);
	}
	
	d_accum.d_is_expecting_a_time_sample = false;
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	// We could get here either by visiting the 'gml:name' or 'gml:description' property, or
	// the description field of a time sample. We actually want the latter, so avoid crashing
	// if we encounter the former (and 'd_accum.reconstruction_poles' is empty).
	if (d_accum.d_is_expecting_a_time_sample)
	{
		d_accum.current_pole().comment = xs_string.get_value().get();
	}
}
