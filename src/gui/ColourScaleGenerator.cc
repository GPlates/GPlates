/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/optional.hpp>
#include <QPainter>
#include <QBrush>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QLocale>
#include <QPalette>
#include <QDebug>

#include "ColourScaleGenerator.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/AgeColourPalettes.h"
#include "gui/Colour.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/ColourPaletteVisitor.h"
#include "gui/CptColourPalette.h"
#include "gui/Dialogs.h"

#include "qt-widgets/QtWidgetUtils.h"


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
	calculate_linear_annotation_multiplier(
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


	template <typename InterpolatorType>
	void
	fill_colour_scale(
			QPainter &painter,
			QPainter &disabled_painter,
			const InterpolatorType &interpolator,
			const GPlatesGui::ColourPalette<double>::non_null_ptr_type &adapted_colour_palette,
			int pixmap_width,
			int pixmap_height)
	{
		QPen pen;
		pen.setWidth(1);

		for (int y = 0; y != pixmap_height; ++y)
		{
			boost::optional<GPlatesGui::Colour> colour = adapted_colour_palette->get_colour(
					interpolator.get_value_at(y));
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

		/**
		 * Grid size of transparent checkerboard pattern.
		 */
		static const int CHECKERBOARD_GRID_SIZE = 8;


		explicit
		ColourScaleGenerator(
				QPixmap &colour_scale_pixmap,
				QPixmap &disabled_colour_scale_pixmap,
				int pixmap_width,
				int pixmap_height,
				boost::optional<double> use_log_scale,
				boost::optional<GPlatesGui::ColourScale::Annotations> annotations) :
			d_colour_scale_pixmap(colour_scale_pixmap),
			d_disabled_colour_scale_pixmap(disabled_colour_scale_pixmap),
			d_pixmap_width(pixmap_width),
			d_pixmap_height(pixmap_height),
			d_use_log_scale(use_log_scale),
			d_annotations(annotations)
		{  }

		bool
		operator()(
				const GPlatesGui::RasterColourPalette::empty &)
		{
			return false;
		}

		template<class ColourPaletteType>
		bool
		operator()(
				const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette)
		{
			RangeVisitor visitor;
			colour_palette->accept_visitor(visitor);
			if (visitor.was_successful())
			{
				double minimum_value = visitor.get_minimum_value();
				double maximum_value = visitor.get_maximum_value();

				if (d_annotations)
				{
					d_annotations->annotations.clear();
				}

				if (minimum_value > maximum_value)
				{
					std::swap(minimum_value, maximum_value);
				}

				// Make sure not exactly the same...
				// NOTE: Not using 'GPlatesMaths::are_almost_exactly_equal()') since some values
				// may be *very* small and it would compare then equal.
				if (maximum_value > minimum_value)
				{
					// Paint a checkerboard as the background of the pixmap first.
					d_colour_scale_pixmap = GPlatesQtWidgets::QtWidgetUtils::create_transparent_checkerboard(
							d_pixmap_width, d_pixmap_height, CHECKERBOARD_GRID_SIZE);
					d_disabled_colour_scale_pixmap = d_colour_scale_pixmap;

					// The colour scale covers the range discovered by the visitor, plus a
					// little bit extra on the top and bottom for the background and foreground
					// colours (for values below the lower bound and above the upper bound,
					// respectively). The height of the background and foreground parts are 10%
					// of the total height of the pixmap, capped at d_pixmap_width.
					int bf_height = static_cast<int>(d_pixmap_height * 0.1);
					if (bf_height > d_pixmap_width)
					{
						bf_height = d_pixmap_width;
					}
					int range_height = d_pixmap_height - 2 * bf_height;

					QPainter painter(&d_colour_scale_pixmap);
					QPainter disabled_painter(&d_disabled_colour_scale_pixmap);
					GPlatesGui::ColourPalette<double>::non_null_ptr_type adapted_colour_palette =
							visitor.get_adapted_colour_palette();
					QLocale loc;

					if (d_use_log_scale)
					{
						LogInterpolator interp(bf_height, maximum_value, bf_height + range_height, minimum_value, d_use_log_scale.get());
						fill_colour_scale(painter, disabled_painter, interp, adapted_colour_palette, d_pixmap_width, d_pixmap_height);

						if (d_annotations)
						{
							if (maximum_value >= 0 &&
								minimum_value <= 0)
							{
								// The min/max range includes zero so we need to handle that specially since
								// we cannot take log of negative numbers.

								// Make sure we annotate zero so user knows where the crossover occurs.
								const int zero_value_pos = interp.get_pos(0.0);
								d_annotations->annotations.push_back(std::make_pair(zero_value_pos, loc.toString(0.0)));

								// Annotate some positive values.
								const int max_value_pos = interp.get_pos(maximum_value);
								const int num_pos_rows = (zero_value_pos - max_value_pos) / d_annotations->annotation_height;
								for (int p = 1; p <= num_pos_rows; ++p)
								{
									const int row_pos = zero_value_pos - p * d_annotations->annotation_height;
									const double value = interp.get_value_at(row_pos);
									d_annotations->annotations.push_back(std::make_pair(row_pos, loc.toString(value, 'e', 1)));
								}

								// Annotate some negative values.
								const int min_value_pos = interp.get_pos(minimum_value);
								const int num_neg_rows = (min_value_pos - zero_value_pos) / d_annotations->annotation_height;
								for (int n = 1; n <= num_neg_rows; ++n)
								{
									const int row_pos = zero_value_pos + n * d_annotations->annotation_height;
									const double value = interp.get_value_at(row_pos);
									d_annotations->annotations.push_back(std::make_pair(row_pos, loc.toString(value, 'e', 1)));
								}
							}
							else // maximum_value < 0 || minimum_value > 0 ...
							{
								// The min/max values is either both positive or both negative,
								// so we're not crossing zero.
								const int min_value_pos = interp.get_pos(minimum_value);
								const int max_value_pos = interp.get_pos(maximum_value);
								const int num_rows = (min_value_pos - max_value_pos) / d_annotations->annotation_height;
								
								if (maximum_value < 0)
								{
									for (int n = 0; n <= num_rows; ++n)
									{
										// Start at the lowest absolute value so the user knows exactly what
										// colour it maps to.
										const int row_pos = max_value_pos + n * d_annotations->annotation_height;
										const double value = interp.get_value_at(row_pos);
										d_annotations->annotations.push_back(std::make_pair(row_pos, loc.toString(value, 'e', 1)));
									}
								}
								else // minimum_value > 0 ...
								{
									for (int n = 0; n <= num_rows; ++n)
									{
										// Start at the lowest absolute value so the user knows exactly what
										// colour it maps to.
										const int row_pos = min_value_pos - n * d_annotations->annotation_height;
										const double value = interp.get_value_at(row_pos);
										d_annotations->annotations.push_back(std::make_pair(row_pos, loc.toString(value, 'e', 1)));
									}
								}
							}
						}
					}
					else
					{
						LinearInterpolator interp(bf_height, maximum_value, bf_height + range_height, minimum_value);
						fill_colour_scale(painter, disabled_painter, interp, adapted_colour_palette, d_pixmap_width, d_pixmap_height);

						if (d_annotations)
						{
							// We annotate the colour scale with numbers. The numbers shown are
							// multiples of 1 * 10^k, 2 * 10^k or 5 * 10^k, for some integer k. We pick
							// the multiple based on the font height, so that they're spaced out nicely.
							int max_rows = d_pixmap_height / d_annotations->annotation_height;
							double multiplier = calculate_linear_annotation_multiplier(maximum_value - minimum_value, max_rows);
							if (multiplier > 0)
							{
								int start = round_up::convert(interp.get_value_at(d_pixmap_height - 1) / multiplier);
								int end = round_down::convert(interp.get_value_at(0) / multiplier) + 1;

								for (int i = start; i != end; ++i)
								{
									double value = i * multiplier;
									d_annotations->annotations.push_back(std::make_pair(interp.get_pos(value), loc.toString(value)));
								}
							}
						}
					}
				}
				else
				{
					// Sanity checks.
					if (d_pixmap_width < 0)
					{
						d_pixmap_width = 0;
					}
					if (d_pixmap_height < 0)
					{
						d_pixmap_height = 0;
					}

					// There's not enough space to do anything sensible, or there's something
					// wrong with the colour scale parameters, so just clear everything.
					d_colour_scale_pixmap = QPixmap(d_pixmap_width, d_pixmap_height);
					if (d_pixmap_width > 0 && d_pixmap_height > 0)
					{
						QPainter painter(&d_colour_scale_pixmap);
						painter.setPen(QPen(Qt::NoPen));
						painter.fillRect(0, 0, d_pixmap_width, d_pixmap_height, QBrush(Qt::white));
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

		/**
		 * Interpolate linearly.
		 */
		class LinearInterpolator
		{
		public:

			explicit
			LinearInterpolator(
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

		/**
		 * Interpolate such that colours are uniformly spaced in log space.
		 */
		class LogInterpolator
		{
		public:

			explicit
			LogInterpolator(
					int maximum_anchor,
					double maximum_value,
					int minimum_anchor,
					double minimum_value,
					double log_deviation_towards_zero) :
				d_maximum_anchor(maximum_anchor),
				d_maximum_value(maximum_value),
				d_minimum_anchor(minimum_anchor),
				d_minimum_value(minimum_value)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_maximum_value > d_minimum_value,
						GPLATES_ASSERTION_SOURCE);

				// The min/max range includes zero so we need to handle that specially since we cannot
				// take the log of negative numbers.
				if (d_maximum_value >= 0 &&
					d_minimum_value <= 0)
				{
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							log_deviation_towards_zero > 0,
							GPLATES_ASSERTION_SOURCE);

					ZeroInfo zero_info;

					if (d_maximum_value > 0 &&
						d_minimum_value < 0)
					{
						// There's a non-zero positive and negative range then place the zero crossing position
						// such that log spacing on both sides changes at the same rate (relative to position).

						const double log_min_max_diff = std::fabs(std::log10(d_maximum_value) - std::log10(-d_minimum_value));

						// Positive range is from 'log(max_value)' to 'log(max_value) - pos_log_deviation'.
						// Negative range is from 'log(-min_value)' to 'log(-min_value) - neg_log_deviation'.
						if (d_maximum_value > -d_minimum_value)
						{
							zero_info.pos_log_deviation_towards_zero = log_deviation_towards_zero + log_min_max_diff;
							zero_info.neg_log_deviation_towards_zero = log_deviation_towards_zero;
						}
						else
						{
							zero_info.pos_log_deviation_towards_zero = log_deviation_towards_zero;
							zero_info.neg_log_deviation_towards_zero = log_deviation_towards_zero + log_min_max_diff;
						}

						// Determine the zero position.
						double fpos = zero_info.pos_log_deviation_towards_zero /
								(zero_info.pos_log_deviation_towards_zero + zero_info.neg_log_deviation_towards_zero);
						zero_info.zero_pos = static_cast<int>(fpos * (d_minimum_anchor - d_maximum_anchor) + d_maximum_anchor + 0.5);

						// Correct the log deviation values to account for the zero position getting truncated to an integer.
						fpos = (zero_info.zero_pos - d_maximum_anchor) / static_cast<double>(d_minimum_anchor - d_maximum_anchor);
						if (d_maximum_value > -d_minimum_value)
						{
							if (fpos > 0.5 && fpos < 1.0)
							{
								log_deviation_towards_zero = (fpos - 1) * log_min_max_diff / (1 - 2 * fpos);
								zero_info.pos_log_deviation_towards_zero = log_deviation_towards_zero + log_min_max_diff;
								zero_info.neg_log_deviation_towards_zero = log_deviation_towards_zero;
							}
						}
						else
						{
							if (fpos > 0.0 && fpos < 0.5)
							{
								log_deviation_towards_zero = fpos * log_min_max_diff / (1 - 2 * fpos);
								zero_info.pos_log_deviation_towards_zero = log_deviation_towards_zero;
								zero_info.neg_log_deviation_towards_zero = log_deviation_towards_zero + log_min_max_diff;
							}
						}
					}
					else if (d_maximum_value > 0)
					{
						// 'd_minimum_value' must be zero which places it at the minimum anchor.
						zero_info.zero_pos = d_minimum_anchor;
						zero_info.pos_log_deviation_towards_zero = log_deviation_towards_zero;
						zero_info.neg_log_deviation_towards_zero = 0;
					}
					else // d_minimum_value < 0 ...
					{
						// 'd_maximum_value' must be zero which places it at the maximum anchor.
						zero_info.zero_pos = d_maximum_anchor;
						zero_info.pos_log_deviation_towards_zero = 0;
						zero_info.neg_log_deviation_towards_zero = log_deviation_towards_zero;
					}

					d_zero_info = zero_info;
				}
			}

			double
			get_value_at(
					int pos) const
			{
				if (d_zero_info) // there's a zero crossing
				{
					if (pos < d_zero_info->zero_pos) // position is positive
					{
						if (d_zero_info->zero_pos == d_maximum_anchor)
						{
							// The maximum value is zero so we can't go positive - so clamp to zero.
							return 0.0;
						}

						// Find ratio of position between max anchor and zero position.
						const double fpos = (pos - d_maximum_anchor) /
								static_cast<double>(d_zero_info->zero_pos - d_maximum_anchor);

						return std::pow(10.0,
								// Ratio determines how much deviation from max value...
								std::log10(d_maximum_value) - d_zero_info->pos_log_deviation_towards_zero * fpos);
					}
					else if (pos > d_zero_info->zero_pos) // position is negative
					{
						if (d_zero_info->zero_pos == d_minimum_anchor)
						{
							// The minimum value is zero so we can't go negative - so clamp to zero.
							return 0.0;
						}

						// Find ratio of position between zero position and min anchor.
						const double fpos = (pos - d_zero_info->zero_pos) /
								static_cast<double>(d_minimum_anchor - d_zero_info->zero_pos);

						return -std::pow(10.0,
								// Ratio determines how much deviation from min value...
								std::log10(-d_minimum_value) - d_zero_info->neg_log_deviation_towards_zero * (1.0 - fpos));
					}
					else
					{
						return 0.0;
					}
				}
				else // no zero crossing...
				{
					const double fpos = (pos - d_maximum_anchor) /
							static_cast<double>(d_minimum_anchor - d_maximum_anchor);

					if (d_maximum_value < 0)
					{
						// Min/max range is negative.
						// Interpolate between log(-min) and log(-max).
						return -std::pow(10.0,
								std::log10(-d_minimum_value) * fpos + std::log10(-d_maximum_value) * (1.0 - fpos));
					}
					else // d_minimum_value > 0 ...
					{
						// Min/max range is positive.
						// Interpolate between log(min) and log(max).
						return std::pow(10.0,
								std::log10(d_minimum_value) * fpos + std::log10(d_maximum_value) * (1.0 - fpos));
					}
				}
			}

			int
			get_pos(
					double value) const
			{
				if (d_zero_info) // there's a zero crossing
				{
					if (value > 0)
					{
						if (d_zero_info->zero_pos == d_maximum_anchor)
						{
							// The maximum value is zero so we can't go positive - so clamp to zero position.
							return d_zero_info->zero_pos;
						}

						const double fpos = (std::log10(d_maximum_value) - std::log10(value)) /
								d_zero_info->pos_log_deviation_towards_zero;
						return static_cast<int>(fpos * (d_zero_info->zero_pos - d_maximum_anchor) + d_maximum_anchor);
					}
					else if (value < 0)
					{
						if (d_zero_info->zero_pos == d_minimum_anchor)
						{
							// The minimum value is zero so we can't go negative - so clamp to zero position.
							return d_zero_info->zero_pos;
						}

						const double fpos = 1.0 - (std::log10(-d_minimum_value) - std::log10(-value)) /
								d_zero_info->neg_log_deviation_towards_zero;
						return static_cast<int>(fpos * (d_minimum_anchor - d_zero_info->zero_pos) + d_zero_info->zero_pos);
					}
					else
					{
						return d_zero_info->zero_pos;
					}
				}
				else if (d_maximum_value < 0)
				{
					// Cannot specify a non-negative value when the min/max range is negative.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							value < 0,
							GPLATES_ASSERTION_SOURCE);

					// Min/max range is negative.
					// Interpolate between log(-min) and log(-max).
					const double fpos = (std::log10(-value) - std::log10(-d_maximum_value)) /
							(std::log10(-d_minimum_value) - std::log10(-d_maximum_value));
					return static_cast<int>(fpos * (d_minimum_anchor - d_maximum_anchor) + d_maximum_anchor);
				}
				else // d_minimum_value > 0 ...
				{
					// Cannot specify a non-positive value when the min/max range is positive.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							value > 0,
							GPLATES_ASSERTION_SOURCE);

					// Min/max range is positive.
					// Interpolate between log(min) and log(max).
					const double fpos = (std::log10(value) - std::log10(d_maximum_value)) /
							(std::log10(d_minimum_value) - std::log10(d_maximum_value));
					return static_cast<int>(fpos * (d_minimum_anchor - d_maximum_anchor) + d_maximum_anchor);
				}
			}

		private:

			int d_maximum_anchor;
			double d_maximum_value;

			int d_minimum_anchor;
			double d_minimum_value;


			struct ZeroInfo
			{
				int zero_pos;
				double pos_log_deviation_towards_zero;
				double neg_log_deviation_towards_zero;
			};

			boost::optional<ZeroInfo> d_zero_info;
		};

		QPixmap &d_colour_scale_pixmap;
		QPixmap &d_disabled_colour_scale_pixmap;
		int d_pixmap_width;
		int d_pixmap_height;

		boost::optional<double> d_use_log_scale;

		boost::optional<GPlatesGui::ColourScale::Annotations> d_annotations;
	};
}


bool
GPlatesGui::ColourScale::generate(
		const RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
		QPixmap &colour_scale_pixmap,
		QPixmap &disabled_colour_scale_pixmap,
		int pixmap_width,
		int pixmap_height,
		boost::optional<double> use_log_scale,
		boost::optional<Annotations> annotations)
{
	ColourScaleGenerator generator(
			colour_scale_pixmap,
			disabled_colour_scale_pixmap,
			pixmap_width,
			pixmap_height,
			use_log_scale,
			annotations);
	return colour_palette->apply_visitor(generator);
}
