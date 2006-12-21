/* $Id: gplates_main.cc 968 2006-11-20 03:28:31Z jboyden $ */

/**
 * \file 
 * $Revision: 968 $
 * $Date: 2006-11-20 14:28:31 +1100 (Mon, 20 Nov 2006) $ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

namespace GPlatesPresenter
{
    class Presenter
    {
    public:
        Presenter &get_presenter()
        {
            static Presenter presenter;
            return presenter;
        }
    private:
        // This is a Singleton so we need to hide the usual suspects.
        Presenter() {  }
        Presenter(const Presenter &);
        Presenter &operator=(const Presenter &);
        ~Presenter();
    };
}
