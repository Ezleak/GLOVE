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
 *  @file       renderPass.h
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Render pass Functionality in Vulkan
 *
 */

#ifndef __VKRENDERPASS_H__
#define __VKRENDERPASS_H__

#include "context.h"

class CacheManager;

namespace vulkanAPI {

class RenderPass {

private:

    const
    vkContext_t *           mVkContext;

    const
    VkPipelineBindPoint     mVkPipelineBindPoint;
    VkRenderPass            mVkRenderPass;
    VkClearValue            mVkClearValues[2];
    VkRect2D                mVkRenderArea;

    VkBool32                mColorClearEnabled;
    VkBool32                mDepthClearEnabled;
    VkBool32                mStencilClearEnabled;

    VkBool32                mColorWriteEnabled;
    VkBool32                mDepthWriteEnabled;
    VkBool32                mStencilWriteEnabled;

    VkBool32                mStarted;

    uint64_t                mHash;
    CacheManager *          mCacheManager;

public:

// Constructor
    RenderPass(const vkContext_t *vkContext = nullptr);

// Destructor
    ~RenderPass();

// Begin/End functions
    void                    Begin   (VkCommandBuffer *activeCmdBuffer, VkFramebuffer *framebuffer, bool hasSecondary);

    bool                    End     (VkCommandBuffer *activeCmdBuffer);

// Create functions
    bool                    Create  (VkFormat colorFormat, VkFormat depthstencilFormat);

// Release functions
    void                    Release (void);

// Get functions
    inline VkBool32         GetColorClearEnabled(void)                    const { FUN_ENTRY(GL_LOG_TRACE); return mColorClearEnabled;   }
    inline VkBool32         GetDepthClearEnabled(void)                    const { FUN_ENTRY(GL_LOG_TRACE); return mDepthClearEnabled;   }
    inline VkBool32         GetStencilClearEnabled(void)                  const { FUN_ENTRY(GL_LOG_TRACE); return mStencilClearEnabled; }
    inline VkBool32         GetColorWriteEnabled(void)                    const { FUN_ENTRY(GL_LOG_TRACE); return mColorWriteEnabled;   }
    inline VkBool32         GetDepthWriteEnabled(void)                    const { FUN_ENTRY(GL_LOG_TRACE); return mDepthWriteEnabled;   }
    inline VkBool32         GetStencilWriteEnabled(void)                  const { FUN_ENTRY(GL_LOG_TRACE); return mStencilWriteEnabled; }
    inline VkRenderPass*    GetRenderPass(void)                                 { FUN_ENTRY(GL_LOG_TRACE); return &mVkRenderPass; }

// Set Functions
    inline void             SetVkContext(const vkContext_t *vkContext)          { FUN_ENTRY(GL_LOG_TRACE); mVkContext           = vkContext; }
    inline void             SetCacheManager(CacheManager *cacheManager)         { FUN_ENTRY(GL_LOG_TRACE); mCacheManager        = cacheManager; }
    inline void             SetColorClearEnabled(VkBool32 enable)               { FUN_ENTRY(GL_LOG_TRACE); mColorClearEnabled   = enable;    }
    inline void             SetDepthClearEnabled(VkBool32 enable)               { FUN_ENTRY(GL_LOG_TRACE); mDepthClearEnabled   = enable;    }
    inline void             SetStencilClearEnabled(VkBool32 enable)             { FUN_ENTRY(GL_LOG_TRACE); mStencilClearEnabled = enable;    }
    inline void             SetColorWriteEnabled(VkBool32 enable)               { FUN_ENTRY(GL_LOG_TRACE); mColorWriteEnabled   = enable;    }
    inline void             SetDepthWriteEnabled(VkBool32 enable)               { FUN_ENTRY(GL_LOG_TRACE); mDepthWriteEnabled   = enable;    }
    inline void             SetStencilWriteEnabled(VkBool32 enable)             { FUN_ENTRY(GL_LOG_TRACE); mStencilWriteEnabled = enable;    }

           void             SetClearArea(const VkRect2D *rect);
           void             SetClearColorValue(const float *value);
           void             SetClearDepthStencilValue(float depth, uint32_t stencil);
};

}

#endif // __VKCLEARPASS_H__
