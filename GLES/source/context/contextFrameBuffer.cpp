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
 *  @file       contextFrameBuffer.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      OpenGL ES API calls related to Framebuffer
 *
 */

#include "context.h"

void
Context::BindFramebuffer(GLenum target, GLuint framebuffer)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_FRAMEBUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    Framebuffer *fbo = mSystemFBO;
    if(framebuffer) {
        fbo = mResourceManager->GetFramebuffer(framebuffer);
        if(fbo->GetTarget() == GL_INVALID_VALUE) {
            fbo->SetTarget(target);
            fbo->SetxContext(mXContext);
            fbo->SetCommandBufferManager(mCommandBufferManager);
            fbo->SetResources(mResourceManager->GetTextureArray(), mResourceManager->GetRenderbufferArray());
            fbo->SetCacheManager(mCacheManager);
            fbo->SetWidth(mSystemFBO->GetWidth());
            fbo->SetHeight(mSystemFBO->GetHeight());
        }
    }

    if(mWriteFBO == fbo) {
        return;
    }

    if(mWriteFBO->IsInDrawState()) {
        Finish();
    }

    mWriteFBO = fbo;
    mWriteFBO->SetStateIdle();

    mStateManager.GetActiveObjectsState()->SetActiveFramebufferObjectID(framebuffer);
    if (mXContext->mIsMaintenanceExtSupported) {
        mPipeline->SetYInverted(mWriteFBO != mSystemFBO);
        GLenum frontFace = mStateManager.GetRasterizationState()->GetFrontFace();
        mPipeline->SetRasterizationFrontFace(GlFrontFaceToXFrontFace(frontFace));
    }
    mPipeline->SetUpdatePipeline(true);
    mPipeline->SetUpdateViewportState(true);
}

GLenum
Context::CheckFramebufferStatus(GLenum target)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_FRAMEBUFFER) {
        RecordError(GL_INVALID_ENUM);
        return 0;
    }

    return (mStateManager.GetActiveObjectsState()->IsDefaultFramebufferObjectActive()) ?
            GL_FRAMEBUFFER_COMPLETE :
            mResourceManager->GetFramebuffer(mStateManager.GetActiveObjectsState()->GetActiveFramebufferObjectID())->CheckStatus();
}

void
Context::DeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(n < 0) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    if(framebuffers == nullptr) {
        return;
    }

    while(n-- != 0) {
        uint32_t fboindex = *framebuffers++;

        if(fboindex && mResourceManager->FramebufferExists(fboindex)) {
            Framebuffer *fbo = mResourceManager->GetFramebuffer(fboindex);

            if(mWriteFBO == fbo) {

                if(mWriteFBO->IsInDrawState()) {
                    Finish();
                }

                mWriteFBO = mSystemFBO;
                mWriteFBO->SetStateIdle();

                mStateManager.GetActiveObjectsState()->SetActiveFramebufferObjectID(0);
                mPipeline->SetUpdatePipeline(true);
                mPipeline->SetUpdateViewportState(true);
            }

            mResourceManager->DeallocateFramebuffer(fboindex);
        }
    }
}

void
Context::FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_FRAMEBUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(renderbuffertarget != GL_RENDERBUFFER && renderbuffer) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(attachment != GL_COLOR_ATTACHMENT0 && attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if( mStateManager.GetActiveObjectsState()->IsDefaultFramebufferObjectActive() ||
      ( renderbuffer && !mResourceManager->RenderbufferExists(renderbuffer))) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    if(renderbuffer != mWriteFBO->GetAttachmentName(attachment) && mWriteFBO->IsInDrawState()) {
        Finish();
    }

    switch(attachment) {
    case GL_COLOR_ATTACHMENT0: {
        if (renderbuffer) {
            Texture *tex = mResourceManager->GetRenderbuffer(renderbuffer)->GetTexture();
            if (!(tex->GetVkImageUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
                tex->SetVkImageUsage(tex->GetVkImageUsage() | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
                tex->Allocate();
            }
            tex->PrepareVkImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        int width  = renderbuffer ? mResourceManager->GetRenderbuffer(renderbuffer)->GetTexture()->GetWidth()  : -1;
        int height = renderbuffer ? mResourceManager->GetRenderbuffer(renderbuffer)->GetTexture()->GetHeight() : -1;
        mWriteFBO->SetColorAttachment(width, height, renderbuffer ? mResourceManager->GetRenderbuffer(renderbuffer)->GetTexture() : nullptr);
        mWriteFBO->SetColorAttachmentType(renderbuffer ? GL_RENDERBUFFER : GL_NONE);
        mWriteFBO->SetColorAttachmentName(renderbuffer);
        mPipeline->SetUpdateViewportState(true);
        break; }
    case GL_DEPTH_ATTACHMENT:
        mWriteFBO->SetDepthAttachmentType(renderbuffer ? GL_RENDERBUFFER : GL_NONE);
        mWriteFBO->SetDepthAttachmentName(renderbuffer);
        break;
    case GL_STENCIL_ATTACHMENT:
        mWriteFBO->SetStencilAttachmentType(renderbuffer ? GL_RENDERBUFFER : GL_NONE);
        mWriteFBO->SetStencilAttachmentName(renderbuffer);
        break;
    }
}

void
Context::FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_FRAMEBUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(texture &&
       (textarget != GL_TEXTURE_2D && (textarget < GL_TEXTURE_CUBE_MAP_POSITIVE_X || textarget > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(texture && level) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    if(mStateManager.GetActiveObjectsState()->IsDefaultFramebufferObjectActive()) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    if(texture && !mResourceManager->TextureExists(texture)) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    if((mResourceManager->GetTexture(texture)->GetTarget() == GL_TEXTURE_2D       && textarget != GL_TEXTURE_2D) ||
       (mResourceManager->GetTexture(texture)->GetTarget() == GL_TEXTURE_CUBE_MAP && textarget == GL_TEXTURE_2D)) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    if(attachment != GL_COLOR_ATTACHMENT0 && attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(texture && texture != mWriteFBO->GetAttachmentName(attachment) && mWriteFBO->IsInDrawState()) {
        Finish();
    }

    switch(attachment) {
    case GL_COLOR_ATTACHMENT0: {
        if (texture) {
            Texture *tex = mResourceManager->GetTexture(texture);
            if (!(tex->GetVkImageUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
                tex->SetVkImageUsage(tex->GetVkImageUsage() | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
                tex->Allocate();
            }
            tex->PrepareVkImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        int width = texture ? mResourceManager->GetTexture(texture)->GetWidth() : -1;
        int height = texture ? mResourceManager->GetTexture(texture)->GetHeight() : -1;
        mWriteFBO->SetColorAttachment(width, height);
        mWriteFBO->SetColorAttachmentType(texture ? GL_TEXTURE : GL_NONE);
        mWriteFBO->SetColorAttachmentName(texture);
        mWriteFBO->SetColorAttachmentLayer(texture && mResourceManager->GetTexture(texture)->IsCubeMap() ? textarget : 0);
        mWriteFBO->SetColorAttachmentLevel(0);
        mPipeline->SetUpdateViewportState(true);
        break; }
    case GL_DEPTH_ATTACHMENT:
        mWriteFBO->SetDepthAttachmentType(texture ? GL_TEXTURE : GL_NONE);
        mWriteFBO->SetDepthAttachmentName(texture);
        mWriteFBO->SetDepthAttachmentLayer(texture && mResourceManager->GetTexture(texture)->IsCubeMap() ? textarget : 0);
        mWriteFBO->SetDepthAttachmentLevel(0);
        break;
    case GL_STENCIL_ATTACHMENT:
        mWriteFBO->SetStencilAttachmentType(texture ? GL_TEXTURE : GL_NONE);
        mWriteFBO->SetStencilAttachmentName(texture);
        mWriteFBO->SetStencilAttachmentLayer(texture && mResourceManager->GetTexture(texture)->IsCubeMap() ? textarget : 0);
        mWriteFBO->SetStencilAttachmentLevel(0);
        break;
    }
}

void
Context::GenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(n < 0) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    if(framebuffers == nullptr) {
        return;
    }

    while (n != 0) {
        *framebuffers++ = mResourceManager->AllocateFramebuffer();
        --n;
    }
}

void
Context::GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_FRAMEBUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(mStateManager.GetActiveObjectsState()->IsDefaultFramebufferObjectActive()) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    uint32_t activeFBOid = mStateManager.GetActiveObjectsState()->GetActiveFramebufferObjectID();
    Framebuffer *fbo = mResourceManager->GetFramebuffer(activeFBOid);

    GLenum type  = GL_NONE;
    GLenum name  = 0;
    GLint  level = 0;
    GLenum layer = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    switch(attachment) {
    case GL_COLOR_ATTACHMENT0:
        type  = fbo->GetColorAttachmentType();
        name  = fbo->GetColorAttachmentName();
        if(type == GL_TEXTURE) {
            level = fbo->GetColorAttachmentLevel();
            layer = fbo->GetColorAttachmentLayer();
        }
        break;
    case GL_DEPTH_ATTACHMENT:
        type  = fbo->GetDepthAttachmentType();
        name  = fbo->GetDepthAttachmentName();
        if(type == GL_TEXTURE) {
            level = fbo->GetDepthAttachmentLevel();
            layer = fbo->GetDepthAttachmentLayer();
        }
        break;
    case GL_STENCIL_ATTACHMENT:
        type = fbo->GetStencilAttachmentType();
        name = fbo->GetStencilAttachmentName();
        if(type == GL_TEXTURE) {
            level = fbo->GetStencilAttachmentLevel();
            layer = fbo->GetStencilAttachmentLayer();
        }
        break;
    default:
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(type == GL_NONE && pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(type == GL_RENDERBUFFER &&
      (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE   && pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME)) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(type == GL_TEXTURE &&
      (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE   && pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME        &&
       pname != GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL && pname != GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE)
      ) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    switch(pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:             *params = static_cast<GLint>(type);   break;
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:             *params = static_cast<GLint>(name);   break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:           *params = level;                      break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:   *params = static_cast<GLint>(layer);  break;
    }
}

GLboolean
Context::IsFramebuffer(GLuint framebuffer)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    return (framebuffer != 0 && mResourceManager->FramebufferExists(framebuffer)) ? GL_TRUE : GL_FALSE;
}
