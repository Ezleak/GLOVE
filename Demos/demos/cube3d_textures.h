/**
 * Copyright (C) 2015-2018 Think Silicon S.A. (https://think-silicon.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public v3
 * License as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */

#ifndef __CUBE3D_TEXTURES_H_
#define __CUBE3D_TEXTURES_H_

#include "../engine/glcore/common.h"
#include "../assets/geometry/cube.h"

#define VERTEX_SHADER_NAME          SOURCES_PATH SHADERS_PATH "geometry3d_textures.vert"
#define FRAGMENT_SHADER_NAME        SOURCES_PATH SHADERS_PATH "geometry3d_textures.frag"
#define BINARY_PROGRAM_SHADER_NAME  SOURCES_PATH SHADERS_PATH "geometry3d_textures.bin"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
static const char* diffuse_textures[] = { "../assets/textures/tsi_256x256.tga", "../assets/textures/vulkan_512x512.dds" };
#elif defined(VK_USE_PLATFORM_IOS_MVK)
static const char* diffuse_textures [] = { "assets/textures/tsi_256x256.tga", "assets/textures/vulkan_512x512.tga"};
#else
static const char* diffuse_textures [] = { "../assets/textures/tsi_256x256.tga", "../assets/textures/vulkan_512x512.tga"};
#endif
static const char* material_titles  [] = { "OPAQUE", "TRANSPARENT" };
static const char* projection_titles[] = { "ORTHOGRAPHIC", "PERSPECTIVE" };
static const char* shading_titles   [] = { "COLOR_TEXTURES"};

#endif // __CUBE3D_TEXTURES_H_
