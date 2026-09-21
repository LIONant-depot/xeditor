#ifndef XEDITOR_IDLE_WORK_H
#define XEDITOR_IDLE_WORK_H
#pragma once

// Idle work: maintenance that only matters when nothing else is happening (consistency scans, cache refreshes).
// Neither the user nor a CLI/AI driver has done anything for a while, so a long disk walk cannot compete with real
// work. This is only the dispatch point: a task subscribes to m_OnRun and, when it starts background work, registers
// it with BeginIdleTask so the Idle Work panel can show it and IdleWorkCancelRequested() can stop it.
//
// Tasks run on xscheduler (priority LOW) rather than a raw thread. The registry and the cancel flag are process-wide
// because a background task can outlive the editor that started it.
#include "dependencies/xdelegate/source/xdelegate.h"
#include "imgui.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace xeditor
{
    enum class idle_task_status : std::uint8_t { Running, Done, Cancelled };

    // What a task reports about itself. The strings are pre-formatted by the task, so the panel needs no task knowledge.
    struct idle_task_record
    {
        std::uint64_t                         m_TaskId    = 0;
        std::string                           m_Name;
        std::string                           m_Description;    // empty = nothing to show
        idle_task_status                      m_Status    = idle_task_status::Running;
        std::chrono::steady_clock::time_point m_StartTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point m_EndTime   = {}; // meaningful once m_Status != Running
        std::string                           m_Info;           // meaningful once m_Status == Done
    };

    inline constexpr std::size_t idle_task_history_cap_v = 100;

    inline std::mutex&                    IdleTaskRegistryMutex() noexcept { static std::mutex M; return M; }
    inline std::vector<idle_task_record>& IdleTaskRegistry()      noexcept { static std::vector<idle_task_record> R; return R; }

    // Set the instant the USER resumes working, so a running task can stop within about one unit of its own work. CLI/pipe
    // activity only resets the idle clock: a command that starts idle work on purpose must not cancel it, and a task that
    // logs must not cancel itself by looking like activity.
    inline std::atomic<bool>& IdleWorkCancelRequested() noexcept { static std::atomic<bool> Flag{ false }; return Flag; }
    inline void RequestIdleWorkCancel() noexcept { IdleWorkCancelRequested().store(true, std::memory_order_relaxed); }

    inline std::uint64_t BeginIdleTask(std::string Name, std::string Description) noexcept
    {
        static std::atomic<std::uint64_t> s_NextId{ 1 };
        const auto TaskId = s_NextId.fetch_add(1, std::memory_order_relaxed);

        std::lock_guard<std::mutex> Lock(IdleTaskRegistryMutex());
        auto& Registry = IdleTaskRegistry();
        if (Registry.size() >= idle_task_history_cap_v)
            Registry.erase(Registry.begin(), Registry.begin() + (Registry.size() - idle_task_history_cap_v + 1));
        Registry.push_back({ TaskId, std::move(Name), std::move(Description) });
        return TaskId;
    }

    inline void EndIdleTask(std::uint64_t TaskId, idle_task_status FinalStatus, std::string Info) noexcept
    {
        std::lock_guard<std::mutex> Lock(IdleTaskRegistryMutex());
        for (auto& Rec : IdleTaskRegistry())
        {
            if (Rec.m_TaskId != TaskId) continue;
            Rec.m_Status  = FinalStatus;
            Rec.m_EndTime = std::chrono::steady_clock::now();
            Rec.m_Info    = std::move(Info);
            break;
        }
    }

    // Approximate (holding a navigation key with no character output is not caught) and app-local: what matters is
    // whether THIS editor is being used, not the whole machine.
    inline bool DetectUserInputActivity() noexcept
    {
        const auto& IO = ImGui::GetIO();
        return IO.MouseDelta.x != 0.0f || IO.MouseDelta.y != 0.0f
            || IO.MouseWheel  != 0.0f || IO.MouseWheelH != 0.0f
            || ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)
            || IO.InputQueueCharacters.Size > 0;
    }

    struct idle_work
    {
        static constexpr double threshold_seconds_v = 30.0;

        // Fired once each time the editor goes quiet (bManual = false), and by RunNow (bManual = true), which is an
        // explicit request: a task should then do its work even if it already did it this session.
        xdelegate::thread_unsafe<bool> m_OnRun;

        std::chrono::steady_clock::time_point m_LastActivity = std::chrono::steady_clock::now();

        double SecondsIdle() const noexcept { return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_LastActivity).count(); }
        bool   isIdle()      const noexcept { return SecondsIdle() >= threshold_seconds_v; }

        // Real activity: a mouse/keyboard event, or a CLI/pipe command. Re-arms the trigger for the next quiet period.
        void NotifyActivity() noexcept
        {
            m_LastActivity = std::chrono::steady_clock::now();
            m_bTriggered   = false;
        }

        // Once per frame. Does nothing until the idle threshold is first crossed, then stays quiet until the next activity.
        void Pump() noexcept
        {
            if (m_bTriggered || !isIdle()) return;
            m_bTriggered = true;
            IdleWorkCancelRequested().store(false, std::memory_order_relaxed);
            m_OnRun.NotifyAll(false);
        }

        void RunNow() noexcept
        {
            IdleWorkCancelRequested().store(false, std::memory_order_relaxed);
            m_OnRun.NotifyAll(true);
        }

    private:
        bool m_bTriggered = false;
    };

    // The Idle Work panel's contents (the caller owns the window). Run Now is enabled by bCanRunNow: only the caller
    // knows whether any task has something to do.
    inline void RenderIdleWorkPanel(idle_work& Idle, bool bCanRunNow) noexcept
    {
        if (Idle.isIdle())
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.0f), "Idle for %.0fs - background maintenance may run", Idle.SecondsIdle());
        else
            ImGui::TextDisabled("Active (%.0fs since last activity, idles at %.0fs)", Idle.SecondsIdle(), idle_work::threshold_seconds_v);

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 90.0f);
        ImGui::BeginDisabled(!bCanRunNow);
        if (ImGui::Button("Run Now")) Idle.RunNow();
        ImGui::EndDisabled();

        ImGui::Separator();

        if (ImGui::BeginTable("##IdleTasks", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Task");
            ImGui::TableSetupColumn("Description");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Duration");
            ImGui::TableSetupColumn("Info");
            ImGui::TableHeadersRow();

            std::lock_guard<std::mutex> Lock(IdleTaskRegistryMutex());
            auto& Registry = IdleTaskRegistry();
            for (auto It = Registry.rbegin(); It != Registry.rend(); ++It) // newest first
            {
                auto& Rec = *It;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(Rec.m_Name.c_str());
                ImGui::TableSetColumnIndex(1);
                if (!Rec.m_Description.empty()) ImGui::TextUnformatted(Rec.m_Description.c_str());
                else ImGui::TextDisabled("-");
                ImGui::TableSetColumnIndex(2);
                switch (Rec.m_Status)
                {
                case idle_task_status::Running:   ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Running");   break;
                case idle_task_status::Done:      ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.0f), "Done");     break;
                case idle_task_status::Cancelled: ImGui::TextColored(ImVec4(0.85f, 0.4f, 0.4f, 1.0f), "Cancelled"); break;
                }
                ImGui::TableSetColumnIndex(3);
                const auto EndPoint = (Rec.m_Status == idle_task_status::Running) ? std::chrono::steady_clock::now() : Rec.m_EndTime;
                ImGui::Text("%.1fs", std::chrono::duration<double>(EndPoint - Rec.m_StartTime).count());
                ImGui::TableSetColumnIndex(4);
                if (Rec.m_Status == idle_task_status::Done && !Rec.m_Info.empty()) ImGui::TextUnformatted(Rec.m_Info.c_str());
                else ImGui::TextDisabled("-");
            }
            ImGui::EndTable();
        }
    }
}

#endif // XEDITOR_IDLE_WORK_H
