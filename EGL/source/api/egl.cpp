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
 *  @file       egl.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Entry points for the EGL API calls
 *
 */

#include "display/displayDriversContainer.h"
#include "api/eglDisplay.h"
#include "display/displayDriver.h"
#include "utils/eglLogger.h"
#include "thread/renderingThread.h"
#include "utils/eglUtils.h"
#include "eglFunctions.h"

#ifdef DEBUG_DEPTH
#   undef DEBUG_DEPTH
#endif // DEBUG_DEPTH
#define DEBUG_DEPTH                          EGL_LOG_INFO

RenderingThread currentThread;

#define THREAD_EXEC_RETURN(func)             FUN_ENTRY(DEBUG_DEPTH);                                                      \
                                             return currentThread.func;

#define CHECK_BAD_DISPLAY(eglDisplayPtr, dpy, erroRetValue)                                                               \
                                             EGLDisplay_t *eglDisplayPtr = EGLDisplay_t::FindDisplay(dpy);                \
                                             if(EGLDisplay_t::CheckBadDisplay(eglDisplayPtr) == EGL_FALSE)                \
                                             { return erroRetValue; }

#define CHECK_UNINITIALIZED_DISPLAY(eglDriverPtr, eglDisplayPtr, erroRetValue)                                                         \
                                             DisplayDriver *eglDriverPtr = DisplayDriversContainer::FindDisplayDriver(eglDisplayPtr);  \
                                             if(DisplayDriver::CheckNonInitializedDisplay(eglDriver) == EGL_FALSE)                     \
                                             { return erroRetValue; }

#define CHECK_BAD_CONFIG(eglDriverPtr, eglConfigPtr, eglConfig, erroRetValue)                                             \
                                             EGLConfig_t *eglConfigPtr = static_cast<EGLConfig_t*>(eglConfig);            \
                                             if(eglDriverPtr->CheckBadConfig(eglConfigPtr) == EGL_FALSE)                  \
                                             { return erroRetValue; }

#define CHECK_BAD_SURFACE(eglDriverPtr, eglSurfacePtr, eglSurface, erroRetValue)                                          \
                                             EGLSurface_t *eglSurfacePtr = static_cast<EGLSurface_t*>(eglSurface);        \
                                             if(eglDriverPtr->CheckBadSurface(eglSurfacePtr) == EGL_FALSE)                \
                                             { return erroRetValue; }

#define CHECK_BAD_CONTEXT(eglContextPtr, eglContext, erroRetValue)                                                        \
                                             EGLContext_t *eglContextPtr = static_cast<EGLContext_t*>(eglContext);        \
                                             if(EGLContext_t::CheckBadContext(eglContextPtr) == EGL_FALSE)                \
                                             { return erroRetValue; }

static void cleanUpResources()
{
    FUN_ENTRY(EGL_LOG_DEBUG);

    if(DisplayDriversContainer::IsEmpty()) {
        DisplayDriversContainer::Destroy();
    };
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay(EGLNativeDisplayType display_id)
{
    FUN_ENTRY(DEBUG_DEPTH);

    EGLDisplay_t *eglDisplay = EGLDisplay_t::GetDisplayByID(display_id);
    return reinterpret_cast<EGLDisplay>(eglDisplay);
}

EGLAPI EGLint EGLAPIENTRY
eglGetError(void)
{
    THREAD_EXEC_RETURN(GetError());
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI(EGLenum api)
{
    THREAD_EXEC_RETURN(BindAPI(api));
}

EGLAPI EGLenum EGLAPIENTRY
eglQueryAPI(void)
{
    THREAD_EXEC_RETURN(QueryAPI());
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient(void)
{
    THREAD_EXEC_RETURN(WaitClient());
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread(void)
{
    THREAD_EXEC_RETURN(ReleaseThread());
}

EGLAPI EGLContext EGLAPIENTRY
eglGetCurrentContext(void)
{
    THREAD_EXEC_RETURN(GetCurrentContext());
}

EGLAPI EGLSurface EGLAPIENTRY
eglGetCurrentSurface(EGLint readdraw)
{
    THREAD_EXEC_RETURN(GetCurrentSurface(readdraw));
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay(void)
{
    THREAD_EXEC_RETURN(GetCurrentDisplay());
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_CONTEXT)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_CONTEXT)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_NO_CONTEXT)
    EGLContext_t* eglShareContext = static_cast<EGLContext_t*>(share_context);
    // TODO::check for valid context
    THREAD_EXEC_RETURN(CreateContext(eglDisplay, eglConfig, eglShareContext, attrib_list));
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_CONTEXT(eglContext, ctx, EGL_FALSE)
    THREAD_EXEC_RETURN(DestroyContext(eglDisplay, eglContext));
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    DisplayDriver *eglDriver = DisplayDriversContainer::FindDisplayDriver(eglDisplay);

    // check for non initialized display only if context/surfaces are not nullptr
    if((ctx != EGL_NO_CONTEXT || draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE) &&
        DisplayDriver::CheckNonInitializedDisplay(eglDriver) == EGL_FALSE){
        return EGL_FALSE;
    }

    EGLBoolean res = currentThread.MakeCurrent(eglDriver, eglDisplay, draw, read, ctx);
    if(res == EGL_TRUE) {
        eglDriver->SetActiveContext(ctx);
    }
    return res;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_CONTEXT(eglContext, ctx, EGL_FALSE)
    THREAD_EXEC_RETURN(QueryContext(eglDisplay, eglContext, attribute, value));
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL(void)
{
    THREAD_EXEC_RETURN(WaitGL());
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative(EGLint engine)
{
    THREAD_EXEC_RETURN(WaitNative(engine));
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    DisplayDriver *eglDriver = DisplayDriversContainer::AddDisplayDriver(eglDisplay);
    EGLBoolean res = eglDriver->Initialize(eglDisplay, major, minor);
    if(eglDriver->Initialized()) {
        EGLDisplay_t::InitializeDisplay(dpy, eglDriver);
    }
    return res;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglTerminate(EGLDisplay dpy)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    DisplayDriver *eglDriver = DisplayDriversContainer::FindDisplayDriver(eglDisplay);
    if(eglDriver == nullptr || !eglDriver->Initialized()) {
        return EGL_FALSE;
    }

    EGLBoolean res = eglDriver->Terminate(eglDisplay);
    DisplayDriversContainer::RemoveDisplayDriver(eglDisplay);
    cleanUpResources();
    if(res == EGL_TRUE) {
       EGLDisplay_t::TerminateDisplay(dpy);
    }

    return res;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString(EGLDisplay dpy, EGLint name)
{
    FUN_ENTRY(DEBUG_DEPTH);

    if(dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) {
        return getEGLClientExtensions();
    }

    CHECK_BAD_DISPLAY(eglDisplay, dpy, nullptr)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, nullptr)

    switch(name) {
    case EGL_CLIENT_APIS:   return "OpenGL_ES\0";
    case EGL_VENDOR:        return "GLOVE (GL Over Vulkan)\0";
    case EGL_VERSION:       return "1.4\0";
    case EGL_EXTENSIONS:    return eglDriver->GetExtensions();
    default:                { currentThread.RecordError(EGL_BAD_PARAMETER); return nullptr; }
    }
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->GetConfigs(eglDisplay, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->ChooseConfig(eglDisplay, attrib_list, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_FALSE)
    return eglDriver->GetConfigAttrib(eglDisplay, eglConfig, attribute, value);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_SURFACE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_SURFACE)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_NO_SURFACE)
    return eglDriver->CreateWindowSurface(eglDisplay, eglConfig, win, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_SURFACE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_SURFACE)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_NO_SURFACE)
    return eglDriver->CreatePbufferSurface(eglDisplay, eglConfig, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_SURFACE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_SURFACE)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_NO_SURFACE)
    return eglDriver->CreatePixmapSurface(eglDisplay, eglConfig, pixmap, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->DestroySurface(eglDisplay, eglSurface);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->QuerySurface(eglDisplay, eglSurface, attribute, value);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_SURFACE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_SURFACE)
    CHECK_BAD_CONFIG(eglDriver, eglConfig, config, EGL_NO_SURFACE)
    return eglDriver->CreatePbufferFromClientBuffer(eglDisplay, buftype, buffer, eglConfig, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->SurfaceAttrib(eglDisplay, eglSurface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->BindTexImage(eglDisplay, eglSurface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->ReleaseTexImage(eglDisplay, eglSurface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->SwapInterval(eglDisplay, interval);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->SwapBuffers(eglDisplay, eglSurface);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    CHECK_BAD_SURFACE(eglDriver, eglSurface, surface, EGL_FALSE)
    return eglDriver->CopyBuffers(eglDisplay, eglSurface, target);
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char *procname)
{
    FUN_ENTRY(DEBUG_DEPTH);

    // get EGL function pointers
    __eglMustCastToProperFunctionPointerType fp = GetEGLProcAddr(procname);
    if(fp != nullptr || strncmp(procname, "egl", 3) == 0) {
        return fp;
    }

    // get GL function pointers
    EGLenum enumAPI = currentThread.QueryAPI();
    if(enumAPI == EGL_OPENGL_ES_API) {
        // Assuming only GLES2 for now
        rendering_api_interface_t* api = nullptr;
        rendering_api_return_e ret = RENDERING_API_load_api(EGL_OPENGL_ES_API, EGL_GL_VERSION_2, &api);
        if(ret == RENDERING_API_LOAD_SUCCESS) {
            fp = api->get_proc_addr_cb(procname);
        }
        // remove the refCounter for this call
        RENDERING_API_terminate_gles2_api();
    } else {
        NOT_IMPLEMENTED();
    }

    return fp;
}

EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_IMAGE_KHR)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_IMAGE_KHR)
    CHECK_BAD_CONTEXT(eglContext, ctx, EGL_NO_IMAGE_KHR)
    return eglDriver->CreateImageKHR(eglDisplay, ctx, target, buffer, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->DestroyImageKHR(eglDisplay, image);
}

//TODO: Implement the KHR_fence_sync extension
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_NO_SYNC_KHR)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_NO_SYNC_KHR)
    return eglDriver->CreateSyncKHR(eglDisplay, type, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->DestroySyncKHR(eglDisplay, sync);
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CHECK_BAD_DISPLAY(eglDisplay, dpy, EGL_FALSE)
    CHECK_UNINITIALIZED_DISPLAY(eglDriver, eglDisplay, EGL_FALSE)
    return eglDriver->ClientWaitSyncKHR(eglDisplay, sync, flags, timeout);
}
