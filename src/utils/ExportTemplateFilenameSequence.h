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

#ifndef GPLATES_UTILS_EXPORTTEMPLATEFILENAMESEQUENCE_H
#define GPLATES_UTILS_EXPORTTEMPLATEFILENAMESEQUENCE_H

#include <cstddef>
#include <iterator>  // std::iterator
#include <boost/operators.hpp>
#include <boost/shared_ptr.hpp>
#include <QDateTime>
#include <QString>

#include "global/GPlatesException.h"
#include "maths/types.h"
#include "model/types.h"


namespace GPlatesUtils
{
	namespace ExportTemplateFilename
	{
		//! Exception when reconstruction time increment is zero.
		class TimeIncrementZero;

		/**
		 * Exception when sign of reconstruction time increment does not match
		 * sign of end reconstruction time minus begin reconstruction time.
		 */
		class IncorrectTimeIncrementSign;

		//! Exception when a format string starting with '%' is not recognised.
		class UnrecognisedFormatString;

		/**
		 * Exception when there are no format strings, in filename template,
		 * that have filename variation (vary with reconstruction frame/time).
		 */
		class NoFilenameVariation;


		/**
		 * Tests for validity of parameters in the filename template.
		 *
		 * You'll need to test validity with a try/catch block to trap the two
		 * exceptions that can be thrown by this function.
		 *
		 * @param filename_template is a string containing the filename template.
		 *
		 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
		 * @throws NoFilenameVariation if no formats have filename variation (vary with reconstruction time).
		 */
		void
		validate_filename_template(
				const QString &filename_template);


		/**
		 * Format string reserved for use by the client.
		 *
		 * If this format string is found in the filename template it will not
		 * be expanded. It is then up to the client to expand this *after*
		 * the export template filename iterator is dereferenced (dereferencing
		 * is when the various format strings are expanded). The client is free
		 * to use and interpret this format string for their own purpose.
		 *
		 * An example is exporting resolved plate polygon boundaries where
		 * this format string is replaced with several different strings for the
		 * various boundary types being exported.
		 */
		const QString PLACEHOLDER_FORMAT_STRING = "%P";
	}


	// Some forward declarations.
	class ExportTemplateFilenameSequenceImpl;
	class ExportTemplateFilenameSequenceIterator;


	/**
	 * Generates a sequence of filenames given a filename template,
	 * a begin reconstruction time, an end reconstruction time and
	 * a reconstruction time increment.
	 */
	class ExportTemplateFilenameSequence
	{
	public:
		/**
		 * Typedef for the forward iterator over filenames.
		 * The iterator deferences to 'const QString'.
		 */
		typedef ExportTemplateFilenameSequenceIterator const_iterator;


		/**
		 * The constructor sets up the sequence and tests for validity of parameters.
		 *
		 * @param filename_template is a string containing the filename template which
		 *        is any string that can be used as a filename with the following
		 *        printf-style format codes inserted anywhere, and any number of times:
		 *
		 *            %% - a literal '%' character.
		 *            %n - the "number" (index + 1) of the frame,
		 *               — will lie in the inclusive range [1, @a size],
		 *               - will be padded to the width of the decimal integer representation of @a size.
		 *            %u - the index of the frame,
		 *               — will lie in the inclusive range [0, (@a size - 1)],
		 *               - will be padded to the width of the decimal integer representation of (@a size - 1).
		 *            %f - the reconstruction-time instant of the frame, in printf-style %f format.
		 *            %d - the reconstruction-time instant of the frame, in printf-style %d format,
		 *               - rounded to the closest integer.
		 *            %T - the user-time instant at which the iterator is first dereferenced,
		 *                 in the format "HH-mm-ss" (HH is 24-hour format).
		 *            %: - the user-time instant at which the iterator is first dereferenced,
		 *                 in the format "HH:mm:ss" (HH is 24-hour format).
		 *            %D - the user-date at which the iterator is first dereferenced, in the format "YYYY-MM-DD".
		 *            %A - the current anchored plate ID.
		 *
		 * @param reconstruction_anchor_plate_id anchor plate id of reconstruction tree.
		 * @param begin_reconstruction_time the time at which the sequence begins.
		 * @param end_reconstruction_time the time at which the sequence ends.
		 * @param reconstruction_time_increment the step size in time when incrementing to the next
		 *        filename in the sequence. For example, the increment to go from 140.0 Ma to 0 Ma
		 *        in steps of 1M should be supplied as -1.0.
		 * @param include_trailing_frame_in_sequence If the supplied time range is not an exact
		 *        integer multiple of the increment, should the shorter trailing frame still be included
		 *        in the sequence?
		 *        This flag only applies if the desired time range does not divide cleanly by the
		 *        increment. In such a case, setting this flag to 'false' will result in a shorter
		 *        total sequence, finishing slightly earlier than the desired end time, but will ensure
		 *        that each frame is exactly @a reconstruction_time_increment apart from the others.
		 *        Setting this flag to 'true' will permit the animation to add one additional ending frame,
		 *        even though that frame is closer to the others.
		 *        For example, consider the range [20, 4.5] with an increment of 2.0 M. Setting
		 *        @a include_trailing_frame_in_sequence to false will end the animation on 6.0 Ma;
		 *        setting it to true will end the animation at exactly 4.5 Ma.
		 *
		 * @throws TimeIncrementZero if reconstruction time increment is zero.
		 * @throws IncorrectTimeIncrementSign if sign of time increment does not match sign of
		 *         end minus begin reconstruction time.
		 * @throws UnrecognisedFormatString if no format recognised at a '%' char.
		 * @throws NoFilenameVariation if no formats have filename variation (vary with reconstruction time).
		 *
		 * Having a begin time equal to the end time may seem a little silly, but it should still work
		 * as a legal time range - only one frame will be written.
		 *
		 * @a TimeIncrementZero could still be thrown in principle, although the UI restricts the increment
		 * to a minimum of 0.01 M. @a IncorrectTimeIncrementSign is similarly possible, though restricted
		 * through the UI and AnimationSequenceUtils.
		 */
		ExportTemplateFilenameSequence(
				const QString &filename_template,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const GPlatesMaths::real_t &begin_reconstruction_time,
				const GPlatesMaths::real_t &end_reconstruction_time,
				const GPlatesMaths::real_t &reconstruction_time_increment,
				const bool include_trailing_frame_in_sequence);

		/**
		 * Returns the length of the sequence.
		 * This is the number of times the @a begin iterator can be incremented
		 * before it is equal to the @a end iterator.
		 */
		std::size_t
		size() const;

		/**
		 * Begin forward iterator over sequence of filenames.
		 * Dereferences to 'const QString'.
		 * Forward iterator can only be incremented, not decremented.
		 */
		const_iterator
		begin() const;

		/**
		 * End forward iterator over sequence of filenames.
		 * Dereferences to 'const QString'.
		 * Forward iterator can only be incremented, not decremented.
		 */
		const_iterator
		end() const;


	private:
		//! Typedef for pointer to implementation.
		typedef boost::shared_ptr<ExportTemplateFilenameSequenceImpl> impl_ptr_type;

		impl_ptr_type d_impl;
	};


	/**
	 * Forward iterator over export template filename sequence.
	 * Dereferencing iterator returns a 'const QString'.
	 */
	class ExportTemplateFilenameSequenceIterator :
			public std::iterator<std::forward_iterator_tag, const QString>,
			public boost::equality_comparable<ExportTemplateFilenameSequenceIterator>,
			public boost::incrementable<ExportTemplateFilenameSequenceIterator>
	{
	public:
		ExportTemplateFilenameSequenceIterator() :
			d_sequence_impl(NULL),
			d_sequence_index(0)
		{  }


		ExportTemplateFilenameSequenceIterator(
				const ExportTemplateFilenameSequenceImpl *sequence_impl,
				std::size_t sequence_index) :
			d_sequence_impl(sequence_impl),
			d_sequence_index(sequence_index),
			d_first_dereference(true)
		{  }


		/**
		 * Access current filename in sequence via iterator dereference.
		 * No 'operator->()' is provided since we're generating a temporary string.
		 *
		 * @throws UninitialisedIteratorException if this iterator was
		 * created with default constructor.
		 */
		const QString
		operator*() const;


		/**
		 * Pre-increment operator.
		 * Post-increment operator provided by base class boost::incrementable.
		 */
		ExportTemplateFilenameSequenceIterator &
		operator++()
		{
			++d_sequence_index;
			return *this;
		}


		/**
		 * Equality comparison for @a ExportTemplateFilenameSequenceIterator.
		 * Inequality operator provided by base class boost::equality_comparable.
		 */
		friend
		bool
		operator==(
				const ExportTemplateFilenameSequenceIterator &lhs,
				const ExportTemplateFilenameSequenceIterator &rhs)
		{
			return lhs.d_sequence_impl == rhs.d_sequence_impl &&
				lhs.d_sequence_index == rhs.d_sequence_index;
		}

	private:
		const ExportTemplateFilenameSequenceImpl *d_sequence_impl;
		std::size_t d_sequence_index;
		mutable QDateTime d_date_time;
		mutable bool d_first_dereference;
	};


	namespace ExportTemplateFilename
	{
		class TimeIncrementZero : public GPlatesGlobal::Exception
		{
		public:
			explicit
			TimeIncrementZero(
					const GPlatesUtils::CallStack::Trace &src) :
				Exception(src)
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "ExportTemplateFilename::TimeIncrementZero";
			}
		};

		class IncorrectTimeIncrementSign : public GPlatesGlobal::Exception
		{
		public:
			explicit
			IncorrectTimeIncrementSign(
					const GPlatesUtils::CallStack::Trace &src) :
				Exception(src)
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "ExportTemplateFilename::IncorrectTimeIncrementSign";
			}
		};

		class UnrecognisedFormatString : public GPlatesGlobal::Exception
		{
		public:
			explicit
			UnrecognisedFormatString(
					const GPlatesUtils::CallStack::Trace &src,
					const QString &format_string) :
				Exception(src),
				d_format_string(format_string)
			{  }

			//! Returns format string at which beginning does not match any specifiers.
			const QString &
			get_format_string() const
			{
				return d_format_string;
			}

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "ExportTemplateFilename::UnrecognisedFormatString";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:
			QString d_format_string;
		};

		class NoFilenameVariation : public GPlatesGlobal::Exception
		{
		public:
			explicit
			NoFilenameVariation(
					const GPlatesUtils::CallStack::Trace &src) :
				Exception(src)
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "ExportTemplateFilename::NoFilenameVariation";
			}
		};
	}
}

#endif // GPLATES_UTILS_EXPORTTEMPLATEFILENAMESEQUENCE_H
