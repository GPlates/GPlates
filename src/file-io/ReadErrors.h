/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_READERRORS_H
#define GPLATES_FILEIO_READERRORS_H

namespace GPlatesFileIO
{
	namespace ReadErrors
	{
		enum Description
		{
			// These are specific to PLATES rotation-format reading.
			/*
			 * These enumerators are derived from the enumeration
			 * 'PlatesRotationFormatReader::ReadErrors::Description' in the header file
			 * "PlatesRotationFormatReader.hh" in the ReconTreeViewer software:
			 *  http://sourceforge.net/projects/recontreeviewer/
			 */
			CommentMovingPlateIdAfterNonCommentSequence,
			ErrorReadingFixedPlateId,
			ErrorReadingGeoTime,
			ErrorReadingMovingPlateId,
			ErrorReadingPoleLatitude,
			ErrorReadingPoleLongitude,
			ErrorReadingRotationAngle,
			MovingPlateIdEqualsFixedPlateId,
			NoCommentFound,
			NoExclMarkToStartComment,
			SamePlateIdsButDuplicateGeoTime,
			SamePlateIdsButEarlierGeoTime,

			// The following are specific to PLATES line-format reading.
			InvalidPlatesRegionNumber,
			InvalidPlatesReferenceNumber,
			InvalidPlatesStringNumber, 
			InvalidPlatesGeographicDescription, 
			InvalidPlatesPlateIdNumber, 
			InvalidPlatesAgeOfAppearance, 
			InvalidPlatesAgeOfDisappearance, 
			InvalidPlatesDataTypeCode, 
			InvalidPlatesDataTypeCodeNumber, 
			InvalidPlatesDataTypeCodeNumberAdditional, 
			InvalidPlatesConjugatePlateIdNumber, 
			InvalidPlatesColourCode, 
			InvalidPlatesNumberOfPoints,
			UnknownPlatesDataTypeCode,
			MissingPlatesPolylinePoint,
			MissingPlatesHeaderSecondLine,
			InvalidPlatesPolylinePoint,
			BadPlatesPolylinePlotterCode,
			BadPlatesPolylineLatitude,
			BadPlatesPolylineLongitude
		};

		enum Result
		{
			// These are specific to PLATES rotation-format reading.
			/*
			 * These enumerators are derived from the enumeration
			 * 'PlatesRotationFormatReader::ReadErrors::Result' in the header file
			 * "PlatesRotationFormatReader.hh" in the ReconTreeViewer software:
			 *  http://sourceforge.net/projects/recontreeviewer/
			 */
			EmptyCommentCreated,
			ExclMarkInsertedAtCommentStart,
			MovingPlateIdChangedToMatchEarlierSequence,
			NewOverlappingSequenceBegun,
			PoleDiscarded,

			// The following are specific to PLATES line-format reading.
			UnclassifiedFeatureCreated,
			FeatureDiscarded
		};
	}
}

#endif  // GPLATES_FILEIO_READERRORS_H
