/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <cstring>
#include "DrawableData.h"
#include "GridData.h"
#include "global/types.h"
#include "global/IllegalParametersException.h"
#include "maths/GridOnSphere.h"
#include "maths/PointOnSphere.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

GridData::GridData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs, 
	const GPlatesMaths::PointOnSphere &origin,
	const GPlatesMaths::PointOnSphere &sc_step,
	const GPlatesMaths::PointOnSphere &gc_step)
	: DrawableData (dt, id, tw, attrs),
	  _lattice (GPlatesMaths::GridOnSphere::Create
	            (origin, sc_step, gc_step))
{
	grid = new Grid;
	grid->offset = 0;
	grid->length = 0;
}

GridData::~GridData ()
{
	GridRowPtr row = grid->rows[0];
	for (index_t i = 0; i < grid->length; ++i, ++row) {
		// TODO: do something with elements in each row?
		//for (index_t j = 0; j < row->length; ++j) {
			//GridElementPtr elem = row->data[j + row->offset];
		//}
		delete[] row->data;
	}
	delete[] grid->rows;
}

void GridData::Add (const GridElement *element, index_t x1, index_t x2)
{
	index_t new_len;
	GridRowPtr *new_rows;
	GridElementPtr *new_elems;

	if (grid->length == 0)
		grid->offset = x1;
	// expand out to the correct row
	if (x1 >= (grid->offset + grid->length)) {
		// expand to right
		new_len = x1 - grid->offset + 1;
		new_rows = new GridRowPtr[new_len];
		if (grid->length > 0) {
			memcpy (new_rows, grid->rows,
					grid->length * sizeof (GridRowPtr));
			delete[] grid->rows;
		}
		memset (&new_rows[grid->length], 0,
				(new_len - grid->length) * sizeof (GridRowPtr));
		grid->length = new_len;
		grid->rows = new_rows;
	} else if (x1 < grid->offset) {
		// expand to left
		new_len = grid->length + grid->offset - x1;
		new_rows = new GridRowPtr[new_len];
		memcpy (&new_rows[grid->offset - x1], grid->rows,
					grid->length * sizeof (GridRowPtr));
		delete[] grid->rows;
		memset (new_rows, 0, (grid->offset - x1) * sizeof (GridRowPtr));
		grid->length = new_len;
		grid->rows = new_rows;
	}

	GridRow *row = grid->rows[x1 - grid->offset];
	if (!row) {
		// allocate this row
		row = new GridRow;
		row->length = 0;
		grid->rows[x1 - grid->offset] = row;
	}

	if (row->length == 0)
		row->offset = x2;
	// expand out to the correct column
	if (x2 >= (row->offset + row->length)) {
		// expand to right
		new_len = x2 - row->offset + 1;
		new_elems = new GridElementPtr[new_len];
		if (row->length > 0) {
			memcpy (new_elems, row->data,
					row->length * sizeof (GridElementPtr));
			delete[] row->data;
		}
		memset (&new_elems[row->length], 0,
				(new_len - row->length) * sizeof (GridElementPtr));
		row->length = new_len;
		row->data = new_elems;
	} else if (x2 < grid->offset) {
		// expand to left
		new_len = row->length + row->offset - x2;
		new_elems = new GridElementPtr[new_len];
		memcpy (&new_elems[row->offset - x2], row->data,
					row->length * sizeof (GridElementPtr));
		delete[] row->data;
		memset (new_elems, 0, (row->offset - x2) * sizeof (GridElementPtr));
		row->length = new_len;
		row->data = new_elems;
	}

	if (row->data[x2 - row->offset]) {
		// already something there!
		// TODO: throw exception?
		return;
	}

	row->data[x2 - row->offset] = element;
}


void GridData::Draw () const
{
	// TODO
	//GPlatesState::Layout::InsertLineDataPos(this, _line);
}


void GridData::RotateAndDraw (const GPlatesMaths::FiniteRotation &rot) const
{
	// TODO
	//GPlatesMaths::PolyLineOnSphere rot_line = (rot * _line);
	//GPlatesState::Layout::InsertLineDataPos(this, rot_line);
}
