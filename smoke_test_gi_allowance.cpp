// The host's shared GI allowance per frame (include/xeditor/gi_allowance.h): the foreground first whatever the render order, the rest for
// the others, never a grant of 0 (0 is "no limit" for the renderer). Standalone: built and run by
// source/Editors/LevelEditor/smoke/test_gi_allowance.py. Exit 0 when every check holds.
#include "xeditor/gi_allowance.h"

#include <cstdio>

namespace
{
    int g_Failed = 0;

    bool Near(float A, float B) { return A > B - 1e-5f && A < B + 1e-5f; }

    void Check(bool bOk, const char* pWhat, float Got)
    {
        if (bOk) return;
        ++g_Failed;
        std::printf("FAIL: %s (got %.3f)\n", pWhat, Got);
    }
}

int main()
{
    xeditor::gi_allowance A;
    A.m_FrameMs = 1.5f;

    // Frame 1: the foreground renders first and uses 1 ms; a preview gets the rest
    A.NewFrame();
    Check(A.Grant(true) == 1.5f, "the foreground alone gets the whole frame", A.Grant(true));
    A.Used(1.0f, true);
    Check(A.Grant(false) == 0.5f, "a preview after the foreground gets what is left", A.Grant(false));
    A.Used(0.5f, false);
    Check(A.Grant(false) > 0 && A.Grant(false) < 0.01f, "nothing left: a small grant, never 0", A.Grant(false));

    // Frame 2: the preview renders before the foreground: what the foreground used last frame is kept for it
    A.NewFrame();
    Check(A.Grant(false) == 0.5f, "a preview before the foreground leaves the foreground's last use", A.Grant(false));
    A.Used(0.5f, false);
    Check(A.Grant(true) == 1.0f, "the foreground still gets its share", A.Grant(true));
    A.Used(0.2f, true);                                                     // settled: it uses less
    Check(Near(A.Grant(false), 0.8f), "once the foreground rendered, a preview gets the rest", A.Grant(false));

    // Frame 3: the foreground's last use (0.2) is kept, but it does not render (closed): in frame 4 nothing is kept for it
    A.NewFrame();
    Check(Near(A.Grant(false), 1.3f), "the foreground's last use (0.2) is kept", A.Grant(false));
    A.NewFrame();
    Check(A.Grant(false) == 1.5f, "a foreground that did not render keeps nothing", A.Grant(false));

    // An overshoot (one unit past the grant) never gives a negative or zero grant
    A.NewFrame();
    A.Used(2.5f, true);
    Check(A.Grant(false) > 0, "past the frame: still a positive grant", A.Grant(false));

    if (g_Failed) return 1;
    std::printf("all checks passed\n");
    return 0;
}
