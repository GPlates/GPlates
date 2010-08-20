/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_GEOREFERENCINGPAGE_H
#define GPLATES_QTWIDGETS_GEOREFERENCINGPAGE_H

#include "GeoreferencingPageUi.h"

#include "property-values/Georeferencing.h"


namespace GPlatesQtWidgets
{
	// Forward declaration.
	class EditAffineTransformGeoreferencingWidget;
	class TimeDependentRasterSequence;

	class GeoreferencingPage: 
			public QWizardPage,
			protected Ui_GeoreferencingPage
	{
		Q_OBJECT
		
	public:

		explicit
		GeoreferencingPage(
				GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
				TimeDependentRasterSequence &raster_sequence,
				QWidget *parent_ = NULL);

		virtual
		void
		initializePage();

		virtual
		bool
		isComplete() const;

	private slots:

		void
		handle_warning_visible_changed(
				bool visible);

	private:

		GPlatesPropertyValues::Georeferencing::non_null_ptr_type &d_georeferencing;
		EditAffineTransformGeoreferencingWidget *d_georeferencing_widget;
		TimeDependentRasterSequence &d_raster_sequence;
		unsigned int d_last_seen_raster_width;
		unsigned int d_last_seen_raster_height;
		bool d_is_complete;
	};
}

#endif  // GPLATES_QTWIDGETS_GEOREFERENCINGPAGE_H
