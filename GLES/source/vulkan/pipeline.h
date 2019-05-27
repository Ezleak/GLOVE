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
 *  @file       pipeline.h
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      Graphics Pipeline Functionality in Vulkan
 *
 */

#ifndef __VKPIPELINE_H__
#define __VKPIPELINE_H__

#include "context.h"
#include "utils/cacheManager.h"

namespace vulkanAPI {

class Pipeline {
private:

    const
    vkContext_t *                               mVkContext;

    VkViewport                                  mVkViewport;
    VkRect2D                                    mVkScissorRect;
    VkPipeline                                  mVkPipeline;
    VkPipelineLayout                            mVkPipelineLayout;
    VkPipelineCache                             mVkPipelineCache;

    VkGraphicsPipelineCreateInfo                mVkPipelineInfo;
    VkPipelineInputAssemblyStateCreateInfo      mVkPipelineInputAssemblyState;

    VkPipelineColorBlendStateCreateInfo         mVkPipelineColorBlendState;
    VkPipelineColorBlendAttachmentState         mVkPipelineColorBlendAttachmentState;
    VkPipelineViewportStateCreateInfo           mVkPipelineViewportState;
    VkPipelineDepthStencilStateCreateInfo       mVkPipelineDepthStencilState;
    VkPipelineRasterizationStateCreateInfo      mVkPipelineRasterizationState;
    VkPipelineVertexInputStateCreateInfo       *mVkPipelineVertexInputState;
    VkPipelineMultisampleStateCreateInfo        mVkPipelineMultisampleState;

    std::vector<bool>                           mEnabledDynamicStatesList;
    VkDynamicState                              mVkPipelineDynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo            mVkPipelineDynamicState;

    int                                         mVkPipelineShaderStageIDs[2];
    uint32_t                                    mVkPipelineShaderStageCount;
    VkPipelineShaderStageCreateInfo             mVkPipelineShaderStages[2];

    struct {
    bool                                        Pipeline;
    bool                                        VertexAttribVBOs;
    bool                                        IndexBuffer;
    bool                                        Viewport;
    }                                           mUpdateState;

    bool                                        mYInverted;
    CacheManager                               *mCacheManager;

    bool                                        CreateGraphicsPipeline(void);
    void                                        Release(void);
    void                                        SetInfo(const VkRenderPass *renderpass);

public:
// Constructor
    Pipeline(const vkContext_t *vkContext);

// Destructor
    ~Pipeline();

// Get Functions
    inline int      * GetShaderStageIDsRef(void)                                { FUN_ENTRY(GL_LOG_TRACE); return mVkPipelineShaderStageIDs; }
    inline uint32_t & GetShaderStageCountRef(void)                              { FUN_ENTRY(GL_LOG_TRACE); return mVkPipelineShaderStageCount; }
    inline VkPipelineShaderStageCreateInfo * GetShaderStages(void)              { FUN_ENTRY(GL_LOG_TRACE); return mVkPipelineShaderStages; }

    inline bool GetUpdatePipelineState(void)                              const { FUN_ENTRY(GL_LOG_TRACE); return mUpdateState.Pipeline; }
    inline bool GetUpdateViewportState(void)                              const { FUN_ENTRY(GL_LOG_TRACE); return mUpdateState.Viewport; }
    inline bool GetUpdateVertexAttribVBOs(void)                           const { FUN_ENTRY(GL_LOG_TRACE); return mUpdateState.VertexAttribVBOs; }
    inline bool GetUpdateIndexBuffer(void)                                const { FUN_ENTRY(GL_LOG_TRACE); return mUpdateState.IndexBuffer; }

// Set Functions
    inline void SetUpdateIndexBuffer(bool enable)                               { FUN_ENTRY(GL_LOG_TRACE); mUpdateState.IndexBuffer      = enable; }
    inline void SetUpdateVertexAttribVBOs(bool enable)                          { FUN_ENTRY(GL_LOG_TRACE); mUpdateState.VertexAttribVBOs = enable; }
    inline void SetUpdateViewportState(bool enable)                             { FUN_ENTRY(GL_LOG_TRACE); mUpdateState.Viewport         = enable; }
    inline void SetUpdatePipeline(bool enable)                                  { FUN_ENTRY(GL_LOG_TRACE); mUpdateState.Pipeline         = enable; }

    inline void SetInputAssemblyTopology(VkPrimitiveTopology topology)          { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineInputAssemblyState.topology            = topology; mUpdateState.Pipeline = true;}
    inline void SetMultisampleAlphaToCoverage(VkBool32 enable)                  { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineMultisampleState.alphaToCoverageEnable = enable;   mUpdateState.Pipeline = true; }

    inline void SetRasterizationPolygonMode(VkPolygonMode mode)                 { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.polygonMode = mode; mUpdateState.Pipeline = true;}
    inline void SetRasterizationCullMode(VkBool32 enable,
                                         VkCullModeFlagBits mode)               { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.cullMode  = enable ? mode : VK_CULL_MODE_NONE; mUpdateState.Pipeline = true;}
    inline void SetRasterizationFrontFace(VkFrontFace face)                     { FUN_ENTRY(GL_LOG_TRACE); face = !mYInverted ? face : (face == VK_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE); mVkPipelineRasterizationState.frontFace = face; mUpdateState.Pipeline = true; }

    inline void SetRasterizationDepthBiasEnable(VkBool32 enable)                { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.depthBiasEnable         = enable; mUpdateState.Pipeline = true;}
    inline void SetRasterizationDepthBiasConstantFactor(float factor)           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.depthBiasConstantFactor = factor; mUpdateState.Pipeline = true;}
    inline void SetRasterizationDepthBiasSlopeFactor(float factor)              { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.depthBiasSlopeFactor    = factor; mUpdateState.Pipeline = true;}
    inline void SetRasterizationLineWidth(float lineWidth)                      { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineRasterizationState.lineWidth = lineWidth; mUpdateState.Pipeline = true;}

    inline void SetColorBlendAttachmentEnable(VkBool32 enable)                  { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.blendEnable = enable; mUpdateState.Pipeline = true; }
    inline void SetColorBlendConstants(float *color)                            { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendState.blendConstants[0] = color[0];
                                                                                                           mVkPipelineColorBlendState.blendConstants[1] = color[1];
                                                                                                           mVkPipelineColorBlendState.blendConstants[2] = color[2];
                                                                                                           mVkPipelineColorBlendState.blendConstants[3] = color[3];    mUpdateState.Pipeline = true;}
    inline void SetColorBlendAttachmentWriteMask(VkColorComponentFlags mask)    { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.colorWriteMask = mask; mUpdateState.Pipeline = true;}

    inline void SetColorBlendAttachmentSrcColorFactor(VkBlendFactor factor)     { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.srcColorBlendFactor = factor; mUpdateState.Pipeline = true;}
    inline void SetColorBlendAttachmentDstColorFactor(VkBlendFactor factor)     { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.dstColorBlendFactor = factor; mUpdateState.Pipeline = true;}
    inline void SetColorBlendAttachmentSrcAlphaFactor(VkBlendFactor factor)     { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.srcAlphaBlendFactor = factor; mUpdateState.Pipeline = true;}
    inline void SetColorBlendAttachmentDstAlphaFactor(VkBlendFactor factor)     { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.dstAlphaBlendFactor = factor; mUpdateState.Pipeline = true;}

    inline void SetColorBlendAttachmentColorOp(VkBlendOp op)                    { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.colorBlendOp = op; mUpdateState.Pipeline = true;}
    inline void SetColorBlendAttachmentAlphaOp(VkBlendOp op)                    { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineColorBlendAttachmentState.alphaBlendOp = op; mUpdateState.Pipeline = true;}

    inline void SetDepthTestEnable(VkBool32 enable)                             { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.depthTestEnable        = enable; mUpdateState.Pipeline = true;}
    inline void SetDepthWriteEnable(VkBool32 enable)                            { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.depthWriteEnable       = enable; mUpdateState.Pipeline = true;}
    inline void SetDepthCompareOp(VkCompareOp op)                               { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.depthCompareOp         = op;     mUpdateState.Pipeline = true;}
    inline void SetDepthBoundsTestEnable(VkBool32 enable)                       { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.depthBoundsTestEnable  = enable; mUpdateState.Pipeline = true;}
    inline void SetMinDepthBounds(float depth)                                  { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.minDepthBounds         = depth;  mUpdateState.Pipeline = true;}
    inline void SetMaxDepthBounds(float depth)                                  { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.maxDepthBounds         = depth;  mUpdateState.Pipeline = true;}

    inline void SetStencilTestEnable(VkBool32 enable)                           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.stencilTestEnable      = enable; mUpdateState.Pipeline = true;}

    inline void SetStencilBackFailOp(VkStencilOp op)                            { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.failOp      = op;     mUpdateState.Pipeline = true;}
    inline void SetStencilBackPassOp(VkStencilOp op)                            { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.passOp      = op;     mUpdateState.Pipeline = true;}
    inline void SetStencilBackZFailOp(VkStencilOp op)                           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.depthFailOp = op;     mUpdateState.Pipeline = true;}
    inline void SetStencilBackWriteMask(uint32_t mask)                          { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.writeMask   = mask;   mUpdateState.Pipeline = true;}
    inline void SetStencilBackCompareOp(VkCompareOp op)                         { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.compareOp   = op;     mUpdateState.Pipeline = true;}
    inline void SetStencilBackCompareMask(uint32_t mask)                        { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.compareMask = mask;   mUpdateState.Pipeline = true;}
    inline void SetStencilBackReference(uint32_t ref)                           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.back.reference   = ref;    mUpdateState.Pipeline = true;}

    inline void SetStencilFrontFailOp(VkStencilOp op)                           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.failOp      = op;    mUpdateState.Pipeline = true;}
    inline void SetStencilFrontPassOp(VkStencilOp op)                           { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.passOp      = op;    mUpdateState.Pipeline = true;}
    inline void SetStencilFrontZFailOp(VkStencilOp op)                          { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.depthFailOp = op;    mUpdateState.Pipeline = true;}
    inline void SetStencilFrontWriteMask(uint32_t mask)                         { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.writeMask   = mask;  mUpdateState.Pipeline = true;}
    inline void SetStencilFrontCompareOp(VkCompareOp op)                        { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.compareOp   = op;    mUpdateState.Pipeline = true;}
    inline void SetStencilFrontCompareMask(uint32_t mask)                       { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.compareMask = mask;  mUpdateState.Pipeline = true;}
    inline void SetStencilFrontReference(uint32_t ref)                          { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineDepthStencilState.front.reference   = ref;   mUpdateState.Pipeline = true;}

    inline void SetCache(VkPipelineCache cache)                                 { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineCache            = cache; }
    inline void SetLayout(VkPipelineLayout layout)                              { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineLayout           = layout; }
    inline void SetVertexInputState(
                            VkPipelineVertexInputStateCreateInfo *vertexInput)  { FUN_ENTRY(GL_LOG_TRACE); mVkPipelineVertexInputState = vertexInput; }
    inline void SetYInverted(bool yInverted)                                    { FUN_ENTRY(GL_LOG_TRACE); mYInverted = yInverted; }
    inline void SetCacheManager(CacheManager *cacheManager)                     { FUN_ENTRY(GL_LOG_TRACE); mCacheManager = cacheManager; }
           void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
           void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height);

// Create Functions
          void CreateInfo(void);
          void CreateInputAssemblyState(VkBool32 primitiveRestartEnable, VkPrimitiveTopology topology);
          void CreateRasterizationState(VkPolygonMode polygonMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace,
                                        VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasSlopeFactor, float depthBiasClamp,
                                        VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable);
          void CreateColorBlendState(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask,
                                     VkBlendFactor srcColorBlendFactor, VkBlendFactor dstColorBlendFactor, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor,
                                     VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkLogicOp logicOp, VkBool32 logicOpEnable, uint32_t attachmentCount, float *blendConstants);
          void CreateViewportState(uint32_t viewportCount, uint32_t scissorCount);
          void CreateDynamicState(const std::vector<VkDynamicState>& states);
          void CreateDepthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable,
                                       float minDepthBounds, float maxDepthBounds, VkBool32  stencilTestEnable, VkStencilOp backfailOp, VkStencilOp backpassOp,
                                       VkStencilOp backdepthFailOp, uint32_t backwriteMask, VkCompareOp backcompareOp, uint32_t backcompareMask, uint32_t backreference,
                                       VkStencilOp frontfailOp, VkStencilOp frontpassOp, VkStencilOp frontdepthFailOp, uint32_t frontwriteMask, VkCompareOp frontcompareOp,
                                       uint32_t  frontcompareMask, uint32_t frontreference);
          void CreateMultisampleState(VkBool32 alphaToOneEnable, VkBool32 alphaToCoverageEnable, VkSampleCountFlagBits rasterizationSamples, VkBool32 sampleShadingEnable, float minSampleShading);

// Compute Functions
          void ComputeViewport(int fboWidth, int fboHeight, int viewportX, int viewportY, int viewportW, int viewportH, float minDepth, float maxDepth);
          void ComputeScissor(int fboWidth, int fboHeight, int scissorX, int scissorY, int scissorW, int scissorH);

// Bind Functions
          void Bind(const VkCommandBuffer *CmdBuffer) const;

// Create Functions
          bool Create(const VkRenderPass *renderpass);
// Update Functions
          void UpdateDynamicState(const VkCommandBuffer *CmdBuffer, float lineWidth) const;
};

}

#endif // __VKPIPELINE_H__
