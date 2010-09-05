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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSDELEGATE_H
#define GPLATES_QTWIDGETS_VISUALLAYERSDELEGATE_H

#include <map>
#include <boost/weak_ptr.hpp>
#include <QItemDelegate>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class VisualLayersProxy;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class ReadErrorAccumulationDialog;
	class VisualLayerWidget;

	/**
	 * VisualLayersDelegate provides display and editing facilities for the model
	 * underlying the VisualLayersWidget.
	 */
	class VisualLayersDelegate :
			public QItemDelegate
	{
		Q_OBJECT

	public:

		VisualLayersDelegate(
				GPlatesGui::VisualLayersProxy &visual_layers,
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ReadErrorAccumulationDialog *read_errors_dialog,
				QObject *parent_ = NULL);

		virtual
		QSize
		sizeHint(
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		virtual
		QWidget *
		createEditor(
				QWidget *parent_,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		virtual
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &index) const;

	private slots:

		void
		handle_layer_about_to_be_removed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	private:

		void
		make_signal_slot_connections();

		void
		emit_size_hint_changed(
				const QModelIndex &index) const;
		
		void
		emit_size_hint_changed(
				const QModelIndex &index);

		/**
		 * Typedef for map that remembers which edit widget is currently displaying
		 * the contents of a particular VisualLayer.
		 */
		typedef std::map<
			boost::weak_ptr<GPlatesPresentation::VisualLayer>,
			VisualLayerWidget *
		> editor_ptr_map_type;

		GPlatesGui::VisualLayersProxy &d_visual_layers;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ReadErrorAccumulationDialog *d_read_errors_dialog;

		mutable editor_ptr_map_type d_editor_ptrs;
	};
}


#endif	// GPLATES_QTWIDGETS_VISUALLAYERSDELEGATE_H
