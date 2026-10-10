#ifndef XEDITOR_HINT_PLACEMENT_H
#define XEDITOR_HINT_PLACEMENT_H
#pragma once

// Where a hint (a tooltip card) opens when it must stay INSIDE a box - the window of the app. ImGui gives a window that is not fully inside the window it was opened from a platform
// window of its own (a second OS window), which for a hint is never wanted. Pure arithmetic, no ImGui: hint.h feeds it the cursor and the viewport, and the standalone test
// (smoke/test_hint_placement.py) feeds it numbers.
namespace xeditor::hint
{
    struct point { float x = 0.0f, y = 0.0f; };

    // The top left corner of a window of Size that opens beside Mouse (Offset away from it, after it when there is room, before it when there is not) and is pushed in until all of it
    // is inside [BoxMin, BoxMax] with Margin to spare. A window bigger than the box keeps its top left corner inside: that edge is the one that is read first.
    inline point PlaceInside(point Mouse, point Size, point BoxMin, point BoxMax, float Offset, float Margin = 4.0f) noexcept
    {
        const auto Axis = [&](float M, float S, float Lo, float Hi) noexcept
        {
            float Pos = M + Offset;                                    // after the cursor
            if (Pos + S > Hi - Margin) Pos = M - Offset - S;           // before it when there is no room after
            const float MaxPos = Hi - Margin - S;
            const float MinPos = Lo + Margin;
            if (Pos > MaxPos) Pos = MaxPos;                            // still not inside: pushed in
            if (Pos < MinPos) Pos = MinPos;                            // too big for the box: its top / left stays inside
            return Pos;
        };
        return { Axis(Mouse.x, Size.x, BoxMin.x, BoxMax.x), Axis(Mouse.y, Size.y, BoxMin.y, BoxMax.y) };
    }

    // A card that GROWS (from a small size that is safe anywhere to its Target size) and is inside the box at every step. Where the finished card goes is decided once, for the Target
    // (PlaceInside); the corner of it that is nearest the cursor stays where it is, and the card grows away from that corner. So every frame's rectangle lies inside the finished card's rectangle,
    // which is inside the box: no step is ever outside, and the card does not jump from one side of the cursor to the other while it grows.
    inline point PlaceGrowing(point Mouse, point Size, point Target, point BoxMin, point BoxMax, float Offset, float Margin = 4.0f) noexcept
    {
        if (Size.x > Target.x) Size.x = Target.x;                      // never bigger than the finished card
        if (Size.y > Target.y) Size.y = Target.y;
        const point Done = PlaceInside(Mouse, Target, BoxMin, BoxMax, Offset, Margin);
        const bool  bBeforeX = Done.x + Target.x * 0.5f < Mouse.x;     // the finished card is left of / above the cursor: its right / bottom edge is the fixed one
        const bool  bBeforeY = Done.y + Target.y * 0.5f < Mouse.y;
        return { bBeforeX ? Done.x + Target.x - Size.x : Done.x, bBeforeY ? Done.y + Target.y - Size.y : Done.y };
    }
}

#endif // XEDITOR_HINT_PLACEMENT_H
