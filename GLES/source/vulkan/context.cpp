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
 *  @file       context.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Context Functionality via Vulkan
 *
 *  @section
 *
 *  Before using Vulkan, an application must initialize it by loading the Vulkan
 *  commands, and creating a VkInstance object. Once Vulkan is initialized, devices
 *  and queues are the primary objects used to interact with a Vulkan implementation.
 *
 */

#include "context.h"
#include "cbManager.h"

namespace vulkanAPI {

#define GLOVE_VK_VALIDATION_LAYERS                      false

#ifdef VK_USE_PLATFORM_XCB_KHR
static const std::vector<const char*> requiredInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                                                    VK_KHR_XCB_SURFACE_EXTENSION_NAME};
#elif defined (VK_USE_PLATFORM_ANDROID_KHR)
static const std::vector<const char*> requiredInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                                                    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
#else // native
static const std::vector<const char*> requiredInstanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                                                    VK_KHR_DISPLAY_EXTENSION_NAME};
#endif

static const std::vector<const char*> requiredDeviceExtensions   = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static const std::vector<const char*> usefulDeviceExtensions     = {"VK_KHR_maintenance1"};

static       char **enabledInstanceLayers           = nullptr;

vkContext_t GloveVkContext;

bool InitVkLayers(uint32_t* nLayers);
bool CheckVkInstanceExtensions(void);
bool CheckVkDeviceExtensions(void);
bool CreateVkInstance(void);
bool EnumerateVkGpus(void);
bool InitVkQueueFamilyIndex(void);
bool CreateVkDevice(void);
bool CreateVkCommandPool(void);
bool CreateVkSemaphores(void);
void InitVkQueue(void);

bool
InitVkLayers(uint32_t* nLayers)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    VkResult res;
    uint32_t layerCount = 0;
    VkLayerProperties *vkLayerProperties = nullptr;

    do {
        res = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        if(!layerCount || res) {
            break;
        }

        vkLayerProperties = (VkLayerProperties *)realloc(vkLayerProperties, layerCount * sizeof(VkLayerProperties));
        if(!vkLayerProperties) {
            return false;
        }

        res = vkEnumerateInstanceLayerProperties(&layerCount, vkLayerProperties);
    } while(res == VK_INCOMPLETE);

    if(layerCount) {
        enabledInstanceLayers = (char**)malloc(layerCount * sizeof(char*));
    }

    *nLayers = layerCount;

    for(uint32_t i = 0; i < layerCount; ++i) {
        enabledInstanceLayers[i] = (char*)malloc(VK_MAX_EXTENSION_NAME_SIZE*sizeof(char));
        strcpy(enabledInstanceLayers[i], vkLayerProperties[i].layerName);
    }

    if(vkLayerProperties) {
        free(vkLayerProperties);
        vkLayerProperties = nullptr;
    }
    return true;
}

bool
CheckVkInstanceExtensions(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    VkResult res;
    uint32_t extensionCount = 0;
    VkExtensionProperties *vkExtensionProperties = nullptr;

    do {
        res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        if(!extensionCount || res) {
            break;
        }

        vkExtensionProperties = (VkExtensionProperties *)realloc(vkExtensionProperties, extensionCount * sizeof(*vkExtensionProperties));
        if(!vkExtensionProperties) {
            return false;
        }

        res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkExtensionProperties);
    } while(res == VK_INCOMPLETE);

    std::vector<bool> requiredExtensionsAvailable(requiredInstanceExtensions.size(), false);
    for(uint32_t i = 0; i < extensionCount; ++i) {
        for(uint32_t j = 0; j < requiredInstanceExtensions.size(); ++j) {
            if(!strcmp(requiredInstanceExtensions[j], vkExtensionProperties[i].extensionName)) {
                requiredExtensionsAvailable[j] = true;
                break;
            }
        }
    }

    if(vkExtensionProperties) {
        free(vkExtensionProperties);
        vkExtensionProperties = nullptr;
    }

    for(uint32_t j = 0; j < requiredInstanceExtensions.size(); ++j) {
        if(!requiredExtensionsAvailable[j]) {
            return false;
        }
    }

    return true;
}

bool
CheckVkDeviceExtensions(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    VkResult res;
    uint32_t extensionCount = 0;
    VkExtensionProperties *vkExtensionProperties = nullptr;

    do {
        res = vkEnumerateDeviceExtensionProperties(GloveVkContext.vkGpus[0], nullptr, &extensionCount, nullptr);

        if(!extensionCount || res) {
            break;
        }

        vkExtensionProperties = (VkExtensionProperties *)realloc(vkExtensionProperties, extensionCount * sizeof(*vkExtensionProperties));
        if(!vkExtensionProperties) {
            return false;
        }

        res = vkEnumerateDeviceExtensionProperties(GloveVkContext.vkGpus[0], nullptr, &extensionCount, vkExtensionProperties);
    } while(res == VK_INCOMPLETE);

    std::vector<bool> requiredExtensionsAvailable(requiredDeviceExtensions.size(), false);
    for(uint32_t i = 0; i < extensionCount; ++i) {
        for(uint32_t j = 0; j < requiredDeviceExtensions.size(); ++j) {
            if(!strcmp(requiredDeviceExtensions[j], vkExtensionProperties[i].extensionName)) {
                requiredExtensionsAvailable[j] = true;
                break;
            }
        }
    }

    GetContext()->mIsMaintenanceExtSupported = false;
    for(uint32_t i = 0; i < extensionCount; ++i) {
        for(uint32_t j = 0; j < usefulDeviceExtensions.size(); ++j) {
            if(!strcmp(usefulDeviceExtensions[j], vkExtensionProperties[i].extensionName)) {
                GetContext()->mIsMaintenanceExtSupported = true;
                break;
            }
        }
    }

    if(vkExtensionProperties) {
        free(vkExtensionProperties);
        vkExtensionProperties = nullptr;
    }

    for(uint32_t j = 0; j < requiredDeviceExtensions.size(); ++j) {
        if(!requiredExtensionsAvailable[j]) {
            printf("\n%s extension is mandatory for GLOVE\n", requiredDeviceExtensions[j]);
            printf("Please link GLOVE to a Vulkan driver which supports the latter\n");
            return false;
        }
    }

    return true;
}

bool
CreateVkInstance(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    uint32_t enabledLayerCount = 0;
    if(GLOVE_VK_VALIDATION_LAYERS) {
        if(InitVkLayers(&enabledLayerCount) == false) {
            return false;
        }
    }

    VkApplicationInfo applicationInfo;
    applicationInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext             = nullptr;
    applicationInfo.pApplicationName  = "GLOVE (GL Over Vulkan)\0";
    applicationInfo.applicationVersion= 1;
    applicationInfo.pEngineName       = "GLOVE (GL Over Vulkan)\0";
    applicationInfo.engineVersion     = 1;
    applicationInfo.apiVersion        = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo;
    memset(static_cast<void *>(&instanceInfo), 0 ,sizeof(instanceInfo));
    instanceInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext                    = nullptr;
    instanceInfo.flags                    = 0;
    instanceInfo.pApplicationInfo         = &applicationInfo;
    instanceInfo.enabledLayerCount        = enabledLayerCount;
    instanceInfo.ppEnabledLayerNames      = enabledInstanceLayers;
    instanceInfo.enabledExtensionCount    = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames  = requiredInstanceExtensions.data();

    VkResult err = vkCreateInstance(&instanceInfo, nullptr, &GloveVkContext.vkInstance);
    assert(!err);

    for(uint32_t i = 0; i < enabledLayerCount; ++i) {
       free(enabledInstanceLayers[i]);
    }
    free(enabledInstanceLayers);

    return (err == VK_SUCCESS);
}

bool
EnumerateVkGpus(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    uint32_t gpuCount;

    VkResult err;
    err = vkEnumeratePhysicalDevices(GloveVkContext.vkInstance, &gpuCount, nullptr);
    assert(!err);

    if(err != VK_SUCCESS) {
        return false;
    }

    GloveVkContext.vkGpus.resize(gpuCount);

    err = vkEnumeratePhysicalDevices(GloveVkContext.vkInstance, &gpuCount, GloveVkContext.vkGpus.data());
    assert(!err);

    if(err != VK_SUCCESS) {
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties(GloveVkContext.vkGpus[0], &GloveVkContext.vkDeviceMemoryProperties);

    return true;
}

bool
InitVkQueueFamilyIndex(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(GloveVkContext.vkGpus[0], &queueFamilyCount, nullptr);
    assert(queueFamilyCount >= 1);
    if(!queueFamilyCount) {
        return false;
    }

    VkQueueFamilyProperties *queueProperties = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(GloveVkContext.vkGpus[0], &queueFamilyCount, queueProperties);

    uint32_t i;
    for(i = 0; i < queueFamilyCount; ++i) {
        if(queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            GloveVkContext.vkGraphicsQueueNodeIndex = i;
            break;
        }
    }

    delete[] queueProperties;
    return i < queueFamilyCount ? true : false;
}

bool
CreateVkDevice(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queueInfo;
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext            = nullptr;
    queueInfo.flags            = 0;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = queue_priorities;
    queueInfo.queueFamilyIndex = GloveVkContext.vkGraphicsQueueNodeIndex;

    VkDeviceCreateInfo deviceInfo;
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext                   = nullptr;
    deviceInfo.flags                   = 0;
    deviceInfo.queueCreateInfoCount    = 1;
    deviceInfo.pQueueCreateInfos       = &queueInfo;
    deviceInfo.enabledLayerCount       = 0;
    deviceInfo.ppEnabledLayerNames     = nullptr;
    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
    deviceInfo.pEnabledFeatures        = nullptr;

    VkResult err = vkCreateDevice(GloveVkContext.vkGpus[0], &deviceInfo, nullptr, &GloveVkContext.vkDevice);
    assert(!err);

    return (err == VK_SUCCESS);
}

bool
CreateVkSemaphores(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    VkResult err;

    GloveVkContext.vkSyncItems = new vkSyncItems_t;

    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    err = vkCreateSemaphore(GloveVkContext.vkDevice, &semaphoreCreateInfo, nullptr, &GloveVkContext.vkSyncItems->vkDrawSemaphore);
    assert(!err);

    if(err != VK_SUCCESS) {
        return false;
    }

    err = vkCreateSemaphore(GloveVkContext.vkDevice, &semaphoreCreateInfo, nullptr, &GloveVkContext.vkSyncItems->vkAcquireSemaphore);
    assert(!err);

    if(err != VK_SUCCESS) {
        return false;
    }

    GloveVkContext.vkSyncItems->acquireSemaphoreFlag = true;
    GloveVkContext.vkSyncItems->drawSemaphoreFlag = false;

    return true;
}

void
InitVkQueue(void)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    vkGetDeviceQueue(GloveVkContext.vkDevice,
                     GloveVkContext.vkGraphicsQueueNodeIndex,
                     0,
                     &GloveVkContext.vkQueue);
}

vkContext_t *
GetContext()
{
    FUN_ENTRY(GL_LOG_DEBUG);

    return &GloveVkContext;
}

void
ResetContextResources()
{
    GloveVkContext.vkInstance                   = VK_NULL_HANDLE;
    GloveVkContext.vkGpus.clear();
    GloveVkContext.vkQueue                      = VK_NULL_HANDLE;
    GloveVkContext.vkGraphicsQueueNodeIndex     = 0;
    GloveVkContext.vkDevice                     = VK_NULL_HANDLE;
    GloveVkContext.vkSyncItems                  = nullptr;
    GloveVkContext.mIsMaintenanceExtSupported   = false;
    GloveVkContext.mInitialized                 = false;
    memset(static_cast<void*>(&GloveVkContext.vkDeviceMemoryProperties), 0,
           sizeof(VkPhysicalDeviceMemoryProperties));
}

bool
InitContext()
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if (GloveVkContext.mInitialized == true) {
        return true;
    }

    ResetContextResources();

    if( !CheckVkInstanceExtensions()  ||
        !CreateVkInstance()           ||
        !EnumerateVkGpus()            ||
        !InitVkQueueFamilyIndex()     ||
        !CheckVkDeviceExtensions()    ||
        !CreateVkDevice()             ||
        !CreateVkSemaphores()
      ) {
        assert(false);
        return false;
    }
    InitVkQueue();

    GloveVkContext.mInitialized = true;

    return GloveVkContext.mInitialized;
}

void
TerminateContext()
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(!GloveVkContext.mInitialized) {
        return;
    }

    if(GloveVkContext.vkSyncItems->vkAcquireSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(GloveVkContext.vkDevice, GloveVkContext.vkSyncItems->vkAcquireSemaphore, nullptr);
        GloveVkContext.vkSyncItems->vkAcquireSemaphore = VK_NULL_HANDLE;
    }

    if(GloveVkContext.vkSyncItems->vkDrawSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(GloveVkContext.vkDevice, GloveVkContext.vkSyncItems->vkDrawSemaphore, nullptr);
        GloveVkContext.vkSyncItems->vkDrawSemaphore = VK_NULL_HANDLE;
    }

    if(GloveVkContext.vkDevice != VK_NULL_HANDLE ) {
        vkDeviceWaitIdle(GloveVkContext.vkDevice);
        vkDestroyDevice(GloveVkContext.vkDevice, nullptr);
        vkDestroyInstance(GloveVkContext.vkInstance, nullptr);
    }

    SafeDelete(GloveVkContext.vkSyncItems);

    ResetContextResources();
}

};
