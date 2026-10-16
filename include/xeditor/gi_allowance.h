#ifndef XEDITOR_GI_ALLOWANCE_H
#define XEDITOR_GI_ALLOWANCE_H
#pragma once

#include <algorithm>

namespace xeditor
{
    // The GI's main-thread time for the whole application, per frame (dependencies/xLIONRender/documentation/gi_dynamic_master_plan.md,
    // section 2 and Phase 1): every renderer with GI (the Level's viewport, the static geometry preview) asks for its grant before it renders
    // and reports what its trace used after. The foreground (the Level's viewport) comes first whatever the render order: what it used in the
    // last frame is kept for it until it has rendered in this one. The host resets it at the top of each frame (host::m_GIAllowance)
    struct gi_allowance
    {
        float m_FrameMs = 1.5f;                 // the contract's 2 ms, less a margin for the one-unit overshoot of each renderer
        float m_UsedMs  = 0;                    // this frame so far
        float m_ForegroundLastMs = 0;           // what the foreground used in the last frame: kept for it
        bool  m_bForegroundDone  = false;       // the foreground rendered in this frame

        // A foreground that did not render in the last frame (the Level closed, its tab hidden) keeps nothing
        void NewFrame() noexcept { if (!m_bForegroundDone) m_ForegroundLastMs = 0; m_UsedMs = 0; m_bForegroundDone = false; }

        // Never 0: an allowance of 0 is "no limit" for the renderer; this small one starts no unit
        float Grant(bool bForeground) const noexcept
        {
            float Left = m_FrameMs - m_UsedMs;
            if (!bForeground && !m_bForegroundDone) Left -= m_ForegroundLastMs;
            return std::max(Left, 0.001f);
        }

        void Used(float Ms, bool bForeground) noexcept
        {
            m_UsedMs += std::max(Ms, 0.0f);
            if (bForeground) { m_ForegroundLastMs = std::min(std::max(Ms, 0.0f), m_FrameMs); m_bForegroundDone = true; }
        }
    };
}

#endif
