/* $Id$ */

/**
 * \file
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
 
#ifndef GPLATES_GUI_RECONSTRUCTIONHOOK_H
#define GPLATES_GUI_RECONSTRUCTIONHOOK_H

#include <QDebug>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	/**
	 * FIXME: Putting this here for now, because everything feels so sketchy.
	 * Although JB suggested the Template Method pattern, the way this is
	 * starting to feel feels more like Strategy - as you wouldn't want
	 * to really replace the ViewportWindow.cc::render_model() algorithm
	 * with a new (template-method-overridden) one, you'd just want to be
	 * able to add and remove several different pluggable 'extras' which can
	 * process things at various stages of the reconstruct/render sequence.
	 *
	 * Of course, I don't want to go overboard and implement the whole
	 * Reconstruction Time Entity idea JB sketched up on the board, but that
	 * seems to be the direction my thoughts inevitably end up on.
	 * -JC
	 */
	class ReconstructionHook:
			public GPlatesUtils::ReferenceCount<ReconstructionHook>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionHook,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
				
		virtual
		~ReconstructionHook()
		{  }
		
		virtual
		void
		pre_reconstruction_hook()
		{  }
		
		virtual
		void
		post_reconstruction_hook()
		{  }
		
		virtual
		void
		post_velocity_computation_hook()
		{  }
	
	protected:
		explicit
		ReconstructionHook()
		{  }
	};
	

	/**
	 * Yes, both in the same file for now, I feel guilty about it but am unrepentant.
	 */
	class ExportVelocityFileReconstructionHook:
			public GPlatesGui::ReconstructionHook
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportVelocityFileReconstructionHook,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
		
		static
		const non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ExportVelocityFileReconstructionHook(),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		
		virtual
		void
		post_velocity_computation_hook()
		{
			// Write files!
		}
		
	protected:
		explicit
		ExportVelocityFileReconstructionHook()
		{  }
	};
	
}

#endif // GPLATES_GUI_RECONSTRUCTIONHOOK_H


