/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_DOCUMENT_H
#define GPLATES_GUI_DOCUMENT_H

// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
#include <boost/python.hpp>

#include <QtCore/QTimer>
#include "DocumentUi.h"
#include "GlobeCanvas.h"
#include "model/Model.h"

namespace GPlatesGui
{
	class Document : 
			public QMainWindow, 
			protected Ui_MainWindow 
	{
		Q_OBJECT
		
	public:
		Document();

	private:
		GlobeCanvas *d_canvas_ptr;
		GPlatesModel::Model *d_model_ptr;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_isochrons;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_total_recon_seqs;
		QTimer *d_timer_ptr;
		double d_time;
		
	private slots:
		void 
		selection_handler(
				std::vector< GlobeCanvas::line_header_type > &items);
				
		void
		mouse_click_handler();
				
		void
		animation_step();
	};
}

#endif

