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
 
#ifndef GPLATES_QTWIDGETS_COLOURINGDIALOG_H
#define GPLATES_QTWIDGETS_COLOURINGDIALOG_H

#include <boost/shared_ptr.hpp>
#include <QIcon>
#include <QColor>
#include <QString>
#include <QStringList>

#include "ui_ColouringDialogUi.h"

#include "GPlatesDialog.h"
#include "OpenFileDialog.h"

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "gui/ColourSchemeDelegator.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureStoreRootHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class ColourSchemeContainer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class ReadErrorAccumulationDialog;

	class ColouringDialog : 
			public GPlatesDialog, 
			protected Ui_ColouringDialog 
	{
		Q_OBJECT

	public:
		friend class DrawStyleDialog;
		/**
		 * Constructs a ColouringDialog. Clones @a existing_globe_canvas for the
		 * previews.
		 */
		ColouringDialog(
			GPlatesPresentation::ViewState &view_state,
			const GlobeAndMapWidget &existing_globe_and_map_widget,
			ReadErrorAccumulationDialog &read_error_accumulation_dialog,
			QWidget* parent_ = NULL);

	private Q_SLOTS:

		void
		handle_close_button_clicked(
				bool checked);

		void
		handle_open_button_clicked(
				bool checked);

		void
		handle_add_button_clicked(
				bool checked);

		void
		handle_remove_button_clicked(
				bool checked);

		void
		handle_categories_table_cell_changed(
				int current_row,
				int current_column,
				int previous_row,
				int previous_column);

		void
		handle_main_repaint(
				bool mouse_down);

		void
		handle_repaint(
				bool mouse_down);

		void
		handle_colour_schemes_list_selection_changed();


		void
		handle_show_thumbnails_changed(
				int state);

		void
		handle_feature_collections_combobox_index_changed(
				int index);

		void
		handle_use_global_changed(
				int state);

		void
		edit_current_colour_scheme();

		void
		handle_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		void
		handle_file_info_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

	private:

		/**
		 * This is a special colour scheme that wraps around the ViewState's copy of
		 * ColourSchemeDelegator (i.e. the one used by the main window), and allows the
		 * colour scheme for a particular feature collection (or globally) to be
		 * changed in order to render a preview without affecting the underlying
		 * ColourSchemeDelegator.
		 */
		class PreviewColourScheme :
				public GPlatesGui::ColourScheme
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<PreviewColourScheme> non_null_ptr_type;

			typedef GPlatesUtils::non_null_intrusive_ptr<const PreviewColourScheme> non_null_ptr_to_const_type;
			
			PreviewColourScheme(
					GPlatesGui::ColourSchemeDelegator::non_null_ptr_type view_state_colour_scheme_delegator);

			void
			set_preview_colour_scheme(
					GPlatesGui::ColourScheme::non_null_ptr_type preview_colour_scheme,
					GPlatesModel::FeatureCollectionHandle::const_weak_ref altered_feature_collection =
						GPlatesModel::FeatureCollectionHandle::const_weak_ref());

			virtual
			boost::optional<GPlatesGui::Colour>
			get_colour(
					const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

			boost::optional<GPlatesGui::Colour>
			get_colour(
					const GPlatesModel::FeatureHandle& feature) const
			{
				//TODO:
				return boost::none;
			}

		private:

			/**
			 * The ViewState's ColourSchemeDelegator that we wrap around.
			 */
			GPlatesGui::ColourSchemeDelegator::non_null_ptr_type d_view_state_colour_scheme_delegator;

			/**
			 * The feature collection for which we'll use a different colour scheme, for
			 * preview purposes.
			 *
			 * If this weak-ref is invalid, then we're previewing global colour schemes.
			 */
			GPlatesModel::FeatureCollectionHandle::const_weak_ref d_altered_feature_collection;

			/**
			 * The colour scheme that we're previewing.
			 */
			boost::optional<GPlatesGui::ColourScheme::non_null_ptr_type> d_preview_colour_scheme;

		}; // class PreviewColourScheme

		void
		reposition();

		void
		make_signal_slot_connections();

		void
		populate_colour_scheme_categories();

		void
		populate_feature_collections();

		void
		load_category(
				GPlatesGui::ColourSchemeCategory::Type category,
				int id_to_select = -1);

		/**
		 * Kicks off the rendering loop.
		 *
		 * Because we can't do off-screen rendering at the moment, the rendering of
		 * icons is a little convoluted.
		 *
		 * The colour scheme for the first icon is set. Then, the GlobeAndMapWidget
		 * that we use for rendering is made visible; after it does its repaint, its
		 * frame buffer is grabbed and used for the first icon. The colour scheme for
		 * the next icon is set, and after the next repaint, the frame buffer is
		 * again grabbed and used for the second icon. That is to say, one icon is
		 * processed in one call of handle_repaint(). This continues until all the
		 * icons have been processed.
		 */
		void
		start_rendering_from(
				int list_index);

		/**
		 * Modifies d_preview_colour_scheme based on the colour scheme index stored
		 * associated with the @a item.
		 */
		void
		load_colour_scheme_from(
				QListWidgetItem *item);

		void
		open_cpt_files(
				const QStringList &file_list);

		template<class CptReaderType, typename PropertyExtractorType>
		void
		open_cpt_files(
				const QStringList &file_list,
				const PropertyExtractorType &property_extractor);

		void
		add_single_colour();

		void
		insert_list_widget_item(
				const GPlatesGui::ColourSchemeInfo &colour_scheme_info,
				GPlatesGui::ColourSchemeContainer::id_type id);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * colouring dialog.
		 */
		void
		dragEnterEvent(
				QDragEnterEvent *ev);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * colouring dialog.
		 */
		void
		dropEvent(
				QDropEvent *ev);

		/**
		 * Used for creating feature-age colour schemes.
		 */
		GPlatesAppLogic::ApplicationState &d_application_state;

		/**
		 * An existing GlobeAndMapWidget from which we'll clone our own
		 * GlobeAndMapWidget.
		 *
		 * We hold onto this pointer because we hook into the repaint events of this
		 * widget. When this widget does a non-intermediate repaint (which we take to
		 * be a repaint when the mouse is not pressed), we get our clone to repaint.
		 */
		const GlobeAndMapWidget *d_existing_globe_and_map_widget_ptr;

		/**
		 * The dialog that shows file read errors.
		 */
		ReadErrorAccumulationDialog *d_read_error_accumulation_dialog_ptr;

		/**
		 * Contains all loaded colour schemes, sorted by category.
		 */
		GPlatesGui::ColourSchemeContainer &d_colour_scheme_container;

		/**
		 * The colour scheme delegator in the ViewState used to control colouring in
		 * the main window.
		 */
		GPlatesGui::ColourSchemeDelegator::non_null_ptr_type d_view_state_colour_scheme_delegator;

		/**
		 * The colour scheme used to control the colour of our previewing GlobeAndMapWidget.
		 */
		PreviewColourScheme::non_null_ptr_type d_preview_colour_scheme;

		/**
		 * The widget that does the rendering of the colour scheme previews.
		 *
		 * Because it's quite expensive (in terms of memory) to create, we create just
		 * one of these, and use it to do all the rendering. After rendering a preview
		 * of a particular colour scheme, the frame buffer is copied into a QIcon for
		 * use in the main list widget, and then we use the same GlobeAndMapWidget to
		 * render the next preview.
		 *
		 * FIXME: We currently don't do off-screen rendering, and the only way to get
		 * the scene to be repainted is to have this widget visible. In fact, it's
		 * moved mostly off-screen so that only one pixel remains showing - very hacky.
		 */
		GlobeAndMapWidget *d_globe_and_map_widget_ptr;

		/**
		 * A weak-ref to the feature store root, so we can find out about new feature collections.
		 */
		GPlatesModel::FeatureStoreRootHandle::const_weak_ref d_feature_store_root;

		/**
		 * A blank icon for use as a placeholder icon.
		 */
		QIcon d_blank_icon;

		/**
		 * Stores the current colour scheme category displayed.
		 */
		GPlatesGui::ColourSchemeCategory::Type d_current_colour_scheme_category;

		/**
		 * Stores the index of the next icon in the colour schemes list to render, so
		 * that handle_repaint() can keep track of where it got up to (it does one
		 * icon in one pass).
		 */
		int d_next_icon_to_render;

		/**
		 * If true, this dialog will show thumbnail previews of colour schemes.
		 */
		bool d_show_thumbnails;

		/**
		 * A hack to prevent unwanted refreshes from happening.
		 */
		bool d_suppress_next_repaint;

		/**
		 * A weak reference to the feature collection for which we are currently
		 * viewing the colour scheme. If it is invalid, we are viewing the global
		 * colour scheme.
		 */
		GPlatesModel::FeatureCollectionHandle::const_weak_ref d_current_feature_collection;

		/**
		 * The last colour added to the Single Colour category.
		 */
		QColor d_last_single_colour;

		OpenFileDialog d_open_regular_cpt_files_dialog;
		OpenFileDialog d_open_categorical_cpt_files_dialog;
		OpenFileDialog d_open_any_cpt_files_dialog;

		/**
		 * The height and width of a preview icon.
		 */
		static const int ICON_SIZE = 145;

		/**
		 * The space between items in the colour schemes list widget.
		 */
		static const int SPACING = 10;
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURINGDIALOG_H
