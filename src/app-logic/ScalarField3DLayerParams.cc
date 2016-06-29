/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QFileInfo>

#include "ScalarField3DLayerParams.h"

#include "ExtractScalarField3DFeatureProperties.h"

#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/ScalarField3DFileFormatReader.h"

#include "model/FeatureVisitor.h"

#include "utils/UnicodeStringUtils.h"


void
GPlatesAppLogic::ScalarField3DLayerParams::set_scalar_field_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> scalar_field_feature)
{
	d_scalar_field_feature = scalar_field_feature;

	// Clear everything in case error (and return early).
	d_minimum_depth_layer_radius = boost::none;
	d_maximum_depth_layer_radius = boost::none;
	d_scalar_min = boost::none;
	d_scalar_max = boost::none;
	d_scalar_mean = boost::none;
	d_scalar_standard_deviation = boost::none;
	d_gradient_magnitude_min = boost::none;
	d_gradient_magnitude_max = boost::none;
	d_gradient_magnitude_mean = boost::none;
	d_gradient_magnitude_standard_deviation = boost::none;

	// If there is no scalar field feature then clear everything.
	if (!d_scalar_field_feature)
	{
		// TODO: Only emit if actually changed. For now just emitting always.
		emit_modified();
		return;
	}

	GPlatesAppLogic::ExtractScalarField3DFeatureProperties visitor;
	visitor.visit_feature(d_scalar_field_feature.get());

	if (!visitor.get_scalar_field_filename())
	{
		// TODO: Only emit if actually changed. For now just emitting always.
		emit_modified();
		return;
	}

	const QString scalar_field_file_name =
			GPlatesUtils::make_qstring(*visitor.get_scalar_field_filename());

	if (!QFileInfo(scalar_field_file_name).exists())
	{
		// TODO: Only emit if actually changed. For now just emitting always.
		emit_modified();
		return;
	}

	// Catch exceptions due to incorrect version or bad formatting.
	try
	{
		// Scalar field reader to access parameters in scalar field file.
		const GPlatesFileIO::ScalarField3DFileFormat::Reader scalar_field_reader(scalar_field_file_name);

		// Read the scalar field depth range.
		d_minimum_depth_layer_radius = scalar_field_reader.get_minimum_depth_layer_radius();
		d_maximum_depth_layer_radius = scalar_field_reader.get_maximum_depth_layer_radius();

		// Read the scalar field statistics.
		d_scalar_min = scalar_field_reader.get_scalar_min();
		d_scalar_max = scalar_field_reader.get_scalar_max();
		d_scalar_mean = scalar_field_reader.get_scalar_mean();
		d_scalar_standard_deviation = scalar_field_reader.get_scalar_standard_deviation();
		d_gradient_magnitude_min = scalar_field_reader.get_gradient_magnitude_min();
		d_gradient_magnitude_max = scalar_field_reader.get_gradient_magnitude_max();
		d_gradient_magnitude_mean = scalar_field_reader.get_gradient_magnitude_mean();
		d_gradient_magnitude_standard_deviation = scalar_field_reader.get_gradient_magnitude_standard_deviation();
	}
	catch (GPlatesFileIO::ScalarField3DFileFormat::UnsupportedVersion &exc)
	{
		qWarning() << exc;
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &exc)
	{
		qWarning() << exc;
	}

	// TODO: Only emit if actually changed. For now just emitting always.
	emit_modified();
}
