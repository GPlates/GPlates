/* $Id$ */

/**
 * \file Generates a sequence of filenames given a filename template.
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

#ifndef GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEIMPL_H
#define GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEIMPL_H

#include <cstddef>
#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QDateTime>
#include <QString>

#include "ExportTemplateFilenameSequenceFormats.h"

#include "model/types.h"
#include "utils/AnimationSequenceUtils.h"


namespace GPlatesFileIO
{
	namespace ExportTemplateFilename
	{
		//! Abstract base class for different types of format used in the template filename.
		class Format;
	}


	/**
	 * Implementation of @a ExportTemplateFilenameSequence.
	 */
	class ExportTemplateFilenameSequenceImpl :
			public boost::noncopyable
	{
	public:
		/**
		 * Tests for validity of parameters in the filename template.
		 *
		 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
		 *
		 * If @a check_filename_variation is true then also checks that there is filename variation
		 * (varies with reconstruction time).
		 */
		static
		void
		validate_filename_template(
				const QString &filename_template,
				bool check_filename_variation);


		/**
		 * Constructor.
		 * @throws NoFilenameVariation if no formats have filename variation.
		 */
		ExportTemplateFilenameSequenceImpl(
				const QString &filename_template,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const QString &default_recon_tree_layer_name,
				const double &begin_reconstruction_time,
				const double &reconstruction_time_increment,
				const GPlatesUtils::AnimationSequence::SequenceInfo sequence_info);

		
		//! Returns number of filenames in the sequence.
		std::size_t
		size() const
		{
			return d_sequence_info.duration_in_frames;
		}


		/**
		 * Gets the filename at index @a sequence_index in the sequence.
		 * Also @a date_time is passed here because it can differ across sequence iterators.
		 */
		QString
		get_filename(
				const std::size_t sequence_index,
				const QDateTime &date_time) const;

	private:
		//! Typedef for memory-managed pointer to @a Format.
		typedef boost::shared_ptr<ExportTemplateFilename::Format> format_ptr_type;

		//! Typedef for sequence of @a Format objects.
		typedef std::vector<format_ptr_type> format_seq_type;

		/**
		 * Filename template string containing placeholders %1, %2, etc for each format.
		 */
		QString d_filename_template;

		const double d_begin_reconstruction_time;
		const double d_reconstruction_time_increment;
		GPlatesUtils::AnimationSequence::SequenceInfo d_sequence_info;

		format_seq_type d_format_seq;


		//! Used to extract @a ExportTemplateFilename::Format from filename template.
		class FormatExtractor
		{
		public:
			/**
			 * Tests for validity of parameters in the filename template.
			 *
			 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
			 *
			 * If @a check_filename_variation is true and if no formats have filename variation
			 * (vary with reconstruction time) then throws NoFilenameVariation.
			 */
			static
			void
			validate_filename_template(
					const QString &filename_template,
					bool check_filename_variation);


			/**
			 * Searches for format patterns in @a filename_template and replaces
			 * them with %1, %2, etc while also extracting a derived
			 * @a ExportTemplateFilename::Format object for each pattern.
			 */
			FormatExtractor(
					QString &filename_template,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const QString &default_recon_tree_layer_name,
					format_seq_type &format_seq,
					const GPlatesUtils::AnimationSequence::SequenceInfo sequence_info) :
				d_filename_template(filename_template),
				d_format_seq(format_seq),
				d_sequence_info(sequence_info),
				d_reconstruction_anchor_plate_id(reconstruction_anchor_plate_id),
				d_default_recon_tree_layer_name(default_recon_tree_layer_name),
				d_format_index(0),
				d_filename_current_pos(0)
			{  }

			/**
			 * Extracts @a ExportTemplateFilename::Format derived objects from the
			 * format patterns in the filename template and also validates them.
			 *
			 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
			 * @throws NoFilenameVariation if no formats have filename variation.
			 */
			void
			extract_formats_from_filename_template();

		private:
			//! Typedef for a @a Format object and the format string it recognised.
			typedef std::pair<format_ptr_type, QString> create_format_info_type;

			//! Typedef for a matched format string and the variation of the format that matched it.
			typedef std::pair<QString, GPlatesFileIO::ExportTemplateFilename::Format::Variation>
					validate_format_info_type;

			//! Utility to help transfer a type with boost::mpl::for_each.
			template <class Type>
			struct Wrap
			{  };

			//! Utility iterated over by boost::mpl::for_each to create a @a Format.
			class CreateFormat
			{
			public:
				CreateFormat(
						FormatExtractor *format_extractor) :
					d_format_extractor(format_extractor)
				{  }

				const boost::optional<create_format_info_type> &
				get_format_info() const
				{
					return d_format_info;
				}

				template <class FormatType>
				void
				operator()(
						Wrap<FormatType>);

			private:
				FormatExtractor *d_format_extractor;
				boost::optional<create_format_info_type> d_format_info;
			};

			//! Utility iterated over by boost::mpl::for_each to find a matching format.
			class ValidateFormat
			{
			public:
				ValidateFormat(
						const QString &rest_of_filename_template) :
					d_rest_of_filename_template(rest_of_filename_template)
				{  }

				const boost::optional<validate_format_info_type> &
				get_format_info() const
				{
					return d_format_info;
				}

				template <class FormatType>
				void
				operator()(
						Wrap<FormatType>);

			private:
				QString d_rest_of_filename_template;
				boost::optional<validate_format_info_type> d_format_info;
			};


			QString &d_filename_template;
			format_seq_type &d_format_seq;

			const GPlatesUtils::AnimationSequence::SequenceInfo d_sequence_info;
			GPlatesModel::integer_plate_id_type d_reconstruction_anchor_plate_id;
			QString d_default_recon_tree_layer_name;

			int d_format_index;
			int d_filename_current_pos;


			/**
			 * Returns a matched format string from @a rest_of_filename_template or
			 * throws @a UnrecognisedFormatString.
			 */
			static
			validate_format_info_type
			validate_format(
					const QString &rest_of_filename_template);


			/**
			 * Creates a format from current position in filename template string
			 * and returns matching format string.
			 *
			 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
			 */
			create_format_info_type
			create_format();


			/**
			 * Returns true if a format of type @a FormatType matches the format string
			 * beginning at @a rest_of_filename_template.
			 *
			 * Returns the matched format string.
			 */
			template <class FormatType>
			static
			boost::optional<QString>
			match_format(
					const QString &rest_of_filename_template);

			/**
			 * Creates a format of type @a FormatType.
			 */
			template <class FormatType>
			format_ptr_type
			create_format(
					const QString &format_string);

			//! Handles format object depending on how it varies with reconstruction time and across iterators.
			void
			handle_format(
					format_ptr_type format,
					const QString &format_string);

			//! Handles format object that varies with reconstruction time or sequence iterator.
			void
			handle_format_varies_with_reconstruction_time_or_iterator(
					format_ptr_type format,
					const QString &format_string);

			//! Handles format object that does not have filename variation.
			void
			handle_format_is_constant(
					format_ptr_type format,
					const QString &format_string);

			//! Throws @a NoFilenameVariation exception if filename template does not vary with reconstruction time.
			void
			check_filename_template_varies_with_reconstruction_time();
		};
	};
}

#endif // GPLATES_FILE_IO_EXPORTTEMPLATEFILENAMESEQUENCEIMPL_H
