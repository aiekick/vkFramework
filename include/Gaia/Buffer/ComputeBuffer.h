/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#pragma warning(disable : 4251)

#include <set>
#include <string>
#include <memory>

#include <ezlibs/ezTools.hpp>
#include <Gaia/gaia.h>

#include <vulkan/vulkan.hpp>
#include <Gaia/Resources/Texture2D.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Core/VulkanDevice.h>
#include <Gaia/Shader/VulkanShader.h>

#include <Gaia/Resources/VulkanRessource.h>
#include <Gaia/Resources/VulkanFrameBuffer.h>
#include <Gaia/Interfaces/OutputSizeInterface.h>
#include <Gaia/gaia.h>

class GAIA_API ComputeBuffer : public OutputSizeInterface {
public:
    static ComputeBufferPtr Create(GaiApi::VulkanCoreWeak vVulkanCore);

protected:
    uint32_t m_BufferIdToResize = 0U;     // buffer id to resize (mostly used in compute, because in pixel, all attachments must have same size)
    bool m_IsRenderPassExternal = false;  // true if the renderpass is not created here, but come from external (inportant for not destroy him)

    bool m_PingPongBufferMode = false;

    bool m_NeedResize = false;   // will be resized if true
    bool m_Loaded = false;       // if shader operationnel
    bool m_JustReseted = false;  // when shader was reseted
    bool m_FirstRender = true;   // 1er rendu

    uint32_t m_CountBuffers = 0U;  // count buffers

    ez::ivec2 m_TemporarySize;           // temporary size before resize can be used by imgui
    int32_t m_TemporaryCountBuffer = 0;  // temporary count before resize can be used by imgui

    uint32_t m_CurrentFrame = 0U;

    // vulkan creation
    GaiApi::VulkanCoreWeak m_VulkanCore;  // vulkan core
    GaiApi::VulkanQueue m_Queue;          // queue
    vk::Device m_Device;                  // device copy

    // ComputeBuffer
    std::vector<std::vector<Texture2DPtr>> m_ComputeBuffers;
    vk::Format m_Format = vk::Format::eR32G32B32A32Sfloat;

    // Submition
    std::vector<vk::Semaphore> m_RenderCompleteSemaphores;
    std::vector<vk::Fence> m_WaitFences;
    std::vector<vk::CommandBuffer> m_CommandBuffers;

    // dynamic state
    // vk::Rect2D m_RenderArea = {};
    // vk::Viewport m_Viewport = {};
    ez::uvec3 m_OutputSize;  // output size for compute stage
    float m_OutputRatio = 1.0f;

public:  // contructor
    ComputeBuffer(GaiApi::VulkanCoreWeak vVulkanCore);
    virtual ~ComputeBuffer();

    // init/unit
    bool Init(const ez::uvec2& vSize, const uint32_t& vCountColorBuffers, const bool& vPingPongBufferMode, const vk::Format& vFormat);
    void Unit();

    // resize
    void NeedResize(ez::ivec2* vNewSize, const uint32_t* vCountColorBuffers = nullptr);  // to call at any moment

    // not to call at any moment, to call only aftter submit or before any command buffer recording
    // return true, if was resized
    bool ResizeIfNeeded();

    // Merger for merged rendering one FBO in the merger
    bool Begin(vk::CommandBuffer* vCmdBufferPtr);
    void End(vk::CommandBuffer* vCmdBufferPtr);

    // get sampler / image / buffer
    vk::DescriptorImageInfo* GetBackDescriptorImageInfo(const uint32_t& vBindingPoint);
    vk::DescriptorImageInfo* GetFrontDescriptorImageInfo(const uint32_t& vBindingPoint);

    uint32_t GetBuffersCount() const;
    bool IsPingPongBufferMode() const;

    // OutputSizeInterface
    float GetOutputRatio() const override;
    ez::fvec2 GetOutputSize() const override;

    bool UpdateMipMapping(const uint32_t& vBindingPoint);

    void Swap();

protected:
    // Framebuffer
    bool CreateComputeBuffers(const ez::uvec2& vSize, const uint32_t& vCountColorBuffers, const vk::Format& vFormat);
    void DestroyComputeBuffers();
};