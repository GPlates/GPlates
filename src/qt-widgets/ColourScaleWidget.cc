/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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

#include <cmath>
#include <utility>
#include <boost/optional.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/foreach.hpp>
#include <QPainter>
#include <QBrush>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QLocale>
#include <QPalette>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QDebug>

#include "ColourScaleWidget.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "QtWidgetUtils.h"

#include "gui/AgeColourPalettes.h"
#include "gui/Colour.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/ColourPaletteVisitor.h"
#include "gui/CptColourPalette.h"
#include "gui/Dialogs.h"


namespace
{
	inline
	double
	get_double_value(
			double d)
	{
		return d;
	}

	inline
	double
	get_double_value(
			GPlatesMaths::Real r)
	{
		return r.dval();
	}

	typedef boost::numeric::converter<
		int,
		double,
		boost::numeric::conversion_traits<int, double>,
		boost::numeric::def_overflow_handler,
		boost::numeric::Ceil< boost::numeric::conversion_traits<int, double>::source_type >
	> round_up;

	typedef boost::numeric::converter<
		int,
		double,
		boost::numeric::conversion_traits<int, double>,
		boost::numeric::def_overflow_handler,
		boost::numeric::Floor< boost::numeric::conversion_traits<int, double>::source_type >
	> round_down;

	/**
	 * Calculates the number that the annotations should be a multiple of.
	 * Returns -1 on error.
	 */
	double
	calculate_annotation_multiplier(
			double range,
			int max_rows)
	{
		try
		{
			// First find the power of 10 that maximises the number of rows that we can
			// display while staying under max_rows.
			double pow_of_10 = std::pow(10.0, round_up::convert(std::log10(range / max_rows)));

			// Try the largest multiple of 2 * 10^k smaller than pow_of_10.
			double test = pow_of_10 / 5;
			if (range / test < max_rows)
			{
				return test;
			}

			// Try the largest multiple of 5 * 10^k smaller than pow_of_10.
			test = pow_of_10 / 2;
			if (range / test < max_rows)
			{
				return test;
			}

			return pow_of_10;
		}
		catch (...)
		{
			return -1;
		}
	}

	template<typename KeyType>
	struct ColourPaletteConverter
	{
		typedef GPlatesGui::StaticCastConverter<KeyType, double> type;
	};

	template<>
	struct ColourPaletteConverter<GPlatesMaths::Real>
	{
		typedef GPlatesGui::RealToBuiltInConverter<double> type;
	};

	class ColourScaleGenerator :
			public boost::static_visitor<bool>
	{
	public:

		explicit
		ColourScaleGenerator(
				QPixmap &colour_scale_pixmap,
				QPixmap &disabled_colour_scale_pixmap,
				GPlatesQtWidgets::ColourScaleWidget::annotations_seq_type &annotations,
				int vertical_margin,
				int font_height,
				const QSize &widget_size) :
			d_colour_scale_pixmap(colour_scale_pixmap),
			d_disabled_colour_scale_pixmap(disabled_colour_scale_pixmap),
			d_annotations(annotations),
			d_vertical_margin(vertical_margin),
			d_font_height(font_height),
			d_widget_size(widget_size)
		{  }

		bool
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			return false;
		}

		template<class ColourPaletteType>
		bool
		operator()(
				const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette) const
		{
			RangeVisitor visitor;
			colour_palette->accept_visitor(visitor);
			if (visitor.was_successful())
			{
				double minimum_value = visitor.get_minimum_value();
				double maximum_value = visitor.get_maximum_value();

				int pixmap_width = GPlatesQtWidgets::ColourScaleWidget::COLOUR_SCALE_WIDTH;
				int pixmap_height = d_widget_size.height() - 2 * d_vertical_margin;

				d_annotations.clear();

				if (pixmap_height >= pixmap_width &&
						!GPlatesMaths::are_almost_exactly_equal(minimum_value, maximum_value))
				{
					if (minimum_value > maximum_value)
					{
						std::swap(minimum_value, maximum_value);
					}

					// Paint a checkerboard as the background of the pixmap first.
					d_colour_scale_pixmap = GPlatesQtWidgets::QtWidgetUtils::create_transparent_checkerboard(
							pixmap_width, pixmap_height, GPlatesQtWidgets::ColourScaleWidget::CHECKERBOARD_GRID_SIZE);
					d_disabled_colour_scale_pixmap = d_colour_scale_pixmap;

					// The colour scale covers the range discovered by the visitor, plus a
					// little bit extra on the top and bottom for the background and foreground
					// colours (for values below the lower bound and above the upper bound,
					// respectively). The height of the background and foreground parts are 10%
					// of the total height of the pixmap, capped at pixmap_width.
					int bf_height = static_cast<int>(pixmap_height * 0.1);
					if (bf_height > pixmap_width)
					{
						bf_height = pixmap_width;
					}

					int range_height = pixmap_height - 2 * bf_height;
					Interpolator interp(
							bf_height,
							maximum_value,
							bf_height + range_height,
							minimum_value);

					QPainter painter(&d_colour_scale_pixmap);
					QPainter disabled_painter(&d_disabled_colour_scale_pixmap);
					QPen pen;
					pen.setWidth(1);
					GPlatesGui::ColourPalette<double>::non_null_ptr_type adapted_colour_palette =
						visitor.get_adapted_colour_palette();
					for (int y = 0; y != pixmap_height; ++y)
					{
						boost::optional<GPlatesGui::Colour> colour = adapted_colour_palette->get_colour(
								interp.get_value_at(y));
						if (colour)
						{
							pen.setColor(QColor(*colour));
							painter.setPen(pen);
							painter.drawLine(0, y, pixmap_width, y);

							GLfloat grey = (colour->red() + colour->green() + colour->blue()) / 3;
							GPlatesGui::Colour disabled_colour(grey, grey, grey, colour->alpha());
							pen.setColor(QColor(disabled_colour));
							disabled_painter.setPen(pen);
							disabled_painter.drawLine(0, y, pixmap_width, y);
						}
					}

					// We annotate the colour scale with numbers. The numbers shown are
					// multiples of 1 * 10^k, 2 * 10^k or 5 * 10^k, for some integer k. We pick
					// the multiple based on the font height, so that they're spaced out nicely.
					int max_rows = pixmap_height / (d_font_height + GPlatesQtWidgets::ColourScaleWidget::ANNOTATION_LINE_SPACING);
					double multiplier = calculate_annotation_multiplier(maximum_value - minimum_value, max_rows);
					if (multiplier > 0)
					{
						int start = round_up::convert(interp.get_value_at(pixmap_height - 1) / multiplier);
						int end = round_down::convert(interp.get_value_at(0) / multiplier) + 1;
						QLocale loc;

						for (int i = start; i != end; ++i)
						{
							double value = i * multiplier;
							d_annotations.push_back(std::make_pair(interp.get_pos(value), loc.toString(value)));
						}
					}
				}
				else
				{
					// Sanity checks.
					if (pixmap_width < 0)
					{
						pixmap_width = 0;
					}
					if (pixmap_height < 0)
					{
						pixmap_height = 0;
					}

					// There's not enough space to do anything sensible, or there's something
					// wrong with the colour scale parameters, so just clear everything.
					d_colour_scale_pixmap = QPixmap(pixmap_width, pixmap_height);
					if (pixmap_width > 0 && pixmap_height > 0)
					{
						QPainter painter(&d_colour_scale_pixmap);
						painter.setPen(QPen(Qt::NoPen));
						painter.fillRect(0, 0, pixmap_width, pixmap_height, QBrush(Qt::white));
					}
					d_disabled_colour_scale_pixmap = d_colour_scale_pixmap;
				}
				return true;
			}
			else
			{
				return false;
			}
		}

	private:

		/**
		 * Extract the range of values covered by a colour palette, which is also
		 * returned, adapted into an integer colour palette.
		 */
		class RangeVisitor :
				public GPlatesGui::ColourPaletteVisitor
		{
		public:

			explicit
			RangeVisitor() :
				d_successful(false),
				d_minimum_value(0.0),
				d_maximum_value(0.0)
			{  }

			bool
			was_successful() const
			{
				return d_successful;
			}

			double
			get_minimum_value() const
			{
				return d_minimum_value;
			}

			double
			get_maximum_value() const
			{
				return d_maximum_value;
			}

			const GPlatesGui::ColourPalette<double>::non_null_ptr_type &
			get_adapted_colour_palette() const
			{
				return *d_adapted_colour_palette;
			}

			virtual
			void
			visit_age_colour_palette(
					GPlatesGui::AgeColourPalette &colour_palette)
			{
				do_visit(colour_palette, colour_palette.get_range());
			}

			virtual
			void
			visit_int32_categorical_cpt_colour_palette(
					GPlatesGui::CategoricalCptColourPalette<boost::int32_t> &colour_palette)
			{
				if (colour_palette.get_range())
				{
					do_visit(colour_palette, colour_palette.get_range().get());
				}
			}

			virtual
			void
			visit_uint32_categorical_cpt_colour_palette(
					GPlatesGui::CategoricalCptColourPalette<boost::uint32_t> &colour_palette)
			{
				if (colour_palette.get_range())
				{
					do_visit(colour_palette, colour_palette.get_range().get());
				}
			}

			virtual
			void
			visit_regular_cpt_colour_palette(
					GPlatesGui::RegularCptColourPalette &colour_palette)
			{
				if (colour_palette.get_range())
				{
					do_visit(colour_palette, colour_palette.get_range().get());
				}
			}

		private:

			template<class ColourPaletteType>
			inline
			void
			do_visit(
					ColourPaletteType &colour_palette,
					const std::pair<
							typename ColourPaletteType::key_type,
							typename ColourPaletteType::key_type> &range)
			{
				typedef typename ColourPaletteType::key_type key_type;
				typedef typename ::ColourPaletteConverter<key_type>::type converter_type;

				d_minimum_value = get_double_value(range.first);
				d_maximum_value = get_double_value(range.second);

				typename ColourPaletteType::non_null_ptr_type colour_palette_ptr(&colour_palette);
				d_adapted_colour_palette = GPlatesGui::convert_colour_palette<
						key_type, double, converter_type>(colour_palette_ptr, converter_type());

				d_successful = true;
			}

			bool d_successful;
			double d_minimum_value;
			double d_maximum_value;
			boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> d_adapted_colour_palette;
		};

		class Interpolator
		{
		public:

			explicit
			Interpolator(
					int top_anchor,
					double top_value,
					int bottom_anchor,
					double bottom_value) :
				d_top_anchor(top_anchor),
				d_top_value(top_value),
				d_bottom_anchor(bottom_anchor),
				d_bottom_value(bottom_value)
			{  }

			double
			get_value_at(
					int pos) const
			{
				double fpos = (pos - d_top_anchor) / static_cast<double>(d_bottom_anchor - d_top_anchor);
				return fpos * (d_bottom_value - d_top_value) + d_top_value;
			}

			int
			get_pos(
					double value) const
			{
				double fpos = (value - d_top_value) / (d_bottom_value - d_top_value);
				return static_cast<int>(fpos * (d_bottom_anchor - d_top_anchor) + d_top_anchor);
			}

		private:

			int d_top_anchor;
			double d_top_value;
			int d_bottom_anchor;
			double d_bottom_value;
		};

		QPixmap &d_colour_scale_pixmap;
		QPixmap &d_disabled_colour_scale_pixmap;
		GPlatesQtWidgets::ColourScaleWidget::annotations_seq_type &d_annotations;

		int d_vertical_margin;
		int d_font_height;
		QSize d_widget_size;
	};


	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	get_file_dialog_filters()
	{
		using namespace GPlatesQtWidgets;

		SaveFileDialog::filter_list_type filters;
		filters.push_back(FileDialogFilter(ColourScaleWidget::tr("PNG image"), "png"));
		filters.push_back(FileDialogFilter(ColourScaleWidget::tr("All files"), "*"));
		return filters;
	}
}


GPlatesQtWidgets::ColourScaleWidget::ColourScaleWidget(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QWidget(parent_),
	d_viewport_window(viewport_window),
	d_curr_colour_palette(GPlatesGui::RasterColourPalette::create()),
	d_save_file_dialog(
			this,
			tr("Save Image As"),
			get_file_dialog_filters(),
			view_state)
{
	setMinimumHeight(MINIMUM_HEIGHT);

	QAction *save_action = new QAction(tr("&Save Image As..."), this);
	d_right_click_actions.push_back(save_action);
}


bool
GPlatesQtWidgets::ColourScaleWidget::populate(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
{
	d_curr_colour_palette = colour_palette;
	return regenerate_contents();
}


void
GPlatesQtWidgets::ColourScaleWidget::paintEvent(
		QPaintEvent *ev)
{
	QPainter painter(this);
	QFontMetrics font_metrics(font());
	int vertical_margin = font_metrics.ascent() / 2;

	// Paint the background.
	QPalette this_palette = palette();
	painter.fillRect(
			0,
			0,
			width(),
			height(),
			QBrush(this_palette.color(QPalette::Window)));

	// Draw the colour scale.
	painter.drawPixmap(
			LEFT_MARGIN,
			vertical_margin,
			isEnabled() ? d_colour_scale_pixmap : d_disabled_colour_scale_pixmap);

	// Draw a border around the colour scale.
	QPen border_pen(Qt::gray);
	border_pen.setWidth(1);
	painter.setPen(border_pen);
	painter.drawRect(
			LEFT_MARGIN - 1,
			vertical_margin - 1,
			d_colour_scale_pixmap.width() + 1,
			d_colour_scale_pixmap.height() + 1);

	// Draw the annotations.
	QPen annotation_pen(this_palette.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
	annotation_pen.setWidth(1);
	BOOST_FOREACH(const annotation_type &annotation, d_annotations)
	{
		int curr_y = annotation.first + vertical_margin;
		painter.setPen(border_pen);
		painter.drawLine(
				LEFT_MARGIN + COLOUR_SCALE_WIDTH,
				curr_y,
				LEFT_MARGIN + COLOUR_SCALE_WIDTH + TICK_LENGTH,
				curr_y);
		painter.setPen(annotation_pen);
		painter.drawText(
				LEFT_MARGIN + COLOUR_SCALE_WIDTH + INTERNAL_SPACING,
				curr_y + font_metrics.ascent() / 2 /* vertically centre text on tick mark */,
				annotation.second);
	}
}


void
GPlatesQtWidgets::ColourScaleWidget::resizeEvent(
		QResizeEvent *ev)
{
	// The colour scale only needs to be regenerated if the height changes - the
	// width of the widget doesn't affect how it looks.
	if (ev->oldSize().height() != height())
	{
		regenerate_contents();
	}

	QWidget::resizeEvent(ev);
}


void
GPlatesQtWidgets::ColourScaleWidget::contextMenuEvent(
		QContextMenuEvent *ev)
{
	QAction *triggered_action = QMenu::exec(d_right_click_actions, ev->globalPos());
	if (triggered_action == d_right_click_actions.first())
	{
		boost::optional<QString> file_name = d_save_file_dialog.get_file_name();
		if (file_name)
		{
			// Grab an image of this widget and save it to disk.
			QPixmap widget_pixmap = QPixmap::grabWidget(this, geometry());
			bool success = widget_pixmap.save(*file_name);
			if (!success)
			{
				QMessageBox::critical(
						&d_viewport_window->dialogs().visual_layers_dialog(),
						tr("Save Image As"),
						tr("GPlates could not save to the chosen file. Please choose another location."));
			}
		}

		ev->accept();
	}
}


bool
GPlatesQtWidgets::ColourScaleWidget::regenerate_contents()
{
	QFontMetrics font_metrics(font());
	int vertical_margin = font_metrics.ascent() / 2;
	int font_height = font_metrics.height();

	ColourScaleGenerator generator(
			d_colour_scale_pixmap,
			d_disabled_colour_scale_pixmap,
			d_annotations,
			vertical_margin,
			font_height,
			size());
	bool result = d_curr_colour_palette->apply_visitor(generator);
	if (result)
	{
		update();
	}

	return result;
}

