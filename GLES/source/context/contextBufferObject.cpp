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
 *  @file       contextBufferObject.cpp
 *  @author     Think Silicon
 *  @date       25/07/2018
 *  @version    1.0
 *
 *  @brief      OpenGL ES API calls related to Buffer Objects
 *
 */

#include "context.h"

void
Context::BindBuffer(GLenum target, GLuint buffer)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    BufferObject *bo = nullptr;
    if(buffer) {
        bo = mResourceManager->GetBuffer(buffer);
        bo->SetTarget(target);
        bo->SetVkContext(mVkContext);
    }
    mStateManager.GetActiveObjectsState()->SetActiveBufferObject(target, bo);

    if(target == GL_ELEMENT_ARRAY_BUFFER || (bo && bo->IsIndexBuffer())) {
        mPipeline->SetUpdateIndexBuffer(true);
    }
}

void
Context::BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(usage != GL_STREAM_DRAW && usage != GL_STATIC_DRAW && usage != GL_DYNAMIC_DRAW) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(size < 0) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    BufferObject *bo = mStateManager.GetActiveObjectsState()->GetActiveBufferObject(target);
    if(!bo) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    bo->SetUsage(usage);
    if((data && bo->HasData()) || (data == nullptr && bo->GetSize() && (size_t)size != bo->GetSize())) {
        bo->Release();
    }

    if(!bo->Allocate(size, data)) {
        RecordError(GL_OUT_OF_MEMORY);
        return;
    }

    if(target == GL_ELEMENT_ARRAY_BUFFER || bo->IsIndexBuffer()) {
        mPipeline->SetUpdateIndexBuffer(true);
    }
}

void
Context::BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    BufferObject *bo = mStateManager.GetActiveObjectsState()->GetActiveBufferObject(target);
    if(!bo) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    if(size < 0 || offset < 0 || (uint32_t)size + (uint32_t)offset > (uint32_t)bo->GetSize()) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    bo->UpdateData(size, offset, data);

    if(target == GL_ELEMENT_ARRAY_BUFFER || bo->IsIndexBuffer()) {
        mPipeline->SetUpdateIndexBuffer(true);
    }
}

void
Context::DeleteBuffers(GLsizei n, const GLuint* buffers)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(n < 0) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    if(buffers == nullptr) {
        return;
    }

    if(mWriteFBO->IsInDrawState()) {
        Finish();
    }

    while(n-- != 0) {
        uint32_t buffer = *buffers++;

        if(buffer && mResourceManager->BufferExists(buffer)) {

            BufferObject *buf = mResourceManager->GetBuffer(buffer);

            if(mStateManager.GetActiveObjectsState()->EqualsActiveBufferObject(buf)) {
                mStateManager.GetActiveObjectsState()->ResetActiveBufferObject(buf->GetTarget());
            }

            mResourceManager->DeallocateBuffer(buffer);
        }
    }
}

void
Context::GetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    if(pname != GL_BUFFER_SIZE && pname != GL_BUFFER_USAGE) {
        RecordError(GL_INVALID_ENUM);
        return;
    }

    BufferObject *bo = mStateManager.GetActiveObjectsState()->GetActiveBufferObject(target);
    if(!bo) {
        RecordError(GL_INVALID_OPERATION);
        return;
    }

    switch(pname) {
    case GL_BUFFER_SIZE:  *params = static_cast<GLint>(bo->GetSize());  break;
    case GL_BUFFER_USAGE: *params = static_cast<GLint>(bo->GetUsage()); break;
    }
}

void
Context::GenBuffers(GLsizei n, GLuint* buffers)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    if(n < 0) {
        RecordError(GL_INVALID_VALUE);
        return;
    }

    if(buffers == nullptr) {
        return;
    }

    while(n != 0) {
        *buffers++ = mResourceManager->AllocateBuffer();
        --n;
    }
}

GLboolean
Context::IsBuffer(GLuint buffer)
{
    FUN_ENTRY(GL_LOG_DEBUG);

    return (buffer != 0 && mResourceManager->BufferExists(buffer)) ? GL_TRUE : GL_FALSE;
}
