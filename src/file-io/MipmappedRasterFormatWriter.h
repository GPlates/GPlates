/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H
#define GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H

#include <vector>
#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDebug>

#include "ErrorOpeningFileForWritingException.h"
#include "MipmappedRasterFormat.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/Mipmapper.h"

#include "property-values/RawRaster.h"


namespace GPlatesFileIO
{
	namespace MipmappedRasterFormatWriterInternals
	{
		template<typename T>
		void
		write(
				QDataStream &out,
				const T *data,
				unsigned int len)
		{
			const T *end = data + len;
			while (data != end)
			{
				out << *data;
				++data;
			}
		}
	}

	template<class RawRasterType>
	class MipmappedRasterFormatWriter
	{
	public:

		/**
		 * Creates mipmaps for @a raster, and writes a Mipmapped Raster Format file
		 * at @a filename.
		 *
		 * @a last_modified is the time of last modification of the source
		 * raster file.
		 *
		 * Throws @a ErrorOpeningFileForWritingException if the file could not
		 * be opened for writing.
		 */
		void
		write(
				const typename RawRasterType::non_null_ptr_to_const_type &raster,
				const QDateTime &last_modified,
				const QString &filename)
		{
			// Open the file for writing.
			QFile file(filename);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
			{
				throw ErrorOpeningFileForWritingException(
						GPLATES_EXCEPTION_SOURCE, filename);
			}
			QDataStream out(&file);

			// Write magic number and version number.
			out << static_cast<quint32>(MipmappedRasterFormat::MAGIC_NUMBER);
			out << static_cast<quint32>(MipmappedRasterFormat::VERSION_NUMBER);
			out.setVersion(MipmappedRasterFormat::Q_DATA_STREAM_VERSION);
			
			typedef GPlatesGui::Mipmapper<RawRasterType> mipmapper_type;
			typedef typename mipmapper_type::output_raster_type mipmapped_raster_type;
			typedef typename mipmapped_raster_type::element_type element_type;
			typedef GPlatesPropertyValues::CoverageRawRaster coverage_raster_type;
			typedef coverage_raster_type::element_type coverage_element_type;
			mipmapper_type mipmapper(raster, MipmappedRasterFormat::THRESHOLD_SIZE);

			// Write mipmap type.
			out << static_cast<quint32>(MipmappedRasterFormat::get_type_as_enum<element_type>());

			// Write number of levels.
			quint32 num_levels = mipmapper.get_number_of_remaining_levels();
			out << num_levels;

			if (num_levels > 0)
			{
				// Stub out the level info for each level with zeroes.
				qint64 level_info_pos = file.pos();
				for (quint32 i = 0; i != num_levels * MipmappedRasterFormat::LevelInfo::NUM_COMPONENTS; ++i)
				{
					out << static_cast<quint32>(0);
				}

				// Stores the level info that will be written later.
				std::vector<MipmappedRasterFormat::LevelInfo> level_info;

				while (mipmapper.generate_next())
				{
					MipmappedRasterFormat::LevelInfo current_level_info;

					// Get and write the mipmap for the current level.
					typename mipmapped_raster_type::non_null_ptr_to_const_type current_mipmap =
						mipmapper.get_current_mipmap();
					current_level_info.width = current_mipmap->width();
					current_level_info.height = current_mipmap->height();
					current_level_info.main_offset = file.pos();
					MipmappedRasterFormatWriterInternals::write<element_type>(
						out,
						current_mipmap->data(),
						current_mipmap->width() * current_mipmap->height());

					// Get and write the associated coverage raster.
					boost::optional<coverage_raster_type::non_null_ptr_to_const_type> current_coverage =
						mipmapper.get_current_coverage();
					if (current_coverage)
					{
						current_level_info.coverage_offset = file.pos();
						MipmappedRasterFormatWriterInternals::write<coverage_element_type>(
							out,
							(*current_coverage)->data(),
							(*current_coverage)->width() * (*current_coverage)->height());
					}
					else
					{
						current_level_info.coverage_offset = 0;
					}

#if 0
					qDebug() << current_level_info.width
							<< current_level_info.height
							<< current_level_info.main_offset
							<< current_level_info.coverage_offset;
#endif
					level_info.push_back(current_level_info);
				}

				// Sanity checking.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						level_info.size() == num_levels,
						GPLATES_ASSERTION_SOURCE);

				// Write out the level info.
				file.seek(level_info_pos);
				typedef std::vector<MipmappedRasterFormat::LevelInfo>::const_iterator level_iterator_type;
				for (level_iterator_type iter = level_info.begin(); iter != level_info.end(); ++iter)
				{
					const MipmappedRasterFormat::LevelInfo &current_level_info = *iter;
					out << current_level_info.width
						<< current_level_info.height
						<< current_level_info.main_offset
						<< current_level_info.coverage_offset;
				}
			}

			file.close();
		}
	};


	/**
	 * Template specialisation for use when you don't know the type of RawRaster.
	 */
	template<>
	class MipmappedRasterFormatWriter<GPlatesPropertyValues::RawRaster> :
			public GPlatesPropertyValues::RawRasterVisitor
	{
	public:

		using GPlatesPropertyValues::RawRasterVisitor::visit;

		virtual
		void
		visit(
				GPlatesPropertyValues::Int8RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::UInt8RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::Int16RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::UInt16RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::Int32RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::UInt32RawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::FloatRawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::DoubleRawRaster &raster)
		{
			write(raster);
		}

		virtual
		void
		visit(
				GPlatesPropertyValues::Rgba8RawRaster &raster)
		{
			write(raster);
		}

		/**
		 * Creates mipmaps for @a raster, and writes a Mipmapped Raster Format file
		 * at @a filename.
		 *
		 * @a last_modified is the time of last modification of the source
		 * raster file.
		 *
		 * Throws @a ErrorOpeningFileForWritingException if the file could not
		 * be opened for writing.
		 */
		void
		write(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const QDateTime &last_modified,
				const QString &filename)
		{
			d_last_modified = last_modified;
			d_filename = filename;

			raster->accept_visitor(*this);
		}

	private:

		template<class RawRasterType>
		void
		write(
				const RawRasterType &raster)
		{
			MipmappedRasterFormatWriter<RawRasterType> writer;
			writer.write(typename RawRasterType::non_null_ptr_to_const_type(&raster),
					d_last_modified, d_filename);
		}

		QDateTime d_last_modified;
		QString d_filename;
	};
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H
