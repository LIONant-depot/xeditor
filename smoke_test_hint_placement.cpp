// The placement rule of a hint that must stay inside the window of the app (include/xeditor/hint_placement.h). Standalone: built and run by
// source/Editors/LevelEditor/smoke/test_hint_placement.py. Exit 0 when every check holds.
#include "xeditor/hint_placement.h"

#include <cstdio>
#include <initializer_list>

namespace
{
    int g_Failed = 0;

    void Check(bool bOk, const char* pWhat, float X, float Y)
    {
        if (bOk) return;
        ++g_Failed;
        std::printf("FAIL: %s (got %.1f, %.1f)\n", pWhat, X, Y);
    }
}

int main()
{
    using namespace xeditor::hint;
    const point Min{ 0, 0 }, Max{ 1000, 800 };
    constexpr float Offset = 16, Margin = 4;

    // room after the cursor: it opens after it, Offset away
    {
        const auto P = PlaceInside({ 100, 100 }, { 300, 200 }, Min, Max, Offset);
        Check(P.x == 116 && P.y == 116, "room right and below: opens at the cursor plus the offset", P.x, P.y);
    }
    // no room to the right: it opens before the cursor (its right edge Offset before it)
    {
        const auto P = PlaceInside({ 900, 100 }, { 300, 200 }, Min, Max, Offset);
        Check(P.x == 900 - 16 - 300 && P.y == 116, "no room right: opens to the left of the cursor", P.x, P.y);
    }
    // no room below: opens above
    {
        const auto P = PlaceInside({ 100, 700 }, { 300, 200 }, Min, Max, Offset);
        Check(P.x == 116 && P.y == 700 - 16 - 200, "no room below: opens above the cursor", P.x, P.y);
    }
    // a corner, and not enough room on either side: pushed in until it is inside with the margin
    {
        const auto P = PlaceInside({ 990, 790 }, { 300, 200 }, Min, Max, Offset);
        Check(P.x >= Min.x + Margin && P.x + 300 <= Max.x - Margin && P.y >= Min.y + Margin && P.y + 200 <= Max.y - Margin, "the corner: all of it inside", P.x, P.y);
    }
    // the cursor at the very left edge, a card that fits only after it: nothing leaves through the left
    {
        const auto P = PlaceInside({ 0, 0 }, { 300, 200 }, Min, Max, Offset);
        Check(P.x == 16 && P.y == 16, "the origin", P.x, P.y);
    }
    // bigger than the box: the top left corner stays inside (the part that is read first)
    {
        const auto P = PlaceInside({ 500, 400 }, { 2000, 2000 }, Min, Max, Offset);
        Check(P.x == Min.x + Margin && P.y == Min.y + Margin, "bigger than the window: top left inside", P.x, P.y);
    }
    // a box that does not start at 0 (a second monitor, a window in the middle of the desktop): the same rules in its own coordinates
    {
        const point Min2{ 1920, 100 }, Max2{ 3000, 900 };
        const auto P = PlaceInside({ 2950, 880 }, { 300, 200 }, Min2, Max2, Offset);
        Check(P.x >= Min2.x + Margin && P.x + 300 <= Max2.x - Margin && P.y >= Min2.y + Margin && P.y + 200 <= Max2.y - Margin, "offset box: all of it inside", P.x, P.y);
    }
    // everywhere in the box, for several sizes that fit: it is always completely inside
    for (const point Size : { point{ 120, 60 }, point{ 480, 260 }, point{ 480, 700 }, point{ 990, 790 } })
        for (float Mx = 0; Mx <= 1000; Mx += 37)
            for (float My = 0; My <= 800; My += 29)
            {
                const auto P = PlaceInside({ Mx, My }, Size, Min, Max, Offset);
                const bool bInside = P.x >= Min.x + Margin && P.y >= Min.y + Margin && P.x + Size.x <= Max.x - Margin && P.y + Size.y <= Max.y - Margin;
                if (!bInside) { Check(false, "a card that fits was not completely inside", P.x, P.y); std::printf("    cursor %.0f,%.0f size %.0fx%.0f\n", Mx, My, Size.x, Size.y); goto done; }
            }
done:
    // A card that grows from a small size to its target: at EVERY step it is completely inside the box, inside the finished card, and the corner nearest the cursor does not move
    for (const point Target : { point{ 160, 80 }, point{ 480, 260 }, point{ 480, 700 }, point{ 900, 760 } })
        for (float Mx = 0; Mx <= 1000; Mx += 41)
            for (float My = 0; My <= 800; My += 31)
            {
                const point Mouse{ Mx, My };
                const point Done = PlaceInside(Mouse, Target, Min, Max, Offset);
                point Anchor{ -1, -1 };
                for (int Step = 0; Step <= 10; ++Step)
                {
                    const float  T = static_cast<float>(Step) / 10.0f;
                    const point  Size{ 40 + (Target.x - 40) * T, 20 + (Target.y - 20) * T };
                    const point  P = PlaceGrowing(Mouse, Size, Target, Min, Max, Offset);
                    const bool bInBox  = P.x >= Min.x + Margin - 0.01f && P.y >= Min.y + Margin - 0.01f && P.x + Size.x <= Max.x - Margin + 0.01f && P.y + Size.y <= Max.y - Margin + 0.01f;
                    const bool bInDone = P.x >= Done.x - 0.01f && P.y >= Done.y - 0.01f && P.x + Size.x <= Done.x + Target.x + 0.01f && P.y + Size.y <= Done.y + Target.y + 0.01f;
                    // the fixed corner: the left edge when the card is after the cursor, else the right edge (same for the top / bottom)
                    const bool bBeforeX = Done.x + Target.x * 0.5f < Mouse.x, bBeforeY = Done.y + Target.y * 0.5f < Mouse.y;
                    const point Corner{ bBeforeX ? P.x + Size.x : P.x, bBeforeY ? P.y + Size.y : P.y };
                    if (Step == 0) Anchor = Corner;
                    const bool bFixed = Corner.x > Anchor.x - 0.01f && Corner.x < Anchor.x + 0.01f && Corner.y > Anchor.y - 0.01f && Corner.y < Anchor.y + 0.01f;
                    if (!bInBox || !bInDone || !bFixed)
                    {
                        Check(false, bInBox ? (bInDone ? "the anchored corner moved while the card grew" : "a step left the finished card") : "a step of the growth left the window", P.x, P.y);
                        std::printf("    cursor %.0f,%.0f target %.0fx%.0f step %d\n", Mx, My, Target.x, Target.y, Step);
                        goto grown;
                    }
                }
                {   // and the last step is the finished card itself, where PlaceInside puts it
                    const point P = PlaceGrowing(Mouse, Target, Target, Min, Max, Offset);
                    if (P.x != Done.x || P.y != Done.y) { Check(false, "the grown card is not where the finished card goes", P.x, P.y); goto grown; }
                }
            }
grown:
    if (g_Failed == 0) std::printf("hint placement: all checks passed\n");
    return g_Failed == 0 ? 0 : 1;
}
