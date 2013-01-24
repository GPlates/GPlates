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

#ifndef GPLATES_GUI_VISUALLAYERSLISTMODEL_H
#define GPLATES_GUI_VISUALLAYERSLISTMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QStringList>


namespace GPlatesGui
{
	class VisualLayersProxy;

	/**
	 * The VisualLayersListModel is a model that represents the ordering of visual
	 * layers that can be viewed and modified in a list view in the GUI.
	 *
	 * It is a thin wrapper around the ordering, stored in the visual layers;
	 * it adapts the visual layers interface for use by Qt.
	 *
	 * This model is not editable, except for drag and drop. Layers must be added
	 * and removed via ReconstructGraph, not through this model. Any changes made via
	 * ReconstructGraph are propagated through to this model, which then emits signals
	 * to its own subscribers in turn. Drag and drop uses a custom mime type,
	 * defined as VISUAL_LAYERS_MIME_TYPE.
	 */
	class VisualLayersListModel :
			public QAbstractListModel
	{
		Q_OBJECT

	public:

		/**
		 * We need to define our own MIME type, otherwise users will be able to do silly
		 * things like drag from the visual layers list into another application.
		 */
		static const QString VISUAL_LAYERS_MIME_TYPE;

		VisualLayersListModel(
				VisualLayersProxy &visual_layers,
				QObject *parent_ = NULL);

		virtual
		Qt::ItemFlags
		flags(
				const QModelIndex &index_) const;

		virtual
		QVariant
		data(
				const QModelIndex &index_,
				int role = Qt::DisplayRole) const;

		virtual
		int
		rowCount(
				const QModelIndex &parent_ = QModelIndex()) const;

		virtual
		Qt::DropActions
		supportedDropActions() const;

		virtual
		QStringList
		mimeTypes() const;

#if 0
		QMimeData *
		mimeData(
				const QModelIndexList &indices) const;
#endif

		virtual
		bool
		dropMimeData(
				const QMimeData *mime_data,
				Qt::DropAction action,
				int row,
				int column,
				const QModelIndex &parent_);

	private Q_SLOTS:

		void
		handle_visual_layers_order_changed(
				size_t first_row,
				size_t last_row);

		void
		handle_visual_layer_about_to_be_added(
				size_t row);

		void
		handle_visual_layer_added(
				size_t row);

		void
		handle_visual_layer_about_to_be_removed(
				size_t row);

		void
		handle_visual_layer_removed(
				size_t row);

		void
		handle_visual_layer_modified(
				size_t row);

	private:

		void
		make_signal_slot_connections();

		VisualLayersProxy &d_visual_layers;
	};
}

#endif // GPLATES_GUI_VISUALLAYERSLISTMODEL_H
