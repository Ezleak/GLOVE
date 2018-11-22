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

/**
 *  @file       WSIXcb.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      WSI XCB module. It gets VkSurface for XCB Window platform.
 *
 */

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "WSIXcb.h"
#include "api/eglDisplay.h"

EGLBoolean
WSIXcb::Initialize()
{
    FUN_ENTRY(DEBUG_DEPTH);

    if (VulkanWSI::Initialize() == EGL_FALSE) {
        return EGL_FALSE;
    }

    if (SetPlatformCallbacks() == EGL_FALSE) {
        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean
WSIXcb::SetPlatformCallbacks(void)
{
    FUN_ENTRY(DEBUG_DEPTH);

    memset(&mWsiXCBCallbacks, 0, sizeof(mWsiXCBCallbacks));

    // VK_KHR_xcb_surface functions
    GET_WSI_FUNCTION_PTR(mWsiXCBCallbacks, CreateXcbSurfaceKHR);

    return EGL_TRUE;
}

void
WSIXcb::GetXCBConnection(xcbContext *xcb)
{
    FUN_ENTRY(DEBUG_DEPTH);

    xcb->connection = nullptr;

    if(xcb->dpy->display_id == EGL_DEFAULT_DISPLAY) {
        int scr;

        if(!(xcb->connection = xcb_connect(nullptr, &scr))) {
            printf("xcb_connect failed.\n");
            fflush(stdout);
        }

        const xcb_setup_t *setup = xcb_get_setup(xcb->connection);
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
        while(scr-- > 0) {
            xcb_screen_next(&iter);
        }

        xcb->screen = iter.data;
    } else {
        // Get xcb connection from X display
        xcb->connection = XGetXCBConnection(xcb->dpy->display_id);
    }
}

VkSurfaceKHR
WSIXcb::CreateSurface(EGLDisplay_t* dpy, EGLNativeWindowType win, EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    if(!surface) {
        return VK_NULL_HANDLE;
    }

    xcbContext xcb = { dpy, nullptr, nullptr };
    GetXCBConnection(&xcb);

    if(!surface->GetWidth() || !surface->GetHeight()) {
        xcb_get_geometry_reply_t *winProps;
        winProps = xcb_get_geometry_reply(xcb.connection,
                                          xcb_get_geometry(xcb.connection, win),
                                          nullptr);
        assert(winProps);

        surface->SetWidth(winProps->width);
        surface->SetHeight(winProps->height);

        free(winProps);
    }

    VkSurfaceKHR vkSurface;
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
    memset(static_cast<void *>(&surfaceCreateInfo), 0 ,sizeof(surfaceCreateInfo));
    surfaceCreateInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext      = nullptr;
    surfaceCreateInfo.connection = xcb.connection;
    surfaceCreateInfo.window     = (xcb_window_t)win;

    if(VK_SUCCESS != mWsiXCBCallbacks.fpCreateXcbSurfaceKHR(mVkInterface->vkInstance, &surfaceCreateInfo, nullptr, &vkSurface)) {
        return VK_NULL_HANDLE;
    }

    return vkSurface;
}
#endif // VK_USE_PLATFORM_XCB_KHR
