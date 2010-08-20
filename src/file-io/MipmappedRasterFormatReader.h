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

#ifndef GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H
#define GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QEvent>
#include <QEventLoop>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QDebug>

#include "ErrorOpeningFileForReadingException.h"
#include "FileFormatNotSupportedException.h"
#include "MipmappedRasterFormat.h"

#include "gui/Colour.h"

#include "property-values/RawRaster.h"


namespace GPlatesFileIO
{
	/**
	 * MipmappedRasterFormatReader reads mipmapped images from a mipmapped raster
	 * file. It is able to read a given region of a given mipmap level.
	 *
	 * The template parameter @a RawRasterType is the type of the *mipmapped*
	 * rasters stored in the file, not the type of the source raster.
	 */
	template<class RawRasterType>
	class MipmappedRasterFormatReader
	{
	public:

		/**
		 * Opens @a filename for reading as a mipmapped raster file.
		 *
		 * @throws @a ErrorOpeningFileForReadingException if @a filename could not be
		 * opened for reading.
		 *
		 * @throws @a FileFormatNotException if the header information is wrong.
		 */
		MipmappedRasterFormatReader(
				const QString &filename) :
			d_file(filename),
			d_in(&d_file),
			d_is_closed(false)
		{
			// Attempt to open the file for reading.
			if (!d_file.open(QIODevice::ReadOnly))
			{
				throw ErrorOpeningFileForReadingException(
						GPLATES_EXCEPTION_SOURCE, filename);
			}

			// Check that there is enough data in the file for magic number and version.
			QFileInfo file_info(d_file);
			static const qint64 MAGIC_NUMBER_AND_VERSION_SIZE = 8;
			if (file_info.size() < MAGIC_NUMBER_AND_VERSION_SIZE)
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad header");
			}

			// Check the magic number.
			quint32 magic_number;
			d_in >> magic_number;
			if (magic_number != MipmappedRasterFormat::MAGIC_NUMBER)
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad magic number");
			}

			// Check the version number.
			quint32 version_number;
			d_in >> version_number;
			if (version_number == 1)
			{
				d_impl.reset(new VersionOneReader(d_file, d_in));
			}
			else
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad version number");
			}
		}

		~MipmappedRasterFormatReader()
		{
			if (!d_is_closed)
			{
				d_file.close();
			}
		}

		/**
		 * Closes the file, and no further reading is possible.
		 */
		void
		close()
		{
			d_file.close();
			d_is_closed = true;
		}

		/**
		 * Returns the number of levels in the current mipmapped raster file.
		 */
		unsigned int
		get_number_of_levels() const
		{
			return d_impl->get_number_of_levels();
		}

		/**
		 * Reads the given region from the mipmap at the given @a level.
		 *
		 * Returns boost::none if the @a level is non-existent, or if the region given
		 * lies partly or wholly outside the mipmap at the given @a level. Also
		 * returns boost::none if the file has already been closed.
		 *
		 * The @a level is the level in the mipmapped raster file. For the first
		 * mipmap level (i.e. one size smaller than the source raster), specify 0 as
		 * the @a level, because the mipmapped raster file does not store the source
		 * raster.
		 */
		boost::optional<typename RawRasterType::non_null_ptr_type>
		read_level(
				unsigned int level,
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (d_is_closed)
			{
				return boost::none;
			}
			else
			{
				return d_impl->read_level(level, x_offset, y_offset, width, height);
			}
		}

		/**
		 * Reads the given region from the coverage raster at the given @a level.
		 *
		 * Returns boost::none if the @a level is non-existent, or if the region given
		 * lies partly or wholly outside the mipmap at the given @a level. Also
		 * returns boost::none if the file has already been closed.
		 *
		 * The @a level is the level in the mipmapped raster file. For the first
		 * mipmap level (i.e. one size smaller than the source raster), specify 0 as
		 * the @a level, because the mipmapped raster file does not store the source
		 * raster.
		 */
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
		read_coverage(
				unsigned int level,
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (d_is_closed)
			{
				return boost::none;
			}
			else
			{
				return d_impl->read_coverage(level, x_offset, y_offset, width, height);
			}
		}

		/**
		 * Retrieves information about the file that we are reading.
		 */
		QFileInfo
		get_file_info() const
		{
			return QFileInfo(d_file);
		}

		/**
		 * Returns the filename of the file that we are reading.
		 */
		QString
		get_filename() const
		{
			return d_file.fileName();
		}

	private:

		class ReaderImpl
		{
		public:

			virtual
			~ReaderImpl()
			{
			}

			virtual
			unsigned int
			get_number_of_levels() = 0;

			virtual
			boost::optional<typename RawRasterType::non_null_ptr_type>
			read_level(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height) = 0;

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height) = 0;
		};

		/**
		 * Reads in mipmapped raster files that have a version number of 1.
		 */
		class VersionOneReader :
				public ReaderImpl
		{
			static const QEvent::Type QUIT_EVENT;
			static const QEvent::Type UNLOCK_MUTEX_EVENT;
			static const QEvent::Type READ_RASTER_EVENT;
			static const QEvent::Type READ_COVERAGE_EVENT;
			static const QEvent::Type READ_AHEAD_EVENT;

			// Events are sorted in order of decreasing priority.
			static const int READ_AHEAD_EVENT_PRIORITY = 0;
			static const int OTHER_EVENT_PRIORITY = 1;
			static const int UNLOCK_MUTEX_EVENT_PRIORITY = 2;

			class QuitEvent :
					public QEvent
			{
			public:

				QuitEvent() :
					QEvent(QUIT_EVENT)
				{  }
			};

			class UnlockMutexEvent :
					public QEvent
			{
			public:

				UnlockMutexEvent() :
					QEvent(UNLOCK_MUTEX_EVENT)
				{  }
			};

			class ReadRasterEvent :
					public QEvent
			{
			public:

				ReadRasterEvent(
						unsigned int level_,
						typename RawRasterType::non_null_ptr_type &dest_raster_,
						unsigned int x_offset_,
						unsigned int y_offset_) :
					QEvent(READ_RASTER_EVENT),
					level(level_),
					dest_raster(dest_raster_),
					x_offset(x_offset_),
					y_offset(y_offset_)
				{  }

				unsigned int level;
				typename RawRasterType::non_null_ptr_type &dest_raster;
				unsigned int x_offset;
				unsigned int y_offset;
			};

			class ReadCoverageEvent :
					public QEvent
			{
			public:

				ReadCoverageEvent(
						unsigned int level_,
						GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &dest_raster_,
						unsigned int x_offset_,
						unsigned int y_offset_) :
					QEvent(READ_COVERAGE_EVENT),
					level(level_),
					dest_raster(dest_raster_),
					x_offset(x_offset_),
					y_offset(y_offset_)
				{  }

				unsigned int level;
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &dest_raster;
				unsigned int x_offset;
				unsigned int y_offset;
			};

			class ReadAheadEvent :
					public QEvent
			{
			public:

				ReadAheadEvent() :
					QEvent(READ_AHEAD_EVENT)
				{  }
			};

			/**
			 * A reader that predicts what blocks will be read next and reads them
			 * into memory from disk so that they can be accessed quickly.
			 */
			class LookAheadReader :
					public QObject
			{
			private:

				/**
				 * Bundles together the level, x-offset and y-offset of a block.
				 * No size is stored as all cached blocks are of the same size,
				 * BLOCK_WIDTH by BLOCK_HEIGHT.
				 */
				struct BlockInfo
				{
					BlockInfo() :
						level(-1),
						x_offset(-1),
						y_offset(-1)
					{  }

					BlockInfo(
							unsigned int level_,
							unsigned int x_offset_,
							unsigned int y_offset_) :
						level(level_),
						x_offset(x_offset_),
						y_offset(y_offset_)
					{  }

					bool
					operator==(
							const BlockInfo &other)
					{
						return level == other.level &&
							x_offset == other.x_offset &&
							y_offset == other.y_offset;
					}

					unsigned int level, x_offset, y_offset;
				};

				/**
				 * A circular buffer of fixed size of BlockInfo objects.
				 * Behaves like a stack (push/pop from top) but where the bottom
				 * elements fall off if the stack gets too high.
				 */
				class CircularBuffer
				{
				public:

					CircularBuffer(
							unsigned int max_size) :
						d_max_size(max_size),
						d_buffer(new BlockInfo[max_size]),
						d_head(0),
						d_size(0)
					{  }

					void
					push(
							const BlockInfo &block)
					{
						d_buffer[d_head] = block;

						d_head = next(d_head);
						if (d_size < d_max_size)
						{
							++d_size;
						}
					}

					bool
					is_empty() const
					{
						return d_size == 0;
					}

					unsigned int
					size() const
					{
						return d_size;
					}

					const BlockInfo &
					top() const
					{
						return d_buffer[prev(d_head)];
					}

					void
					pop()
					{
						if (d_size != 0)
						{
							d_head = prev(d_head);
							--d_size;
							d_buffer[d_head] = BlockInfo();
						}
					}

					bool
					contains(
							const BlockInfo &info) const
					{
						unsigned int curr_index = prev(d_head);
						for (unsigned int i = 0; i != d_size; ++i)
						{
							if (d_buffer[curr_index] == info)
							{
								return true;
							}
							curr_index = prev(curr_index);
						}
						return false;
					}

					void
					push_if_not_contains(
							const BlockInfo &info)
					{
						if (!contains(info))
						{
							push(info);
						}
					}

				private:

					unsigned int
					prev(
							unsigned int index) const
					{
						if (index == 0)
						{
							return d_max_size - 1;
						}
						else
						{
							return index - 1;
						}
					}

					unsigned int
					next(
							unsigned int index) const
					{
						if (index == d_max_size - 1)
						{
							return 0;
						}
						else
						{
							return index + 1;
						}
					}

					unsigned int d_max_size;
					boost::scoped_array<BlockInfo> d_buffer;
					unsigned int d_head;
					unsigned int d_size;
				};

			public:

				static const unsigned int BLOCK_WIDTH = 256;
				static const unsigned int BLOCK_HEIGHT = 256;

				LookAheadReader(
						QMutex &mutex,
						QWaitCondition &ready,
						std::vector<MipmappedRasterFormat::LevelInfo> &level_infos,
						unsigned int cache_size,
						bool cache_coverages,
						const boost::function<
							void (
							unsigned int,
							unsigned int,
							typename RawRasterType::element_type *,
							unsigned int,
							unsigned int,
							unsigned int,
							unsigned int)
						> &copy_raster_region_function,
						const boost::function<
							void (
							unsigned int,
							unsigned int,
							GPlatesPropertyValues::CoverageRawRaster::element_type *,
							unsigned int,
							unsigned int,
							unsigned int,
							unsigned int)
						> &copy_coverage_region_function,
						const boost::function<void ()> &quit_function) :
					d_mutex(mutex),
					d_ready(ready),
					d_level_infos(level_infos),
					d_copy_raster_region_function(copy_raster_region_function),
					d_copy_coverage_region_function(copy_coverage_region_function),
					d_quit_function(quit_function),
					d_recently_read_blocks(cache_size * 2),
					d_read_ahead_blocks(cache_size),
					d_cache_priority(new unsigned int[cache_size]),
					d_cache_mapping(new BlockInfo[cache_size]),
					d_pending_read_ahead_event(false)
				{
					for (unsigned int i = 0; i != cache_size; ++i)
					{
						d_raster_cache.push_back(RawRasterType::create(BLOCK_WIDTH, BLOCK_HEIGHT));
						if (cache_coverages)
						{
							d_coverage_cache.push_back(
								GPlatesPropertyValues::CoverageRawRaster::create(BLOCK_WIDTH, BLOCK_HEIGHT));
						}
						d_cache_priority[i] = i;
					}
				}

				virtual
				bool
				event(
						QEvent *event_)
				{
					if (event_->type() == QUIT_EVENT)
					{
						d_quit_function();
						return true;
					}
					else if (event_->type() == UNLOCK_MUTEX_EVENT)
					{
						d_mutex.unlock();
						return true;
					}
					else if (event_->type() == READ_RASTER_EVENT)
					{
						d_mutex.lock();
						handle_read_raster_event(
								static_cast<ReadRasterEvent *>(event_));
						d_ready.wakeAll();
						d_mutex.unlock();
						return true;
					}
					else if (event_->type() == READ_COVERAGE_EVENT)
					{
						d_mutex.lock();
						handle_read_coverage_event(
								static_cast<ReadCoverageEvent *>(event_));
						d_ready.wakeAll();
						d_mutex.unlock();
						return true;
					}
					else if (event_->type() == READ_AHEAD_EVENT)
					{
#if 0
						d_mutex.lock();
#endif
						handle_read_ahead_event();
#if 0
						d_mutex.unlock();
#endif
						return true;
					}
					else
					{
						return QObject::event(event_);
					}
				}

				void
				handle_read_raster_event(
						ReadRasterEvent *read_raster_event)
				{
					// See whether the block is in our cache.
					typename RawRasterType::non_null_ptr_type &dest_raster = read_raster_event->dest_raster;
					BlockInfo current_block(read_raster_event->level, read_raster_event->x_offset, read_raster_event->y_offset);
					if (dest_raster->width() == BLOCK_WIDTH && dest_raster->height() == BLOCK_HEIGHT)
					{
						for (unsigned int i = 0; i != d_raster_cache.size(); ++i)
						{
							if (d_cache_mapping[i] == current_block)
							{
								// qDebug() << "******** cache hit!";
								typename RawRasterType::non_null_ptr_type &cached_raster = d_raster_cache[i];
								std::copy(
										cached_raster->data(),
										cached_raster->data() + BLOCK_WIDTH * BLOCK_HEIGHT,
										dest_raster->data());

								return;
							}
						}
					}

					// It's not in our cache, so let's just read it directly.
					MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[read_raster_event->level];
					d_copy_raster_region_function(
							level_info.main_offset,
							level_info.width,
							dest_raster->data(),
							read_raster_event->x_offset,
							read_raster_event->y_offset,
							dest_raster->width(),
							dest_raster->height());
					d_recently_read_blocks.push_if_not_contains(current_block);

					// Read ahead blocks based on this block.
					if (dest_raster->width() == BLOCK_WIDTH && dest_raster->height() == BLOCK_HEIGHT)
					{
						add_read_ahead_blocks(current_block);
					}

					if (!d_read_ahead_blocks.is_empty() && !d_pending_read_ahead_event)
					{
						QCoreApplication::postEvent(this, new ReadAheadEvent(), READ_AHEAD_EVENT_PRIORITY);
						d_pending_read_ahead_event = true;
					}
				}

				void
				handle_read_coverage_event(
						ReadCoverageEvent *read_raster_event)
				{
					// See whether the block is in our cache.
					GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &dest_raster = read_raster_event->dest_raster;
					BlockInfo current_block(read_raster_event->level, read_raster_event->x_offset, read_raster_event->y_offset);
					if (dest_raster->width() == BLOCK_WIDTH && dest_raster->height() == BLOCK_HEIGHT)
					{
						// Note if coverages aren't to be cached, the loop body does not run.
						for (unsigned int i = 0; i != d_coverage_cache.size(); ++i)
						{
							if (d_cache_mapping[i] == current_block)
							{
								// qDebug() << "******** coverage cache hit!";
								GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &cached_raster = d_coverage_cache[i];
								std::copy(
										cached_raster->data(),
										cached_raster->data() + BLOCK_WIDTH * BLOCK_HEIGHT,
										dest_raster->data());

								return;
							}
						}
					}

					// It's not in our cache, so let's just read it directly.
					MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[read_raster_event->level];
					d_copy_coverage_region_function(
							level_info.coverage_offset, // coverage_offset != 0 is checked for by the main thread.
							level_info.width,
							dest_raster->data(),
							read_raster_event->x_offset,
							read_raster_event->y_offset,
							dest_raster->width(),
							dest_raster->height());

					// We don't queue up additional blocks for reading ahead
					// when dealing with coverages (c.f. the code above in
					// handle_read_raster_event).
					// This works on the assumption that if client code is
					// interested in the coverage for a region, it will also
					// be interested in the main raster data too, and so it will
					// be duplication of work if we do bookkeeping here too.
					// If this assumption doesn't hold, then this nice threaded
					// look-ahead reader degenerates to looking up everything on
					// disk all the time, which is ok...
				}

				void
				handle_read_ahead_event()
				{
					if (!d_read_ahead_blocks.is_empty())
					{
						BlockInfo block = d_read_ahead_blocks.top();
						d_read_ahead_blocks.pop();

						if (!d_recently_read_blocks.contains(block))
						{
							unsigned int cache_index = d_cache_priority[0];

							// We're just populating this cache slot, so let's
							// make sure we hold on to it for a while.
							move_index_to_back_of_priority(cache_index);

							// Read raster data into the cache.
							typename RawRasterType::non_null_ptr_type &cached_raster =
								d_raster_cache[cache_index];
							MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[block.level];
							d_copy_raster_region_function(
									level_info.main_offset,
									level_info.width,
									cached_raster->data(),
									block.x_offset,
									block.y_offset,
									BLOCK_WIDTH,
									BLOCK_HEIGHT);

							// Read coverage data into the cache.
							if (d_coverage_cache.size() && level_info.coverage_offset)
							{
								GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &cached_coverage =
									d_coverage_cache[cache_index];
								d_copy_coverage_region_function(
										level_info.coverage_offset,
										level_info.width,
										cached_coverage->data(),
										block.x_offset,
										block.y_offset,
										BLOCK_WIDTH,
										BLOCK_HEIGHT);
							}

							// Make a note that we've read it.
							d_cache_mapping[cache_index] = block;
							d_recently_read_blocks.push(block);
						}
					}

					if (d_read_ahead_blocks.is_empty())
					{
						d_pending_read_ahead_event = false;
					}
					else
					{
						QCoreApplication::postEvent(this, new ReadAheadEvent(), READ_AHEAD_EVENT_PRIORITY);
					}
				}

			private:

#if 0
				void
				move_index_to_front_of_priority(
						int index)
				{
					// Shift all elements before index forward one step.
					bool found = false;
					for (unsigned int i = d_raster_cache.size() - 1; i != 0; --i)
					{
						if (d_cache_priority[i] == index)
						{
							found = true;
						}

						if (found)
						{
							d_cache_priority[i] = d_cache_priority[i - 1];
						}
					}
					
					d_cache_priority[0] = index;
				}
#endif

				void
				move_index_to_back_of_priority(
						unsigned int index)
				{
					// Shift all elements after index back one step.
					bool found = false;
					for (unsigned int i = 0; i != d_raster_cache.size() - 1; ++i)
					{
						if (d_cache_priority[i] == index)
						{
							found = true;
						}

						if (found)
						{
							d_cache_priority[i] = d_cache_priority[i + 1];
						}
					}

					d_cache_priority[d_raster_cache.size() - 1] = index;
				}

				void
				add_read_ahead_blocks(
						const BlockInfo &block)
				{
					if (block.x_offset % BLOCK_WIDTH != 0 ||
						block.y_offset % BLOCK_HEIGHT != 0)
					{
						return;
					}

					// One block up.
					if (block.y_offset != 0)
					{
						BlockInfo up(block.level, block.x_offset, block.y_offset - BLOCK_HEIGHT);
						d_read_ahead_blocks.push_if_not_contains(up);
					}

					// One block left.
					if (block.x_offset != 0)
					{
						BlockInfo left(block.level, block.x_offset - BLOCK_WIDTH, block.y_offset);
						d_read_ahead_blocks.push_if_not_contains(left);
					}

					MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[block.level];
					
					// One block down.
					if (block.y_offset + BLOCK_HEIGHT * 2 <= level_info.height)
					{
						BlockInfo down(block.level, block.x_offset, block.y_offset + BLOCK_HEIGHT);
						d_read_ahead_blocks.push_if_not_contains(down);
					}

					// One block right.
					if (block.x_offset + BLOCK_WIDTH * 2 <= level_info.width)
					{
						BlockInfo right(block.level, block.x_offset + BLOCK_WIDTH, block.y_offset);
						d_read_ahead_blocks.push_if_not_contains(right);
					}

					// Add the block in the level below.
					// Note that this block in the level below covers 4 blocks
					// in this level. As such, we only add the block if we are
					// the top left of those 4 blocks.
					if (block.level + 1 < d_level_infos.size())
					{
						unsigned int new_x_offset = block.x_offset / 2;
						unsigned int new_y_offset = block.y_offset / 2;
						MipmappedRasterFormat::LevelInfo &level_below_info = d_level_infos[block.level + 1];
						if (new_x_offset % BLOCK_WIDTH == 0 &&
							new_y_offset % BLOCK_HEIGHT == 0 &&
							new_x_offset + BLOCK_WIDTH <= level_below_info.width &&
							new_y_offset + BLOCK_HEIGHT <= level_below_info.height)
						{
							BlockInfo level_below(block.level + 1, new_x_offset, new_y_offset);
							d_read_ahead_blocks.push_if_not_contains(level_below);
						}
					}

					// Add the blocks in the level above.
					// Note that the block in this level corresponds to 4 blocks
					// in the level above.
					if (block.level != 0)
					{
						MipmappedRasterFormat::LevelInfo &level_above_info = d_level_infos[block.level - 1];
						for (unsigned int i = 0; i != 2; ++i)
						{
							for (unsigned int j = 0; j != 2; ++j)
							{
								unsigned int new_x_offset = block.x_offset * 2 + i * BLOCK_WIDTH;
								unsigned int new_y_offset = block.y_offset * 2 + j * BLOCK_HEIGHT;
								if (new_x_offset + BLOCK_WIDTH <= level_above_info.width &&
									new_y_offset + BLOCK_HEIGHT <= level_above_info.height)
								{
									BlockInfo level_above(block.level - 1, new_x_offset, new_y_offset);
									d_read_ahead_blocks.push_if_not_contains(level_above);
								}
							}
						}
					}
				}

				QMutex &d_mutex;
				QWaitCondition &d_ready;
				std::vector<MipmappedRasterFormat::LevelInfo> &d_level_infos;
				boost::function<
					void (
					unsigned int,
					unsigned int,
					typename RawRasterType::element_type *,
					unsigned int,
					unsigned int,
					unsigned int,
					unsigned int)
				> d_copy_raster_region_function;
				boost::function<
					void (
					unsigned int,
					unsigned int,
					GPlatesPropertyValues::CoverageRawRaster::element_type *,
					unsigned int,
					unsigned int,
					unsigned int,
					unsigned int)
				> d_copy_coverage_region_function;
				boost::function<void ()> d_quit_function;

				// Internal data structures for bookkeeping:

				std::vector<typename RawRasterType::non_null_ptr_type> d_raster_cache;
				std::vector<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> d_coverage_cache;

				CircularBuffer d_recently_read_blocks;
				CircularBuffer d_read_ahead_blocks;

				boost::scoped_array<unsigned int> d_cache_priority; // index 0 is the next cached block to use.
				boost::scoped_array<BlockInfo> d_cache_mapping;

				bool d_pending_read_ahead_event;
			};

			/**
			 * The thread in which the LookAheadReader instance exists.
			 *
			 * The design of this thread class and the other associated classes
			 * should (I hope) mean there are no deadlocks and race conditions
			 * and general crashing.
			 *
			 * Why there should be no problems at all:
			 *  - When the thread is started, the main thread waits until the event loop
			 *    in this thread is ready to go.
			 *  - When the main thread wants a region of a raster, it will send a Qt
			 *    event to the LookAheadReader object (which lives in this thread) and
			 *    will block until that thread says the data is ready.
			 *  - Basically, all access to this thread is serialised via the Qt event
			 *    dispatch mechanism. At any one time, only one of handle_read_region_event,
			 *    handle_read_coverage_event or handle_read_ahead_event is running.
			 *  - Read-aheads are only done when the main thread isn't waiting for a
			 *    region to be read.
			 *  - An important design decision: this thread, except on start-up, when the
			 *    main thread is blocked, ***does not allocate anything on the heap***.
			 *    This obviates the need to link against a multithreaded library to
			 *    provide thread-safe new and delete operations.
			 *  - When the main thread wants a region of size 256 x 256, it is the one
			 *    that creates the memory to hold a region of that size. If the region
			 *    happily happens to have been read in by the look ahead reader, then
			 *    we copy the raster data from the cache into the memory provided by
			 *    the main thread. If the region hasn't been read in yet, then the
			 *    region gets read directly into the memory provided by the main thread.
			 *
			 * LookAheadReader assumes that the OpenGL code is going to want 256 x 256
			 * blocks, but if that assumption happens to change in the future, this can
			 * be remedied by changing the BLOCK_WIDTH and BLOCK_HEIGHT variables.
			 */
			class ReaderThread :
					public QThread
			{
			public:

				ReaderThread(
						std::vector<MipmappedRasterFormat::LevelInfo> &level_infos,
						const boost::function<
							void (
							unsigned int,
							unsigned int,
							typename RawRasterType::element_type *,
							unsigned int,
							unsigned int,
							unsigned int,
							unsigned int)
						> &copy_raster_region_function,
						const boost::function<
							void (
							unsigned int,
							unsigned int,
							GPlatesPropertyValues::CoverageRawRaster::element_type *,
							unsigned int,
							unsigned int,
							unsigned int,
							unsigned int)
						> &copy_coverage_region_function,
						QObject *parent_ = NULL) :
					QThread(parent_),
					d_main_thread_should_block(true),
					d_level_infos(level_infos),
					d_cache_size(0),
					d_cache_coverages(false),
					d_copy_raster_region_function(copy_raster_region_function),
					d_copy_coverage_region_function(copy_coverage_region_function)
				{  }

				QMutex &
				get_mutex()
				{
					return d_mutex;
				}

				QWaitCondition &
				get_ready()
				{
					return d_ready;
				}

				LookAheadReader *
				get_look_ahead_reader()
				{
					return d_look_ahead_reader.get();
				}

				bool &
				get_main_thread_should_block()
				{
					return d_main_thread_should_block;
				}

				void
				set_cache_size(
						unsigned int cache_size)
				{
					d_cache_size = cache_size;
				}

				void
				set_cache_coverages(
						bool cache_coverages)
				{
					d_cache_coverages = cache_coverages;
				}

			protected:

				virtual
				void
				run()
				{
					d_mutex.lock();
					d_look_ahead_reader.reset(new LookAheadReader(
							d_mutex,
							d_ready,
							d_level_infos,
							d_cache_size,
							d_cache_coverages,
							d_copy_raster_region_function,
							d_copy_coverage_region_function,
							boost::bind(&ReaderThread::quit, boost::ref(*this))));
					QEventLoop event_loop;
					d_main_thread_should_block = false;
					d_ready.wakeAll();

					// We unlock the mutex in an event. This guarantees that the
					// event loop is running before the main thread wakes up.
					// Note that it is possible to post events after the event
					// loop has been created, but before it has been fired up.
					QCoreApplication::postEvent(d_look_ahead_reader.get(),
						new UnlockMutexEvent(), UNLOCK_MUTEX_EVENT_PRIORITY);

					event_loop.exec();
				}

			private:

				QMutex d_mutex;
				QWaitCondition d_ready;
				bool d_main_thread_should_block;
				std::vector<MipmappedRasterFormat::LevelInfo> &d_level_infos;
				unsigned int d_cache_size;
				bool d_cache_coverages;
				boost::function<
					void (
					unsigned int,
					unsigned int,
					typename RawRasterType::element_type *,
					unsigned int,
					unsigned int,
					unsigned int,
					unsigned int)
				> d_copy_raster_region_function;
				boost::function<
					void (
					unsigned int,
					unsigned int,
					GPlatesPropertyValues::CoverageRawRaster::element_type *,
					unsigned int,
					unsigned int,
					unsigned int,
					unsigned int)
				> d_copy_coverage_region_function;
				boost::scoped_ptr<LookAheadReader> d_look_ahead_reader;
			};

		public:

			VersionOneReader(
					QFile &file,
					QDataStream &in) :
				d_file(file),
				d_in(in),
				d_reader_thread(NULL)
			{
				d_in.setVersion(MipmappedRasterFormat::Q_DATA_STREAM_VERSION);

				// Check that the file is big enough to hold at least a v1 header.
				QFileInfo file_info(d_file);
				static const qint64 MINIMUM_HEADER_SIZE = 16;
				if (file_info.size() < MINIMUM_HEADER_SIZE)
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad v1 header");
				}

				// Check that the type of raster stored in the file is as requested.
				quint32 type;
				static const qint64 TYPE_OFFSET = 8;
				d_file.seek(TYPE_OFFSET);
				d_in >> type;
				if (type != static_cast<quint32>(MipmappedRasterFormat::get_type_as_enum<
							typename RawRasterType::element_type>()))
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad raster type");
				}

				// Read the number of levels.
				quint32 num_levels;
				static const qint64 NUM_LEVELS_OFFSET = 12;
				static const qint64 LEVEL_INFO_OFFSET = 16;
				static const qint64 LEVEL_INFO_SIZE = 16;
				d_file.seek(NUM_LEVELS_OFFSET);
				d_in >> num_levels;
				if (file_info.size() < LEVEL_INFO_OFFSET + LEVEL_INFO_SIZE * num_levels)
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "insufficient levels");
				}

				// Read the level info.
				d_file.seek(LEVEL_INFO_OFFSET);
				bool any_coverages = false;
				for (quint32 i = 0; i != num_levels; ++i)
				{
					MipmappedRasterFormat::LevelInfo current_level;
					d_in >> current_level.width
							>> current_level.height
							>> current_level.main_offset
							>> current_level.coverage_offset;

					d_level_infos.push_back(current_level);

					if (current_level.coverage_offset != 0)
					{
						any_coverages = true;
					}
				}

				// Set up the reader thread.
				if (d_level_infos.size())
				{
					static const unsigned int MIN_THREADED_IMAGE_WIDTH = 1024; // pixels; arbitrary limit.
					static const unsigned int MIN_THREADED_IMAGE_HEIGHT = 768;
					MipmappedRasterFormat::LevelInfo &level0 = d_level_infos[0];

					if (level0.width >= MIN_THREADED_IMAGE_WIDTH &&
						level0.height >= MIN_THREADED_IMAGE_HEIGHT)
					{
						d_reader_thread.reset(new ReaderThread(
								d_level_infos,
								boost::bind(
									&VersionOneReader::copy_region<typename RawRasterType::element_type>,
									boost::ref(*this), _1, _2, _3, _4, _5, _6, _7),
								boost::bind(
									&VersionOneReader::copy_region<GPlatesPropertyValues::CoverageRawRaster::element_type>,
									boost::ref(*this), _1, _2, _3, _4, _5, _6, _7)));

						// Calculate the maximum number of blocks possible.
						int cache_size = 0;
						BOOST_FOREACH(MipmappedRasterFormat::LevelInfo &level_info, d_level_infos)
						{
							cache_size += (level_info.width / LookAheadReader::BLOCK_WIDTH) *
									(level_info.height / LookAheadReader::BLOCK_HEIGHT);
						}

						static const int MAX_CACHE_SIZE = 50; // number of blocks to cache; arbitrary limit.
						if (cache_size > MAX_CACHE_SIZE)
						{
							cache_size = MAX_CACHE_SIZE;
						}
						d_reader_thread->set_cache_size(cache_size);
						d_reader_thread->set_cache_coverages(any_coverages);

						// Start the thread and wait until its event loop has started up.
						qDebug() << "Using threaded mipmap reader...";
						d_reader_thread->start();
						d_reader_thread->get_mutex().lock();

						// Why is this if necessary?
						// Suppose the event loop started up so fast that it finished before we
						// got to d_reader_thread->get_mutex().lock();
						// Then we'd be waiting around for a wake-up call that never comes.
						// Note the variable gets set, in the thread, after the mutex gets
						// locked, so no problems there.
						if (d_reader_thread->get_main_thread_should_block())
						{
							static const int READER_THREAD_SETUP_WAIT = 5000;
							d_reader_thread->get_ready().wait(&d_reader_thread->get_mutex(), READER_THREAD_SETUP_WAIT);
						}
						d_reader_thread->get_mutex().unlock();
					}
				}
			}

			~VersionOneReader()
			{
				if (d_reader_thread)
				{
					QCoreApplication::postEvent(d_reader_thread->get_look_ahead_reader(), new QuitEvent(), OTHER_EVENT_PRIORITY);

					static const int READER_THREAD_QUIT_WAIT = 6000;
					if (!d_reader_thread->wait(READER_THREAD_QUIT_WAIT))
					{
						d_reader_thread->terminate();
					}
				}
			}

			virtual
			unsigned int
			get_number_of_levels()
			{
				return d_level_infos.size();
			}

			virtual
			boost::optional<typename RawRasterType::non_null_ptr_type>
			read_level(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (!is_valid_region(level, x_offset, y_offset, width, height))
				{
					return boost::none;
				}

				typename RawRasterType::non_null_ptr_type result = RawRasterType::create(
						width, height);
				
				if (d_reader_thread)
				{
					d_reader_thread->get_mutex().lock();
					QCoreApplication::postEvent(d_reader_thread->get_look_ahead_reader(),
							new ReadRasterEvent(
								level,
								result,
								x_offset,
								y_offset),
							OTHER_EVENT_PRIORITY);
					static const int READ_LEVEL_WAIT = 10000;
					d_reader_thread->get_ready().wait(&d_reader_thread->get_mutex(), READ_LEVEL_WAIT);
					d_reader_thread->get_mutex().unlock();
				}
				else
				{
					const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
					copy_region(
							level_info.main_offset,
							level_info.width,
							result->data(),
							x_offset,
							y_offset,
							width,
							height);
				}

				return result;
			}

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (!is_valid_region(level, x_offset, y_offset, width, height))
				{
					return boost::none;
				}

				const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
				if (level_info.coverage_offset == 0)
				{
					// No coverage for this level.
					return boost::none;
				}
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type result =
					GPlatesPropertyValues::CoverageRawRaster::create(width, height);

				if (d_reader_thread)
				{
					d_reader_thread->get_mutex().lock();
					QCoreApplication::postEvent(d_reader_thread->get_look_ahead_reader(),
							new ReadCoverageEvent(
								level,
								result,
								x_offset,
								y_offset),
							OTHER_EVENT_PRIORITY);
					static const int READ_COVERAGE_WAIT = 10000;
					d_reader_thread->get_ready().wait(&d_reader_thread->get_mutex(), READ_COVERAGE_WAIT);
					d_reader_thread->get_mutex().unlock();
				}
				else
				{
					copy_region(
							level_info.coverage_offset,
							level_info.width,
							result->data(),
							x_offset,
							y_offset,
							width,
							height);
				}

				return result;
			}

		private:

			bool
			is_valid_region(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (level >= d_level_infos.size())
				{
					return false;
				}

				const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
				return width > 0 && height > 0 &&
					x_offset + width <= level_info.width &&
					y_offset + height <= level_info.height;
			}

			template<typename T>
			void
			copy_region(
					unsigned int offset_in_file,
					unsigned int level_width,
					T *dest_data,
					unsigned int dest_x_offset_in_source,
					unsigned int dest_y_offset_in_source,
					unsigned int dest_width,
					unsigned int dest_height)
			{
				for (unsigned int i = 0; i != dest_height; ++i)
				{
					unsigned int source_row = dest_y_offset_in_source + i;
					d_file.seek(offset_in_file +
							(source_row * level_width + dest_x_offset_in_source) * sizeof(T));
					T *dest = dest_data + i * dest_width;
					T *dest_end = dest + dest_width;

					while (dest != dest_end)
					{
						d_in >> *dest;
						++dest;
					}
				}
			}

			QFile &d_file;
			QDataStream &d_in;
			std::vector<MipmappedRasterFormat::LevelInfo> d_level_infos;

			boost::scoped_ptr<ReaderThread> d_reader_thread;
		};

		QFile d_file;
		QDataStream d_in;
		boost::scoped_ptr<ReaderImpl> d_impl;
		bool d_is_closed;
	};


	template<class RawRasterType>
	const QEvent::Type
	MipmappedRasterFormatReader<RawRasterType>::VersionOneReader::QUIT_EVENT = static_cast<QEvent::Type>(QEvent::registerEventType());

	template<class RawRasterType>
	const QEvent::Type
	MipmappedRasterFormatReader<RawRasterType>::VersionOneReader::UNLOCK_MUTEX_EVENT = static_cast<QEvent::Type>(QEvent::registerEventType());

	template<class RawRasterType>
	const QEvent::Type
	MipmappedRasterFormatReader<RawRasterType>::VersionOneReader::READ_RASTER_EVENT = static_cast<QEvent::Type>(QEvent::registerEventType());

	template<class RawRasterType>
	const QEvent::Type
	MipmappedRasterFormatReader<RawRasterType>::VersionOneReader::READ_COVERAGE_EVENT = static_cast<QEvent::Type>(QEvent::registerEventType());

	template<class RawRasterType>
	const QEvent::Type
	MipmappedRasterFormatReader<RawRasterType>::VersionOneReader::READ_AHEAD_EVENT = static_cast<QEvent::Type>(QEvent::registerEventType());
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H
