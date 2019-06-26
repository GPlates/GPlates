/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "PyRevisionedVector.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	std::string
	get_python_list_operations_docstring(
			const char *class_name)
	{
		std::stringstream python_list_docstring_stream;
		python_list_docstring_stream <<
				"=========================== ==========================================================\n"
				"Operation                   Result\n"
				"=========================== ==========================================================\n"
				"``for x in s``              iterates over the elements *x* of *s*\n"
				"``x in s``                  ``True`` if *x* is an item of *s*\n"
				"``x not in s``              ``False`` if *x* is an item of *s*\n"
				"``s += t``                  the *" << class_name << "* instance *s* is extended by sequence *t*\n"
				"``s + t``                   the concatenation of sequences *s* and *t* where either, or both, is a *" << class_name << "*\n"
				"``s[i]``                    the item of *s* at index *i*\n"
				"``s[i] = x``                replace the item of *s* at index *i* with *x*\n"
				"``del s[i]``                remove the item at index *i* from *s*\n"
				"``s[i:j]``                  slice of *s* from *i* to *j*\n"
				"``s[i:j] = t``              slice of *s* from *i* to *j* is replaced by the contents of the sequence *t* "
				"(the slice and *t* can be different lengths)\n"
				"``del s[i:j]``              same as ``s[i:j] = []``\n"
				"``s[i:j:k]``                slice of *s* from *i* to *j* with step *k*\n"
				"``del s[i:j:k]``            removes the elements of ``s[i:j:k]`` from the list\n"
				"``s[i:j:k] = t``            the elements of ``s[i:j:k]`` are replaced by those of *t* "
				"(the slice and *t* **must** be the same length if ``k != 1``)\n"
				"``len(s)``                  length of *s*\n"
				"``s.append(x)``             add element *x* to the end of *s*\n"
				"``s.extend(t)``             add the elements in sequence *t* to the end of *s*\n"
				"``s.insert(i,x)``           insert element *x* at index *i* in *s*\n"
				"``s.pop([i])``              removes the element at index *i* in *s* and returns it (defaults to last element)\n"
				"``s.remove(x)``             removes the first element in *s* that equals *x* (raises ``ValueError`` if not found)\n"
				"``s.count(x)``              number of occurrences of *x* in *s*\n"
				"``s.index(x[,i[,j]])``      smallest *k* such that ``s[k] == x`` and ``i <= k < j`` (raises ``ValueError`` if not found)\n"
				"``s.reverse()``             reverses the items of *s* in place\n"
				"``s.sort(key[,reverse])``   sort the items of *s* in place (note that *key* is **not** optional and, like python 3.0, we removed *cmp*)\n"
				"=========================== ==========================================================\n"
		;

		return python_list_docstring_stream.str();
	}
}

#endif // GPLATES_NO_PYTHON
