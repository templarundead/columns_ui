#include "pch.h"

namespace cui::colours {

int get_system_colour_id(const colour_identifier_t colour_id)
{
    switch (colour_id) {
    case colours::colour_text:
        return COLOR_WINDOWTEXT;
    case colours::colour_selection_text:
        return COLOR_HIGHLIGHTTEXT;
    case colours::colour_background:
        return COLOR_WINDOW;
    case colours::colour_selection_background:
        return COLOR_HIGHLIGHT;
    case colours::colour_inactive_selection_text:
        return COLOR_BTNTEXT;
    case colours::colour_inactive_selection_background:
        return COLOR_BTNFACE;
    case colours::colour_active_item_frame:
        return COLOR_WINDOWFRAME;
    default:
        uBugCheck();
    }
}

} // namespace cui::colours
