/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_MERGERECONSTRUCTIONLAYERSDIALOG_H
#define GPLATES_QTWIDGETS_MERGERECONSTRUCTIONLAYERSDIALOG_H

#include <vector>
#include <boost/weak_ptr.hpp>
#include <QWidget>

#include "MergeReconstructionLayersDialogUi.h"

#include "app-logic/Layer.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class VisualLayer;
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * Dialog to select 'Reconstruction Tree' layers to merge into the current layer.
	 */
	class MergeReconstructionLayersDialog :
			public QDialog,
			protected Ui_MergeReconstructionLayersDialog
	{
		Q_OBJECT

	public:

		explicit
		MergeReconstructionLayersDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		/**
		 * Causes the dialog to be populated with all 'Reconstruction Tree' layers except the current @a visual_layer.
		 * Returns true iff the dialog was successfully populated.
		 */
		bool
		populate(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

	private Q_SLOTS:

		void
		react_clear_all_layers();

		void
		react_select_all_layers();

		void
		react_cell_changed_layers(
				int row,
				int column);

		void
		handle_apply();

		void
		handle_reject();

	private:

		/**
		 * Keeps track of which layers are enabled/disabled by the user.
		 */
		class LayerState
		{
		public:
			//! Layers are enabled by default - user will need to disable them.
			explicit
			LayerState(
					const GPlatesAppLogic::Layer &layer_) :
				layer(layer_),
				enabled(true)
			{  }

			GPlatesAppLogic::Layer layer;
			bool enabled;
		};
		typedef std::vector<LayerState> layer_state_seq_type;

		/**
		 * These should match the table columns set up in the UI designer.
		 */
		enum LayerColumnName
		{
			LAYER_NAME_COLUMN,
			ENABLE_LAYER_COLUMN
		};


		void
		setup_connections();

		void
		clear_layers();

		std::vector<GPlatesAppLogic::Layer>
		get_selected_layers() const;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * The visual layer for which we are currently merging other 'Reconstruction Tree' layers into.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

		/**
		 * Keeps track of which layers are enabled by the user in the GUI.
		 */
		layer_state_seq_type d_layer_state_seq;
	};
}

#endif  // GPLATES_QTWIDGETS_MERGERECONSTRUCTIONLAYERSDIALOG_H
