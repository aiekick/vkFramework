﻿/*
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

#include <set>
#include <cmath>
#include <array>
#include <stack>
#include <memory>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include <ImGuiPack.h>
#include <Gaia/gaia.h>

///////////////////////////////////
//// MACROS ///////////////////////
///////////////////////////////////

#define vkProfBeginFrame(label) GaiApi::vkProfiler::Instance()->BeginFrame(label)
#define vkProfEndFrame GaiApi::vkProfiler::Instance()->EndFrame()
#define vkProfCollectFrame GaiApi::vkProfiler::Instance()->Collect()

#define vkProfBeginZone(commandBuffer, section, fmt, ...) \
    GaiApi::vkProfiler::Instance()->beginChildZone(commandBuffer, nullptr, section, fmt, ##__VA_ARGS__)
#define vkProfBeginZonePtr(commandBuffer, ptr, section, fmt, ...) \
    GaiApi::vkProfiler::Instance()->beginChildZone(commandBuffer, ptr, section, fmt, ##__VA_ARGS__);
#define vkProfEndZone(commandBuffer) GaiApi::vkProfiler::Instance()->endChildZone(commandBuffer)

#define vkProfScopedStages(stages, commandBuffer, section, fmt, ...)                                                         \
    auto __vkProf__ScopedChildZone = GaiApi::vkScopedChildZone(stages, commandBuffer, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZone
#define vkProfScopedStagesPtr(stages, commandBuffer, ptr, section, fmt, ...)                                             \
    auto __vkProf__ScopedChildZone = GaiApi::vkScopedChildZone(stages, commandBuffer, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZone

#define vkProfScoped(commandBuffer, section, fmt, ...)                                                               \
    auto __vkProf__ScopedChildZone = GaiApi::vkScopedChildZone(commandBuffer, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZone
#define vkProfScopedPtr(commandBuffer, ptr, section, fmt, ...)                                                   \
    auto __vkProf__ScopedChildZone = GaiApi::vkScopedChildZone(commandBuffer, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZone

#define vkProfScopedStagesNoCmd(stages, section, fmt, ...)                                                              \
    auto __vkProf__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(stages, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZoneNoCmd
#define vkProfScopedStagesPtrNoCmd(stages, ptr, section, fmt, ...)                                                  \
    auto __vkProf__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(stages, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZoneNoCmd

#define vkProfScopedNoCmd(section, fmt, ...)                                                                    \
    auto __vkProf__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZoneNoCmd
#define vkProfScopedPtrNoCmd(ptr, section, fmt, ...)                                                        \
    auto __vkProf__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(ptr, section, fmt, ##__VA_ARGS__); \
    (void)__vkProf__ScopedChildZoneNoCmd

#define vkProfBeginZoneNoCmd(section, fmt, ...) GaiApi::vkProfiler::Instance()->beginChildZoneNoCmd(nullptr, section, fmt, ##__VA_ARGS__)
#define vkProfBeginZonePtrNoCmd(ptr, section, fmt, ...) GaiApi::vkProfiler::Instance()->beginChildZoneNoCmd(ptr, section, fmt, ##__VA_ARGS__);
#define vkProfEndZoneNoCmd GaiApi::vkProfiler::Instance()->endChildZoneNoCmd()

///////////////////////////////////
///////////////////////////////////
///////////////////////////////////

#ifndef vkProf_RECURSIVE_LEVELS_COUNT
#define vkProf_RECURSIVE_LEVELS_COUNT 20U
#endif  // RECURSIVE_LEVELS_COUNT

#ifndef vkProf_MEAN_AVERAGE_LEVELS_COUNT
#define vkProf_MEAN_AVERAGE_LEVELS_COUNT 60U
#endif  // MEAN_AVERAGE_LEVELS_COUNT

namespace GaiApi {

typedef uint64_t vkTimeStamp;

class vkProfQueryZone;
typedef std::shared_ptr<vkProfQueryZone> vkProfQueryZonePtr;
typedef std::weak_ptr<vkProfQueryZone> vkProfQueryZoneWeak;

class vkProfiler;
typedef std::shared_ptr<vkProfiler> vkProfilerPtr;
typedef std::weak_ptr<vkProfiler> vkProfilerWeak;

enum vkProfGraphTypeEnum { IN_APP_GPU_HORIZONTAL = 0, IN_APP_GPU_CIRCULAR, IN_APP_GPU_Count };

template <typename T>
class vkProfAverageValue {
private:
    static constexpr uint32_t sCountAverageValues = vkProf_MEAN_AVERAGE_LEVELS_COUNT;
    T m_PerFrame[sCountAverageValues] = {};
    int m_PerFrameIdx = (T)0;
    T m_PerFrameAccum = (T)0;
    T m_AverageValue = (T)0;

public:
    vkProfAverageValue();
    void AddValue(T vValue);
    T GetAverage();
};

template <typename T>
vkProfAverageValue<T>::vkProfAverageValue() {
    memset(m_PerFrame, 0, sizeof(T) * sCountAverageValues);
    m_PerFrameIdx = (T)0;
    m_PerFrameAccum = (T)0;
    m_AverageValue = (T)0;
}

template <typename T>
void vkProfAverageValue<T>::AddValue(T vValue) {
    if (vValue < m_PerFrame[m_PerFrameIdx]) {
        memset(m_PerFrame, 0, sizeof(T) * sCountAverageValues);
        m_PerFrameIdx = (T)0;
        m_PerFrameAccum = (T)0;
        m_AverageValue = (T)0;
    }
    m_PerFrameAccum += vValue - m_PerFrame[m_PerFrameIdx];
    m_PerFrame[m_PerFrameIdx] = vValue;
    m_PerFrameIdx = (m_PerFrameIdx + (T)1) % sCountAverageValues;
    if (m_PerFrameAccum > (T)0) {
        m_AverageValue = m_PerFrameAccum / (T)sCountAverageValues;
    }
}

template <typename T>
T vkProfAverageValue<T>::GetAverage() {
    return m_AverageValue;
}

class GAIA_API vkProfQueryZone {
public:
    struct circularSettings {
        float count_point = 20.0f;  // 60 for 2pi
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float base_radius = 50.0f;
        float space = 5.0f;
        float thick = 10.0f;
    };

public:
    static uint32_t sMaxDepthToOpen;
    static bool sShowLeafMode;
    static float sContrastRatio;
    static bool sActivateLogger;
    static uint32_t sCurrentDepth;  // current depth catched by Profiler
    static uint32_t sMaxDepth;      // max depth catched ever
    static std::vector<vkProfQueryZoneWeak> sTabbedQueryZones;
    static vkProfQueryZonePtr create(
        void* vThreadPtr, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    static circularSettings sCircularSettings;

public:
    uint32_t depth = 0U;  // the depth of the QueryZone
    // inc the query each calls (for identify where a id is called
    // many time per frame but not reseted before and causse layer issue)
    uint32_t calledCountPerFrame = 0U;
    std::vector<vkProfQueryZonePtr> zonesOrdered;
    std::unordered_map<const void*, std::unordered_map<std::string, vkProfQueryZonePtr>> zonesDico;  // main container
    std::string name;
    std::string imGuiLabel;
    std::string imGuiTitle;
    vkProfQueryZonePtr parentPtr = nullptr;
    vkProfQueryZonePtr rootPtr = nullptr;
    uint32_t current_count = 0U;
    uint32_t last_count = 0U;
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;

private:
    uint32_t ids[2] = {0U, 0U};
    vkProfQueryZoneWeak m_This;
    bool m_IsRoot = false;
    //const void* m_Ptr = nullptr;
    double m_ElapsedTime = 0.0;
    double m_StartTime = 0.0;
    double m_EndTime = 0.0;
    uint32_t m_StartFrameId = 0;
    uint32_t m_EndFrameId = 0;
    uint64_t m_StartTimeStamp = 0;
    uint64_t m_EndTimeStamp = 0;
    bool m_Expanded = false;
    bool m_Highlighted = false;
    vkProfAverageValue<uint64_t> m_AverageStartValue;
    vkProfAverageValue<uint64_t> m_AverageEndValue;
    //void* m_ThreadPtr = nullptr;
    std::string m_BarLabel;
    std::string m_SectionName;
    ImVec4 cv4;
    ImVec4 hsv;
   // vkProfGraphTypeEnum m_GraphType = vkProfGraphTypeEnum::IN_APP_GPU_HORIZONTAL;

    // fil d'ariane
    std::array<vkProfQueryZoneWeak, vkProf_RECURSIVE_LEVELS_COUNT> m_BreadCrumbTrail;  // the parent cound is done by current depth

    // circular
    const float _1PI_ = 3.141592653589793238462643383279f;
    //const float _2PI_ = 6.283185307179586476925286766559f;
    const ImU32 m_BlackU32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
    ImVec2 m_P0, m_P1, m_LP0, m_LP1;

public:
    vkProfQueryZone() = default;
    vkProfQueryZone(void* vThreadPtr, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    ~vkProfQueryZone();
    void Clear();
    const uint32_t& GetIdForWrite(const size_t& vIdx);
    const uint32_t& GetId(const size_t& vIdx) const;
    void SetId(const size_t& vIdx, const uint32_t& vID);
    bool wasSeen() const;
    void NewFrame();
    void SetStartTimeStamp(const uint64_t& vValue);
    void SetEndTimeStamp(const uint64_t& vValue);
    void ComputeElapsedTime();
    void DrawDetails();
    bool DrawFlamGraph(vkProfGraphTypeEnum vGraphType,  //
        vkProfQueryZoneWeak& vOutSelectedQuery,         //
        vkProfQueryZoneWeak vParent = {},               //
        uint32_t vDepth = 0);
    void UpdateBreadCrumbTrail();
    void DrawBreadCrumbTrail(vkProfQueryZoneWeak& vOutSelectedQuery);

private:
    void m_DrawList_DrawBar(const char* vLabel, const ImRect& vRect, const ImVec4& vColor, const bool& vHovered);
    bool m_ComputeRatios(vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio);
    bool m_DrawHorizontalFlameGraph(vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak& vOutSelectedQuery, vkProfQueryZoneWeak vParent, uint32_t vDepth);
    bool m_DrawCircularFlameGraph(vkProfQueryZonePtr vRoot, vkProfQueryZoneWeak& vOutSelectedQuery, vkProfQueryZoneWeak vParent, uint32_t vDepth);
};

class GAIA_API vkProfiler {
private:
    static constexpr uint32_t sMaxQueryCount = 1024U;
    static constexpr uint32_t sMaxDepth = 64U;

public:
    typedef std::function<bool(const char*, bool*, ImGuiWindowFlags)> ImGuiBeginFunctor;
    typedef std::function<void()> ImGuiEndFunctor;
    class CommandBufferInfos {
    private:
        vk::Device device;
        VulkanCoreWeak core;
        vk::QueryPool queryPool;
        vkProfiler* parentProfilerPtr = nullptr;

    public:
        std::array<vk::CommandBuffer, 2U> cmds;
        std::array<vk::Fence, 2U> fences;
        CommandBufferInfos() = default;
        ~CommandBufferInfos();
        void Init(VulkanCoreWeak vCore, vk::Device vDevice, vk::CommandPool vCmdPool, vk::QueryPool vQueryPool, vkProfiler* vParentProfilerPtr);
        void begin(const size_t& idx);
        void end(const size_t& idx);
        void writeTimeStamp(const size_t& idx, vkProfQueryZoneWeak vQueryZone, vk::PipelineStageFlagBits vStages);
    };

public:
    static vkProfilerPtr create(VulkanCoreWeak vVulkanCore);
    static vkProfilerPtr Instance(VulkanCoreWeak vVulkanCore = {}) {
        static auto _instance_ptr = vkProfiler::create(vVulkanCore);
        return _instance_ptr;
    };

private:
    vkProfGraphTypeEnum m_GraphType = vkProfGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
    int32_t m_QueryZoneToClose = -1;
    ImGuiBeginFunctor m_ImGuiBeginFunctor =                                     //
        [](const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) -> bool {  //
        return ImGui::Begin(vLabel, pOpen, vFlags);
    };
    ImGuiEndFunctor m_ImGuiEndFunctor = []() { ImGui::End(); };
    bool m_ShowDetails = false;
    bool m_IsLoaded = false;
    void* m_ThreadPtr = nullptr;
    vkProfilerWeak m_This;
    VulkanCoreWeak m_VulkanCore;
    vkProfQueryZonePtr m_RootZone = nullptr;
    vkProfQueryZoneWeak m_SelectedQuery;                                  // query to show the flamegraph in this context
    std::array<vkProfQueryZonePtr, sMaxQueryCount> m_QueryIDToZone = {};  // Get the zone for a query id because a query have to id's : start and end
    std::array<vkProfQueryZonePtr, sMaxDepth> m_DepthToLastZone = {};     // last zone registered at this depth
    std::array<vkTimeStamp, sMaxQueryCount> m_TimeStampMeasures = {};
    vk::QueryPool m_QueryPool = {};
    uint32_t m_QueryHead = 0U;
    uint32_t m_QueryCount = 0U;     // reseted each frames
    uint32_t m_MaxQueryCount = 0U;  // tuned at creation
    bool m_IsActive = false;
    bool m_IsPaused = false;

    std::unordered_map<std::string, CommandBufferInfos> m_CommandBuffers;

    std::stack<vkProfQueryZoneWeak> m_QueryStack;

    char m_TempBuffer[1024] = {};

public:
    vkProfiler() = default;
    ~vkProfiler() = default;

    bool Init(VulkanCoreWeak vVulkanCore);
    void Unit();
    void Clear();

    void DrawDetails(ImGuiWindowFlags vFlags = 0);
    void DrawDetailsNoWin();

    void DrawFlamGraph(const vkProfGraphTypeEnum& vGraphType);
    void DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags = 0);
    void DrawFlamGraphNoWin();
    void DrawFlamGraphChilds(ImGuiWindowFlags vFlags = 0);

    void SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor);
    void SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor);

    void Collect();

    bool& isActiveRef();
    const bool& isActive();
    bool& isPausedRef();
    const bool& isPaused();

    bool canRecordTimeStamp(const bool& isRoot = false);

    vkProfQueryZonePtr GetQueryZoneForName(const void* vPtr, const std::string& vName, const std::string& vSection = "", const bool& vIsRoot = false);

    void BeginFrame(const char* vLabel);
    void EndFrame();

    bool beginChildZone(const VkCommandBuffer& vCmd, const void* vPtr, const std::string& vSection, const char* fmt, ...);
    bool endChildZone(const VkCommandBuffer& vCmd);

    void writeTimeStamp(const vk::CommandBuffer& vCmd, const size_t& idx, vkProfQueryZoneWeak vQueryZone, vk::PipelineStageFlagBits vStages);

    CommandBufferInfos* beginChildZoneNoCmd(const void* vPtr, const std::string& vSection, const char* fmt, ...);
    void endChildZoneNoCmd(CommandBufferInfos* vCommandBufferInfosPtr);

    CommandBufferInfos* GetCommandBufferInfosPtr(const void* vPtr, const std::string& vSection, const char* fmt, ...);
    CommandBufferInfos* GetCommandBufferInfosPtr(const void* vPtr, const std::string& vSection, const char* fmt, va_list vArgs);

private:
    void m_ClearMeasures();
    void m_AddMeasure();
    bool m_BeginZone(const VkCommandBuffer& vCmd, const bool& vIsRoot, const void* vPtr, const std::string& vSection, const char* label);
    bool m_BeginZone(const VkCommandBuffer& vCmd, const bool& vIsRoot, const void* vPtr, const std::string& vSection, const char* fmt, va_list vArgs);
    bool m_EndZone(const VkCommandBuffer& vCmd, const bool& vIsRoot);

    void m_SetQueryZoneForDepth(vkProfQueryZonePtr vQueryZone, uint32_t vDepth);
    vkProfQueryZonePtr m_GetQueryZoneFromDepth(uint32_t vDepth);
    void m_DrawMenuBar();
    int32_t m_GetNextQueryId();
};

class GAIA_API vkScopedChildZone {
public:
    vkProfQueryZonePtr queryZonePtr = nullptr;
    VkCommandBuffer commandBuffer = {};
    vk::PipelineStageFlagBits stages = vk::PipelineStageFlagBits::eBottomOfPipe;

public:
    vkScopedChildZone(                             //
        const vk::PipelineStageFlagBits& vStages,  //
        const VkCommandBuffer& vCmd,               //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    vkScopedChildZone(                             //
        const VkCommandBuffer& vCmd,               //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    ~vkScopedChildZone();
};

class GAIA_API vkScopedChildZoneNoCmd {
public:
    vkProfQueryZonePtr queryZonePtr = nullptr;
    vkProfiler::CommandBufferInfos* infosPtr = nullptr;
    vk::PipelineStageFlagBits stages = vk::PipelineStageFlagBits::eBottomOfPipe;

public:
    vkScopedChildZoneNoCmd(                        //
        const vk::PipelineStageFlagBits& vStages,  //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    vkScopedChildZoneNoCmd(                        //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    ~vkScopedChildZoneNoCmd();
};

}  // namespace GaiApi
