/* $Id$ */

/**
 * \file Various formats used in @a ExportTemplateFilenameSequence.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEFORMATS_H
#define GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEFORMATS_H

#include <cstddef>
#include <boost/mpl/vector.hpp>
#include <boost/optional.hpp>
#include <QDateTime>
#include <QString>
#include <QRegExp>

#include "global/AssertionFailureException.h"
#include "model/types.h"


namespace GPlatesFileIO
{
	namespace ExportTemplateFilename
	{
		// Forward declarations of all @a Format types.
		class DateTimeFormat;
		class DefaultReconstructionTreeLayerNameFormat;
		class FrameNumberFormat;
		class PercentCharacterFormat;
		class PlaceholderFormat;
		class ReconstructionAnchorPlateIdFormat;
		class ReconstructionTimePrintfFormat;

		/**
		 * Add all @a Format types here.
		 *
		 * When searching for a matching format the sequence order below is followed
		 * until a match is found.
		 */
		typedef boost::mpl::vector<
				PercentCharacterFormat,
				PlaceholderFormat,
				ReconstructionAnchorPlateIdFormat,
				DefaultReconstructionTreeLayerNameFormat,
				FrameNumberFormat,
				DateTimeFormat,
				// NOTE: Extract the printf-style format last in case we mistakenly add
				// a new format that overlaps with printf-style formatting.
				ReconstructionTimePrintfFormat
		> format_types;


		/**
		 * Abstract base class for different types of format used in the
		 * template filename.
		 */
		class Format
		{
		public:
			/*
			 * NOTE: each derived class must have a static method that matches the following
			 * signature:
			 *
			 *   Returns true if the start of @a rest_of_filename_template matches
			 *   the format specifier for this class.
			 *   If so then also returns length of matched format string.
			 *
			 *     static
			 *     boost::optional<int>
			 *     match_format(
			 *         const QString &rest_of_filename_template)
			 *     {
			 *     }
			 */


			/**
			 * Enumeration that specified whether a format varies with reconstruction time,
			 * or varies across sequence iterators or is constant always.
			 */
			enum Variation
			{
				VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME,
				VARIES_WITH_SEQUENCE_ITERATOR,
				IS_CONSTANT
			};


			virtual
			~Format()
			{  }


			/**
			 * Returns @a Variation enum specifying whether this format varies with reconstruction time,
			 * or varies across sequence iterators or is constant always.
			 */
			virtual
			Variation
			get_variation_type() const = 0;


			/**
			 * Converts this format to a @a QString potentially using the current
			 * index and reconstruction time in the sequence and the date/time.
			 */
			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const = 0;
		};


		/**
		 * Simple format pattern percent '%' character.
		 */
		class PercentCharacterFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = IS_CONSTANT;

			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const
			{
				return "%";
			}
		};


		/**
		 * Simple format pattern for a placeholder.
		 *
		 * The placeholder format is different than other formats in that
		 * it isn't replaced with anything. It is simply a pattern that client
		 * code reserves for its own use and interpretation.
		 */
		class PlaceholderFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = IS_CONSTANT;

			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;
		};


		/**
		 * Simple format pattern for reconstruction anchor plate id.
		 */
		class ReconstructionAnchorPlateIdFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = IS_CONSTANT;


			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			ReconstructionAnchorPlateIdFormat(
					const GPlatesModel::integer_plate_id_type &anchor_plate_id) :
				d_reconstruction_anchor_plate_id(anchor_plate_id)
			{  }

			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;

		private:
			GPlatesModel::integer_plate_id_type d_reconstruction_anchor_plate_id;
		};


		/**
		 * Simple format pattern for the layer name of the default reconstruction tree layer.
		 */
		class DefaultReconstructionTreeLayerNameFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = IS_CONSTANT;


			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			DefaultReconstructionTreeLayerNameFormat(
					const QString &default_recon_tree_layer_name) :
				d_default_recon_tree_layer_name(default_recon_tree_layer_name)
			{
				// Some Operating Systems may have trouble with spaces in filenames.
				// For now we'll just replace spaces with underscores.
				// TODO: Replace other whitespace characters also.
				d_default_recon_tree_layer_name.replace(' ', '_');
			}

			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;

		private:
			QString d_default_recon_tree_layer_name;
		};


		/**
		 * Format pattern for frame number or index.
		 */
		class FrameNumberFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME;

			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			FrameNumberFormat(
					const QString &format_string,
					std::size_t sequence_size);

			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;

		private:
			int d_max_digits;
			//! Is frame number [1,N] otherwise it's [0,N-1].
			bool d_use_frame_number;


			/**
			 * Calculate maximum number of digits.
			 * Requires @a d_use_frame_number to be set.
			 */
			void
			calc_max_digits(
					std::size_t sequence_size);
		};


		/**
		 * Format pattern for reconstruction time in printf-style format.
		 */
		class ReconstructionTimePrintfFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = VARIES_WITH_RECONSTRUCTION_TIME_OR_FRAME;

			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			/**
			 * @param format_string printf-style format string.
			 */
			ReconstructionTimePrintfFormat(
					const QString &format_string);


			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;

		private:
			std::string d_format_string;
			bool d_is_integer_format;

			//! Returns regular expression used to match reconstruction time printf-style.
			static
			const QRegExp &
			get_full_regular_expression();

			//! Returns regular expression used to match reconstruction time printf-style integer.
			static
			const QRegExp &
			get_integer_regular_expression();
		};


		/**
		 * Format pattern for date/time.
		 */
		class DateTimeFormat :
				public Format
		{
		public:
			//! How this format varies.
			static const Variation VARIATION_TYPE = VARIES_WITH_SEQUENCE_ITERATOR;

			/**
			 * Returns true if the start of @a rest_of_filename_template matches
			 * the format specifier for this class.
			 *
			 * If so then also returns length of matched format string.
			 */
			static
			boost::optional<int>
			match_format(
					const QString &rest_of_filename_template);


			/**
			 * @param format_string printf-style format string.
			 */
			DateTimeFormat(
					const QString &format_string);


			//! The format variation type.
			virtual
			Variation
			get_variation_type() const
			{
				return VARIATION_TYPE;
			}

			virtual
			QString
			expand_format_string(
					std::size_t sequence_index,
					const double &reconstruction_time,
					const QDateTime &date_time) const;

		private:
			QString d_date_time_format;

			static const QString HOURS_MINS_SECS_WITH_DASHES_SPECIFIER;
			static const QString YEAR_MONTH_DAY_WITH_DASHES_SPECIFIER;
		};
	}
}

#endif // GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEFORMATS_H
