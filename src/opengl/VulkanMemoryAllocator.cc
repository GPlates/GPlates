/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include "VulkanMemoryAllocator.h"

// Enable VMA implementation. Only one '.cc' file should define this.
//
// Note: It's OK that this header is also included in our "VulkanMemoryAllocator.h" because the
//       VMA_IMPLEMENTATION part of the header doesn't not have an include guard around it.
//
// NOTE: We include this AFTER "VulkanMemoryAllocator.h" since that contains some VMA preprocessor defines
//       that determine the behaviour of VMA (and we don't want to repeat them here).
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
