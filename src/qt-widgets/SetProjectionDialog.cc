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
#include "MapView.h"
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

		combo_projection->addItem(
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

		combo_projection->addItem(
				tr(GPlatesGui::MapProjection::get_display_name(map_projection_type)),
				// Add map projection indices *after* the globe projection indices...
				GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_index);
	}

	// The central_meridian spinbox should be disabled if we're in a globe projection. 
	update_central_meridian_status();

	QObject::connect(
			combo_projection,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(update_central_meridian_status()));

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
		const GPlatesGui::ViewportProjection &viewport_projection)
{
	// Get the projection index of the viewport projection (it's either a globe or map projection).
	unsigned int projection_index;
	if (boost::optional<GPlatesGui::GlobeProjection::Type> globe_projection_type =
		viewport_projection.get_globe_projection_type())
	{
		projection_index = globe_projection_type.get();
	}
	else // map projection...
	{
		boost::optional<GPlatesGui::MapProjection::Type> map_projection_type =
				viewport_projection.get_map_projection_type();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				map_projection_type,
				GPLATES_ASSERTION_SOURCE);

		// Map projection indices come *after* the globe projection indices.
		projection_index = GPlatesGui::GlobeProjection::NUM_PROJECTIONS + map_projection_type.get();
	}

	// Now we can quickly select the appropriate line of the combobox
	// by finding our projection ID (and not worrying about the text
	// label)
	const int idx = combo_projection->findData(projection_index);
	if (idx != -1)
	{
		combo_projection->setCurrentIndex(idx);
	}
}

void
GPlatesQtWidgets::SetProjectionDialog::set_map_central_meridian(
		double central_meridian_)
{
	spin_central_meridian->setValue(central_meridian_);
}

void
GPlatesQtWidgets::SetProjectionDialog::update_central_meridian_status()
{
	// Disable for the globe projections (map projections added to combo box after globe projections).
	spin_central_meridian->setDisabled(
			combo_projection->currentIndex() < GPlatesGui::GlobeProjection::NUM_PROJECTIONS);
}

GPlatesGui::ViewportProjection::projection_type
GPlatesQtWidgets::SetProjectionDialog::get_projection_type() const
{
	// Retrieve the embedded QVariant for the selected combobox choice.
	QVariant projection_qv = combo_projection->itemData(combo_projection->currentIndex());

	// Extract projection index from QVariant.
	const unsigned int projection_index = projection_qv.toUInt();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS + GPlatesGui::MapProjection::NUM_PROJECTIONS,
			GPLATES_ASSERTION_SOURCE);

	// If it's a globe projection.
	if (projection_index < GPlatesGui::GlobeProjection::NUM_PROJECTIONS)
	{
		return GPlatesGui::ViewportProjection::projection_type(
				static_cast<GPlatesGui::GlobeProjection::Type>(projection_index));
	}

	// Map projection (projection index is after all globe projections).
	return GPlatesGui::ViewportProjection::projection_type(
			static_cast<GPlatesGui::MapProjection::Type>(projection_index - GPlatesGui::GlobeProjection::NUM_PROJECTIONS));
}

double
GPlatesQtWidgets::SetProjectionDialog::get_map_central_meridian()
{
	return spin_central_meridian->value();
}
