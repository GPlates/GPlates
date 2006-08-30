/* $Id$ */

#define FAST_GRID_INIT

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004 The GPlates Consortium
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
 */

#include <cstring>
#include "DrawableData.h"
#include "GridData.h"
#include "GridElement.h"
#include "global/types.h"
#include "global/InvalidParametersException.h"
#include "maths/GridOnSphere.h"
#include "maths/PointOnSphere.h"
#include "state/Layout.h"


GPlatesGeo::GridData::GridData
			(const DataType_t &dt, const RotationGroupId_t &id,
			const TimeWindow &tw,
			const std::string &first_header_line,
			const std::string &second_header_line,
			const Attributes_t &attrs, 
			const GPlatesMaths::PointOnSphere &origin,
			const GPlatesMaths::PointOnSphere &sc_step,
			const GPlatesMaths::PointOnSphere &gc_step)
	: DrawableData (dt, id, tw, first_header_line, second_header_line,
	   attrs),
	  _lattice (GPlatesMaths::GridOnSphere::Create
		    (origin, sc_step, gc_step))
{
	grid = new Grid;
	grid->offset = 0;
	grid->length = 0;

	min_val = max_val = 0.0;
}

GPlatesGeo::GridData::~GridData ()
{
	if (grid->length < 1)
		return;
	for (index_t i = 0; i < grid->length; ++i) {
		GridRowPtr row = grid->rows[i];
		if (!row || !row->data || row->length < 1)
			continue;
		for (index_t j = 0; j < row->length; ++j)
			delete row->data[j];
		delete[] row->data;
		delete row;
	}
	delete[] grid->rows;

	delete grid;
}

void GPlatesGeo::GridData::Add_Elem (GridElement *element, index_t x1, index_t x2)
{
	index_t new_len;
	GridRowPtr *new_rows;
	GridElementPtr *new_elems;

	if (grid->length == 0) {
		// First value
		grid->offset = x1;
		min_val = max_val = element->getValue ();
	}

	// expand out to the correct row
	if (x1 >= (grid->offset + grid->length)) {
		// expand to right
		new_len = x1 - grid->offset + 1;
		new_rows = new GridRowPtr[new_len];
#ifndef FAST_GRID_INIT
		for (index_t i = 0; i < new_len; ++i)
			new_rows[i] = 0;
#else
		memset (new_rows, 0, new_len * sizeof (GridRowPtr));
#endif
		if (grid->length > 0) {
#ifndef FAST_GRID_INIT
			for (index_t i = 0; i < grid->length; ++i)
				new_rows[i] = grid->rows[i];
#else
			memcpy (new_rows, grid->rows, grid->length *
							sizeof (GridRowPtr));
#endif
			delete[] grid->rows;
		}
		grid->length = new_len;
		grid->rows = new_rows;
	} else if (x1 < grid->offset) {
		// expand to left
		new_len = grid->length + grid->offset - x1;
		new_rows = new GridRowPtr[new_len];
#ifndef FAST_GRID_INIT
		for (index_t i = 0; i < new_len; ++i)
			new_rows[i] = 0;
		for (index_t i = 0; i < grid->length; ++i)
			new_rows[i + (grid->offset - x1)] = grid->rows[i];
#else
		memset (new_rows, 0, new_len * sizeof (GridRowPtr));
		memcpy (&new_rows[grid->offset - x1], grid->rows, grid->length *
							sizeof (GridRowPtr));
#endif
		delete[] grid->rows;
		grid->length = new_len;
		grid->rows = new_rows;
		grid->offset = x1;
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
#ifndef FAST_GRID_INIT
		for (index_t i = 0; i < new_len; ++i)
			new_elems[i] = 0;
#else
		memset (new_elems, 0, new_len * sizeof (GridElementPtr));
#endif
		if (row->length > 0) {
#ifndef FAST_GRID_INIT
			for (index_t i = 0; i < row->length; ++i)
				new_elems[i] = row->data[i];
#else
			memcpy (new_elems, row->data, row->length *
						sizeof (GridElementPtr));
#endif
			delete[] row->data;
		}
		row->length = new_len;
		row->data = new_elems;
	} else if (x2 < row->offset) {
		// expand to left
		new_len = row->length + row->offset - x2;
		new_elems = new GridElementPtr[new_len];
#ifndef FAST_GRID_INIT
		for (index_t i = 0; i < new_len; ++i)
			new_elems[i] = 0;
		for (index_t i = 0; i < row->length; ++i)
			new_elems[i + (row->offset - x2)] = row->data[i];
#else
		memset (new_elems, 0, new_len * sizeof (GridElementPtr));
		memcpy (&new_elems[row->offset - x2], row->data,
					row->length * sizeof (GridElementPtr));
#endif
		delete[] row->data;
		row->length = new_len;
		row->data = new_elems;
		row->offset = x2;
	}

	if (row->data[x2 - row->offset]) {
		// already something there!
		// TODO: throw exception?
		delete element;
		return;
	}

	row->data[x2 - row->offset] = element;
	float val = element->getValue ();
	if (min_val > val)
		min_val = val;
	if (max_val < val)
		max_val = val;
}


void GPlatesGeo::GridData::Draw ()
{
	// TODO
	//GPlatesState::Layout::InsertLineDataPos(this, _line);
}


void GPlatesGeo::GridData::RotateAndDraw (const GPlatesMaths::FiniteRotation
								&rot)
{
	// TODO
	//GPlatesMaths::PolyLineOnSphere rot_line = (rot * _line);
	//GPlatesState::Layout::InsertLineDataPos(this, rot_line);
}

const GPlatesGeo::GridElement *GPlatesGeo::GridData::get (index_t x, index_t y)
									const
{
	if ((x < grid->offset) || (x >= (grid->offset + grid->length)))
		return 0;
	GridRowPtr row = grid->rows[x - grid->offset];
	if (!row)
		return 0;
	if ((y < row->offset) || (y >= (row->offset + row->length)))
		return 0;
	return row->data[y - row->offset];
}

void GPlatesGeo::GridData::getDimensions (index_t &x_sz, index_t &y_sz) const
{
	x_sz = grid->offset + grid->length;

	y_sz = 0;
	for (index_t i = 0; i < grid->length; ++i) {
		GridRowPtr row = grid->rows[i];
		index_t row_sz = row->offset + row->length;
		if (row_sz > y_sz)
			y_sz = row_sz;
	}
}
