/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBEEXTERNAL_H
#define GPLATES_SCRIBE_TRANSCRIBEEXTERNAL_H

//
// Include 'transcribe()' specialisations and overloads for *external libraries* in one place.
//
// This header has to be included explicitly because external library header files do not contain
// 'transcribe()' specialisations and overloads (because we can't, of course, edit those files).
//
// This is not necessary for most GPlates classes (transcribing objects instantiated from them) because
// header files for GPlates classes contain the necessary specialisations/overloads for 'transcribe()'.
//
// NOTE: Only types from external libraries should be handled by this file.
// Classes from the GPlates source code should handle transcribing by declaring their 'transcribe()'
// specialisation/overload in the class's header file.
//

#include "Transcribe.h"

#include "TranscribeBoost.h"
#include "TranscribeQt.h"
#include "TranscribeStd.h"

// As a special case we include a 'transcribe() specialisation/overload for handling
// 'GPlatesUtils::non_null_intrusive_ptr<>'.
// It's not, strictly speaking, an external library but it was copied straight from
// "boost/intrusive_ptr.hpp" which is external.
// We could have put it directly in "utils/non_null_intrusive_ptr.h" instead of here but
// one disadvantage of that is slower compile times since 'GPlatesUtils::non_null_intrusive_ptr<>'
// is used quite extensively throughout GPlates and would pull in heavyweight "Scribe.h" everywhere.
#include "TranscribeNonNullIntrusivePtr.h"

#endif // GPLATES_SCRIBE_TRANSCRIBEEXTERNAL_H
