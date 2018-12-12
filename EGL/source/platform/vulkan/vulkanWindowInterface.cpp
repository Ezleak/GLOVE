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
 *  @file       vulkanwindowInterface.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Vulkan Window Interface functionality (implements PlatformWindowInterface). It connects EGL to the WSI module of Vulkan driver
 *
 */

#include "vulkanWindowInterface.h"

VulkanWindowInterface::VulkanWindowInterface(void)
: mVkInitialized(false), mGLES2Interface(nullptr), mVkAPI(nullptr), mVkWSI(nullptr)
{
    FUN_ENTRY(DEBUG_DEPTH);
}

VulkanWindowInterface::~VulkanWindowInterface(void)
{
    FUN_ENTRY(DEBUG_DEPTH);

    delete mVkWSI;
}

EGLBoolean
VulkanWindowInterface::InitializeVulkanAPI()
{
    FUN_ENTRY(DEBUG_DEPTH);

    if(!mVkInitialized) {
        rendering_api_return_e ret = RENDERING_API_init_api(EGL_OPENGL_ES_API, EGL_GL_VERSION_2, &mGLES2Interface);
        if(ret != RENDERING_API_INIT_SUCCESS) {
            return EGL_FALSE;
        }

        mVkInterface = reinterpret_cast<vkInterface_t *>(mGLES2Interface->state);

        mVkAPI = new VulkanAPI(mVkInterface);

        mVkWSI->SetVkInterface(mVkInterface);

        if(mVkWSI->Initialize() == EGL_FALSE) {
            return EGL_FALSE;
        }

        mVkAPI->SetWSICallbacks(mVkWSI->GetWsiCallbacks());

        mVkInitialized = true;
    }

    return EGL_TRUE;
}

void
VulkanWindowInterface::TerminateVulkanAPI()
{
    FUN_ENTRY(DEBUG_DEPTH);

    if(mVkAPI) {
        RENDERING_API_terminate_gles2_api();
        delete mVkAPI;
    }
}

EGLBoolean
VulkanWindowInterface::InitSwapchainExtension(const EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    return mVkAPI->DoesSupportPresent(dynamic_cast<const VulkanResources *>(surface->GetPlatformResources())) ? EGL_TRUE : EGL_FALSE;
}

VkExtent2D
VulkanWindowInterface::SetSwapchainExtent(EGLSurface_t* surface, VkSurfaceCapabilitiesKHR *surfCapabilities)
{
    FUN_ENTRY(DEBUG_DEPTH);

    EGLBoolean ASSERT_ONLY res;

    res = mVkAPI->GetPhysicalDevSurfaceCapabilities(dynamic_cast<const VulkanResources *>(surface->GetPlatformResources()), surfCapabilities);
    assert(res == EGL_TRUE);

    VkExtent2D swapChainExtent;
    if(surfCapabilities->currentExtent.width == (uint32_t)-1) {
        swapChainExtent.width = surface->GetWidth();
        swapChainExtent.height = surface->GetHeight();
    } else {
        swapChainExtent = surfCapabilities->currentExtent;
        surface->SetWidth(swapChainExtent.width);
        surface->SetHeight(swapChainExtent.height);
    }

    return swapChainExtent;
}

VkPresentModeKHR
VulkanWindowInterface::SetSwapchainPresentMode(EGLSurface_t* surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    EGLBoolean ASSERT_ONLY res;
    uint32_t presentModeCount;
    const VulkanResources *vkResources = dynamic_cast<const VulkanResources *>(surface->GetPlatformResources());
    assert(vkResources);

    presentModeCount = mVkAPI->GetPhysicalDevPresentModesCount(vkResources);
    assert(presentModeCount);

    VkPresentModeKHR *presentModes = new VkPresentModeKHR[presentModeCount];
    assert(presentModes);

    res = mVkAPI->GetPhysicalDevPresentModes(vkResources, presentModeCount, presentModes);
    assert(res == EGL_TRUE);

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    //Select the appropriate present mode
    if(surface->GetSwapInterval() == 0) {
        for(size_t i = 0; i < presentModeCount; i++) {
            //VK_PRESENT_MODE_MAILBOX_KHR, if supported, is prefered to VK_PRESENT_MODE_IMMEDIATE_KHR
            if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR &&
               swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
    delete[] presentModes;

    return swapchainPresentMode;
}

void
VulkanWindowInterface::SetSurfaceColorFormat(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    /// Get the list of VkFormats that are supported:
    uint32_t formatCount;
    EGLBoolean ASSERT_ONLY res = EGL_FALSE;
    const VulkanResources *vkResources = dynamic_cast<const VulkanResources *>(surface->GetPlatformResources());
    assert(vkResources);

    formatCount = mVkAPI->GetPhysicalDevFormatsCount(vkResources);
    assert(formatCount);

    VkSurfaceFormatKHR *surfFormats = new VkSurfaceFormatKHR[formatCount];
    res = mVkAPI->GetPhysicalDevFormats(vkResources, formatCount, surfFormats);
    assert(res == EGL_TRUE);

    VkFormat format = VK_FORMAT_UNDEFINED;
    if(formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        format = mVkDefaultFormat;
    } else {
        assert(formatCount >= 1);
        for(uint32_t i = 0; i < formatCount; ++i) {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
            if (surfFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB && surfFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                format = surfFormats[i].format;
                break;
            }
#else
            if (surfFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surfFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                format = surfFormats[i].format;
                break;
            }
#endif
        }
    }
    delete[] surfFormats;

    assert(res != VK_FORMAT_UNDEFINED);

    /// Query if the selected format is actually supported by the physical device
    VkFormatProperties formatDeviceProps;
    mVkAPI->GetPhysicalDevFormatProperties(format, &formatDeviceProps);

    if(!(formatDeviceProps.linearTilingFeatures  & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) &&
       !(formatDeviceProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
        format = VK_FORMAT_UNDEFINED;
    }
    assert(res != VK_FORMAT_UNDEFINED);


    surface->SetColorFormat(static_cast<EGLint>(format));
}

void
VulkanWindowInterface::CreateVkSwapchain(EGLSurface_t* surface,
                                   VkPresentModeKHR swapchainPresentMode,
                                   VkExtent2D swapChainExtent,
                                   VkSurfaceCapabilitiesKHR surfCapabilities)
{
    FUN_ENTRY(DEBUG_DEPTH);

    EGLBoolean ASSERT_ONLY wsiSuccess;
    /// Determine number of buffers
    assert(surfCapabilities.minImageCount >= 1);
    uint32_t desiredNumberOfSwapChainImages = 2;

    VulkanResources *vkResources = dynamic_cast<VulkanResources *>(surface->GetPlatformResources());
    assert(vkResources);

    VkSwapchainKHR vkSwapchain = mVkAPI->CreateSwapchain(vkResources,
                                                         desiredNumberOfSwapChainImages,
                                                         surfCapabilities,
                                                         swapChainExtent,
                                                         swapchainPresentMode,
                                                         static_cast<VkFormat>(surface->GetColorFormat()),
                                                         VK_NULL_HANDLE);
    assert(vkSwapchain != VK_NULL_HANDLE);

    vkResources->SetSwapchain(vkSwapchain);
}

EGLBoolean
VulkanWindowInterface::CreateSurface(EGLDisplay_t* dpy, EGLNativeWindowType win, EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    VkSurfaceKHR newSurface = mVkWSI->CreateSurface(dpy, win, surface);

    if(VK_NULL_HANDLE == newSurface) {
        return EGL_FALSE;
    }

    VulkanResources *vkResources = dynamic_cast<VulkanResources *>(surface->GetPlatformResources());
    if(vkResources) {
        vkResources->SetSurface(newSurface);
    }

    return EGL_TRUE;
}

void
VulkanWindowInterface::CreateSwapchain(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    // Is present supported?
    if(InitSwapchainExtension(surface) == EGL_FALSE) {
        assert(0);
        return;
    }

    VkSurfaceCapabilitiesKHR surfCapabilities = {};
    VkExtent2D swapChainExtent = SetSwapchainExtent(surface, &surfCapabilities);
    VkPresentModeKHR swapchainPresentMode = SetSwapchainPresentMode(surface);
    SetSurfaceColorFormat(surface);
    CreateVkSwapchain(surface, swapchainPresentMode, swapChainExtent, surfCapabilities);
}

void
VulkanWindowInterface::AllocateSurfaceImages(EGLSurface_t* surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    CreateSwapchain(surface);

    EGLBoolean ASSERT_ONLY wsiSuccess;
    uint32_t swapChainImageCount = 0;
    VkImage *swapChainImages = nullptr;
    VulkanResources *vkResources = dynamic_cast<VulkanResources *>(surface->GetPlatformResources());
    assert(vkResources);

    swapChainImageCount = mVkAPI->GetSwapChainImagesCount(vkResources);
    assert(swapChainImageCount);

    swapChainImages = new VkImage[swapChainImageCount]();
    assert(swapChainImages);

    wsiSuccess = mVkAPI->GetSwapChainImages(vkResources, swapChainImageCount, swapChainImages);
    assert(EGL_TRUE == wsiSuccess);

    vkResources->SetSwapChainImageCount(swapChainImageCount);
    vkResources->SetSwapChainImages(swapChainImages);
}

EGLBoolean
VulkanWindowInterface::AcquireNextImage(EGLSurface_t *surface, uint32_t *imageIndex)
{
    FUN_ENTRY(DEBUG_DEPTH);

    const VulkanResources *vkResources = dynamic_cast<const VulkanResources *>(surface->GetPlatformResources());
    if(vkResources == nullptr) {
        return EGL_FALSE;
    }

    VkResult res = mVkAPI->AcquireNextImage(vkResources, imageIndex);
    if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        mVkAPI->DeviceWaitIdle();

        return EGL_FALSE;
    }

    surface->SetCurrentImageIndex(*imageIndex);

    return EGL_TRUE;
}

void
VulkanWindowInterface::DestroySwapchain(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    VulkanResources *vkResources = dynamic_cast<VulkanResources *>(surface->GetPlatformResources());
    if(vkResources && vkResources->GetSwapchain() != VK_NULL_HANDLE) {
        mVkAPI->DestroySwapchain(vkResources);
        vkResources->SetSwapchain(VK_NULL_HANDLE);
    }

    if(vkResources) {
        vkResources->Release();
    }
}

void
VulkanWindowInterface::DestroySurface(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    VulkanResources *vkResources = dynamic_cast<VulkanResources *>(surface->GetPlatformResources());
    if(vkResources && vkResources->GetSurface() != VK_NULL_HANDLE) {
        mVkAPI->DestroyPlatformSurface(vkResources);
        vkResources->SetSurface(VK_NULL_HANDLE);
        mGLES2Interface->delete_shared_surface_data_cb(surface->GetEGLSurfaceInterface());
    }
}

void
VulkanWindowInterface::DestroySurfaceImages(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    DestroySwapchain(surface);
}

EGLBoolean
VulkanWindowInterface::PresentImage(EGLSurface_t *surface)
{
    FUN_ENTRY(DEBUG_DEPTH);

    std::vector<VkSemaphore> pSems;
    if(mVkInterface->vkSyncItems->drawSemaphoreFlag) {
        pSems.push_back(mVkInterface->vkSyncItems->vkDrawSemaphore);
    } else {
        pSems.push_back(mVkInterface->vkSyncItems->vkAcquireSemaphore);
    }

    mVkInterface->vkSyncItems->acquireSemaphoreFlag = true;
    mVkInterface->vkSyncItems->drawSemaphoreFlag = false;

    uint32_t imageIndex = surface->GetCurrentImageIndex();
    VkResult res = mVkAPI->PresentImage(dynamic_cast<const VulkanResources *>(surface->GetPlatformResources()), imageIndex, pSems);
    if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        mVkAPI->DeviceWaitIdle();

        return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLBoolean
VulkanWindowInterface::Initialize()
{
    FUN_ENTRY(DEBUG_DEPTH);

    return InitializeVulkanAPI();
}

EGLBoolean
VulkanWindowInterface::Terminate()
{
    FUN_ENTRY(DEBUG_DEPTH);

    TerminateVulkanAPI();

    return EGL_TRUE;
}
