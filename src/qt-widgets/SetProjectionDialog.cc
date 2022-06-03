/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include "SetProjectionDialog.h"

#include "MapCanvas.h"
#include "ReconstructionViewWidget.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


GPlatesQtWidgets::SetProjectionDialog::SetProjectionDialog(
		QWidget *parent_) :
	GPlatesDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint)
{
	setupUi(this);

	// First add the globe projections.
	for (unsigned int globe_projection_index = 0;
		globe_projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS;
		++globe_projection_index)
	{
		const GPlatesGui::GlobeProjection::Type globe_projection_type =
				static_cast<GPlatesGui::GlobeProjection::Type>(globe_projection_index);

		combo_globe_map_projection->addItem(
				tr(GPlatesGui::GlobeProjection::get_display_name(globe_projection_type)),
				globe_projection_index);
	}

	// Then add the map projections.
	// NOTE: The map projections will have combo box indices after the globe projections.
	for (unsigned int map_projection_index = 0;
		map_projection_index < GPlatesGui::MapProjection::NUM_PROJECTIONS;
		++map_projection_index)
	{
		const GPlatesGui::MapProjection::Type map_projection_type =
				static_cast<GPlatesGui::MapProjection::Type>(map_projection_index);

		combo_globe_map_projection->addItem(
				tr(GPlatesGui::MapProjection::get_display_name(map_projection_type)),
				// Add map projection indices *after* the globe projection indices...
				GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_index);
	}

	// Now add the viewport projections.
	for (unsigned int viewport_projection_index = 0;
		viewport_projection_index < GPlatesGui::ViewportProjection::NUM_PROJECTIONS;
		++viewport_projection_index)
	{
		const GPlatesGui::ViewportProjection::Type viewport_projection =
				static_cast<GPlatesGui::ViewportProjection::Type>(viewport_projection_index);

		// Add to the combo box.
		combo_viewport_projection->addItem(
				tr(GPlatesGui::ViewportProjection::get_display_name(viewport_projection)),
				viewport_projection_index);
	}

	// The central_meridian spinbox should be disabled if we're in a globe projection. 
	update_map_central_meridian_status();

	QObject::connect(
			combo_globe_map_projection,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(update_map_central_meridian_status()));

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::SetProjectionDialog::set_projection(
		const GPlatesGui::Projection &projection)
{
	//
	// Globe/map projection.
	//

	const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection = projection.get_globe_map_projection();

	// Get the globe/map projection index (it's either a globe or map projection).
	unsigned int globe_map_projection_index;
	if (globe_map_projection.is_viewing_globe_projection())
	{
		globe_map_projection_index = globe_map_projection.get_globe_projection_type();
	}
	else // map projection...
	{
		// Map projection indices come *after* the globe projection indices.
		globe_map_projection_index = GPlatesGui::GlobeProjection::NUM_PROJECTIONS + globe_map_projection.get_map_projection_type();

		set_map_central_meridian(globe_map_projection.get_map_central_meridian());
	}

	// Now we can quickly select the appropriate line of the combobox by finding our globe/map projection ID
	// (and not worrying about the text label).
	const int globe_map_idx = combo_globe_map_projection->findData(globe_map_projection_index);
	if (globe_map_idx != -1)
	{
		combo_globe_map_projection->setCurrentIndex(globe_map_idx);
	}

	//
	// Viewport projection.
	//

	const GPlatesGui::Projection::viewport_projection_type viewport_projection = projection.get_viewport_projection();

	// Get the viewport projection index of the viewport projection.
	const unsigned int viewport_projection_index = viewport_projection;

	// Now we can quickly select the appropriate line of the combobox by finding our viewport projection ID
	// (and not worrying about the text label).
	const int viewport_idx = combo_viewport_projection->findData(viewport_projection_index);
	if (viewport_idx != -1)
	{
		combo_viewport_projection->setCurrentIndex(viewport_idx);
	}
}


GPlatesGui::Projection::globe_map_projection_type
GPlatesQtWidgets::SetProjectionDialog::get_globe_map_projection() const
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant globe_map_projection_qv = combo_globe_map_projection->itemData(combo_globe_map_projection->currentIndex());

	// Extract globe/map projection index from QVariant.
	const unsigned int projection_index = globe_map_projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS + GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// If it's a globe projection.
	if (projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		return GPlatesGui::Projection::globe_map_projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(projection_index));
	}

	const GPlatesGui::MapProjection::Type map_projection_type =
			// Projection index is after all globe projections...
			static_cast<GPlatesGui::MapProjection::Type>(projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS);
	const double map_central_meridian = get_map_central_meridian();

	return GPlatesGui::Projection::globe_map_projection_type(
			map_projection_type,
			map_central_meridian);
}


GPlatesGui::Projection::viewport_projection_type
GPlatesQtWidgets::SetProjectionDialog::get_viewport_projection() const
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant viewport_projection_qv = combo_viewport_projection->itemData(combo_viewport_projection->currentIndex());

	// Extract viewport projection index from QVariant.
	const unsigned int projection_index = viewport_projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_index < GPlatesGui::ViewportProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesGui::Projection::viewport_projection_type viewport_projection =
			static_cast<GPlatesGui::ViewportProjection::Type>(projection_index);

	return viewport_projection;
}


void
GPlatesQtWidgets::SetProjectionDialog::update_map_central_meridian_status()
{
	// Disable for the globe projections (map projections added to combo box after globe projections).
	spin_central_meridian->setDisabled(
			combo_globe_map_projection->currentIndex() < GPlatesGui::GlobeProjection::NUM_PROJECTIONS);
}


void
GPlatesQtWidgets::SetProjectionDialog::set_map_central_meridian(
		const double &map_central_meridian)
{
	spin_central_meridian->setValue(map_central_meridian);
}


double
GPlatesQtWidgets::SetProjectionDialog::get_map_central_meridian() const
{
	return spin_central_meridian->value();
}
