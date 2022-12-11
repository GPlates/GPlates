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

#ifndef GPLATES_OPENGL_VULKAN_MEMORY_ALLOCATOR_H
#define GPLATES_OPENGL_VULKAN_MEMORY_ALLOCATOR_H

//
// VMA memory allocator.
//
#define VMA_VULKAN_VERSION 1000000 // Vulkan 1.0
// Get VMA to fetch Vulkan function pointers dynamically using vkGetInstanceProcAddr/vkGetDeviceProcAddr.
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

#endif // GPLATES_OPENGL_VULKAN_MEMORY_ALLOCATOR_H
