/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "ScalarField3DVisualLayerParams.h"

#include "app-logic/ScalarField3DLayerTask.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/RasterColourPalette.h"


GPlatesPresentation::ScalarField3DVisualLayerParams::ScalarField3DVisualLayerParams(
		GPlatesAppLogic::LayerTaskParams &layer_task_params) :
	VisualLayerParams(layer_task_params),
	d_colour_palette_filename(QString()),
	d_colour_palette(GPlatesGui::DefaultNormalisedRasterColourPalette::create())
{
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
		dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
				&get_layer_task_params());

	if (layer_task_params && layer_task_params->get_scalar_field_feature())
	{
		update(true);
		return;
	}
	// ...there's no scalar field feature...

	if (d_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		d_colour_palette = GPlatesGui::DefaultNormalisedRasterColourPalette::create();
	}

	emit_modified();
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::update(
		bool always_emit_modified_signal)
{
	bool emit_modified_signal = always_emit_modified_signal;

	// If the iso-value has not yet been set then initialise it with the scalar field mean value.
	if (!d_iso_value)
	{
		GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
					&get_layer_task_params());

		// If we have a scalar field then set the iso-value.
		if (layer_task_params && layer_task_params->get_scalar_mean())
		{
			d_iso_value = layer_task_params->get_scalar_mean().get();
			emit_modified_signal = true;
		}
	}

	// Create a new colour palette if the colour palette currently in place was not
	// loaded from a file.
	if (d_colour_palette_filename.isEmpty())
	{
		d_colour_palette = GPlatesGui::DefaultNormalisedRasterColourPalette::create();
		emit_modified_signal = true;
	}

	if (emit_modified_signal)
	{
		emit_modified();
	}
}


const QString &
GPlatesPresentation::ScalarField3DVisualLayerParams::get_colour_palette_filename() const
{
	return d_colour_palette_filename;
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::set_colour_palette(
		const QString &filename,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette)
{
	d_colour_palette_filename = filename;
	d_colour_palette = colour_palette;
	emit_modified();
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::use_auto_generated_colour_palette()
{
	d_colour_palette_filename = QString();
	update(true /* emit modified signal for us */);
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesPresentation::ScalarField3DVisualLayerParams::get_colour_palette() const
{
	return d_colour_palette;
}
