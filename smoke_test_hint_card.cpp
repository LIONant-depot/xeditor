// The hover card (xeditor::hint::growing_card) driven frame by frame with ImGui itself, with no window and no GPU (ImGui only needs a display size and a mouse): every frame in which the
// card is visible must be completely inside the window, on the first hover and on the second one (when its size is known) too, and a known card must open at its size, not smaller.
// Standalone: built and run by source/Editors/LevelEditor/smoke/test_hint_card.py. Exit 0 when every check holds.
#include "imgui.h"
#include "imgui_internal.h"
#include "dependencies/xeditor/include/xeditor/hint.h"

#include <cstdio>

namespace
{
    constexpr float kW = 1280.0f, kH = 720.0f;
    int g_Failed = 0;

    xeditor::hint::growing_card g_Card;
    ImVec2                      g_ItemPos(0, 0);
    ImVec2                      g_ContentSize(400, 260);        // what the card's content measures

    struct visible { bool m_bShown = false; ImVec2 m_Pos, m_Size; };

    // One frame: a host window the size of the display with one item at g_ItemPos; the card is shown while the mouse is on that item.
    visible Frame(ImVec2 Mouse)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(Mouse.x, Mouse.y);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(kW, kH));
        ImGui::Begin("host", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::SetCursorScreenPos(g_ItemPos);
        ImGui::Dummy(ImVec2(64, 64));
        if (ImGui::IsItemHovered())
        {
            if (g_Card.Begin(ImGui::GetItemID(), 480.0f))
            {
                ImGui::Dummy(g_ContentSize);
                g_Card.End();
            }
        }
        ImGui::End();
        ImGui::Render();

        visible Out;
        if (const ImGuiWindow* pWindow = ImGui::FindWindowByName("##Tooltip_00"))
            if (pWindow->WasActive && pWindow->Active && !pWindow->Hidden && pWindow->HiddenFramesCannotSkipItems == 0 && pWindow->HiddenFramesCanSkipItems == 0)
                Out = { true, pWindow->Pos, pWindow->Size };
        return Out;
    }

    bool Inside(const visible& V) { return V.m_Pos.x >= 0 && V.m_Pos.y >= 0 && V.m_Pos.x + V.m_Size.x <= kW && V.m_Pos.y + V.m_Size.y <= kH; }

    void Fail(const char* pWhat, int FrameNo, const visible& V)
    {
        ++g_Failed;
        std::printf("FAIL: %s (frame %d: at %.1f,%.1f size %.1fx%.1f)\n", pWhat, FrameNo, V.m_Pos.x, V.m_Pos.y, V.m_Size.x, V.m_Size.y);
    }

    // Hovers the item at ItemAt (mouse on it) for Frames frames; every visible frame must be inside the window. Returns the last visible rectangle and, in FirstSize, the size of the first one.
    visible Hover(ImVec2 ItemAt, int Frames, const char* pName, visible* pFirst = nullptr)
    {
        g_ItemPos = ItemAt;
        const ImVec2 Mouse(ItemAt.x + 32, ItemAt.y + 32);
        visible Last, First;
        bool bFirst = true;
        for (int i = 0; i < Frames; ++i)
        {
            const visible V = Frame(Mouse);
            if (!V.m_bShown) continue;
            if (bFirst) { First = V; bFirst = false; }
            if (!Inside(V)) { Fail(pName, i, V); }
            Last = V;
        }
        if (pFirst) *pFirst = First;
        if (!Last.m_bShown) Fail("the card never became visible", Frames, Last);
        return Last;
    }

    void Away(int Frames)               // the mouse leaves the item: no card
    {
        for (int i = 0; i < Frames; ++i) Frame(ImVec2(5, 5));
    }
}

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(kW, kH);
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = nullptr;
    io.Fonts->AddFontDefault();
    unsigned char* pPixels = nullptr; int W = 0, H = 0;
    io.Fonts->GetTexDataAsRGBA32(&pPixels, &W, &H);

    // The places an item can be: the middle, each edge, each corner (the card must flip or be pushed in)
    const ImVec2 Places[] = { { 600, 300 }, { 1200, 300 }, { 10, 300 }, { 600, 10 }, { 600, 650 }, { 1210, 650 }, { 10, 10 }, { 1210, 10 }, { 10, 650 } };
    for (const ImVec2 Place : Places)
    {
        g_Card = {};                                                    // a card nobody has shown yet
        // first hover: the size is not known: it grows, and every step is inside
        visible FirstShown;
        const visible Done = Hover(Place, 90, "first hover left the window", &FirstShown);
        const ImVec2 FirstSize = FirstShown.m_Size;
        if (Done.m_Size.x < g_ContentSize.x - 1.0f || Done.m_Size.y < g_ContentSize.y - 1.0f) Fail("the grown card is smaller than its content", 90, Done);
        if (FirstSize.x > 200.0f) { visible V; V.m_Pos = ImVec2(0, 0); V.m_Size = FirstSize; Fail("a card that is not known yet did not start small", 0, V); }

        // the mouse goes away and comes back: now the size is known, so the FIRST visible frame is already the full size, and inside
        Away(20);
        visible SecondFirst;
        const visible SecondLast = Hover(Place, 30, "second hover (size known) left the window", &SecondFirst);
        if (SecondFirst.m_Size.x < Done.m_Size.x - 1.0f || SecondFirst.m_Size.y < Done.m_Size.y - 1.0f) Fail("a known card did not open at its size", 0, SecondFirst);
        // and in the place it stays in: the first visible frame is the same rectangle as the last (nothing moves, nothing resizes after it opened)
        const auto Near = [](float A, float B) { return A > B - 0.6f && A < B + 0.6f; };
        if (!Near(SecondFirst.m_Pos.x, SecondLast.m_Pos.x) || !Near(SecondFirst.m_Pos.y, SecondLast.m_Pos.y) || !Near(SecondFirst.m_Size.x, SecondLast.m_Size.x) || !Near(SecondFirst.m_Size.y, SecondLast.m_Size.y))
        {
            Fail("a known card opened in another place or size than it keeps (first frame)", 0, SecondFirst);
            Fail("... where it stays (last frame)", 0, SecondLast);
        }
        // away from every edge the card is beside the cursor: the offset (16) from it on the side that has room
        if (Place.x == 600 && Place.y == 300)
        {
            const ImVec2 Mouse(Place.x + 32, Place.y + 32);
            const bool bBesideX = Near(SecondFirst.m_Pos.x, Mouse.x + 16.0f) || Near(SecondFirst.m_Pos.x + SecondFirst.m_Size.x, Mouse.x - 16.0f);
            const bool bBesideY = Near(SecondFirst.m_Pos.y, Mouse.y + 16.0f) || Near(SecondFirst.m_Pos.y + SecondFirst.m_Size.y, Mouse.y - 16.0f);
            if (!bBesideX || !bBesideY) Fail("a card in the middle of the window is not 16 pixels from the cursor", 0, SecondFirst);
        }
    }

    ImGui::DestroyContext();
    if (g_Failed == 0) std::printf("hint card: all checks passed\n");
    return g_Failed == 0 ? 0 : 1;
}
