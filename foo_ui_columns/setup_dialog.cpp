#include "pch.h"

#include "setup_dialog.h"

#include "dark_mode.h"
#include "ng_playlist/ng_playlist.h"
#include "main_window.h"

INT_PTR QuickSetupDialog::SetupDialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        m_wnd = wnd;
        modeless_dialog_manager::g_add(wnd);
        s_instances.emplace_back(this);

        HWND wnd_lv = GetDlgItem(wnd, IDC_LIST);
        HWND wnd_mode = GetDlgItem(wnd, IDC_DARK_MODE);
        HWND wnd_theming = GetDlgItem(wnd, IDC_THEMING);
        HWND wnd_grouping = GetDlgItem(wnd, IDC_GROUPING);

        uih::list_view_set_explorer_theme(wnd_lv);

        const auto monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfo(monitor, &monitor_info)) {
            const auto& rc_work = monitor_info.rcWork;

            RECT rc_dialog{};
            GetWindowRect(wnd, &rc_dialog);

            const unsigned cx = rc_dialog.right - rc_dialog.left;
            const unsigned cy = rc_dialog.bottom - rc_dialog.top;

            unsigned left = (rc_work.right - rc_work.left - cx) / 2;
            unsigned top = (rc_work.bottom - rc_work.top - cy) / 2;

            SetWindowPos(wnd, nullptr, left, top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        }

        ListView_SetExtendedListViewStyleEx(
            wnd_lv, LVS_EX_FULLROWSELECT | 0x10000000, LVS_EX_FULLROWSELECT | 0x10000000);

        LVCOLUMN lvc{};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        // MapDialogWidth(wnd, 228);

        RECT rc;
        GetClientRect(wnd_lv, &rc);

        uih::list_view_insert_column_text(wnd_lv, 0, _T("Preset"), RECT_CX(rc));

        SendMessage(wnd_lv, WM_SETREDRAW, FALSE, 0);

        cfg_layout.get_preset(cfg_layout.get_active(), m_previous_layout);

        LayoutWindow::g_get_default_presets(m_presets);

        LVITEM lvi{};
        lvi.mask = LVIF_TEXT;
        size_t count = m_presets.get_count();
        for (size_t i = 0; i < count; i++) {
            uih::list_view_insert_item_text(wnd_lv, gsl::narrow<int>(i), 0, m_presets[i].m_name, false);
        }

        EnableWindow(wnd_mode, cui::dark::does_os_support_dark_mode());
        ComboBox_InsertString(wnd_mode, 0, L"Light");
        ComboBox_InsertString(wnd_mode, 1, L"Dark");
        ComboBox_InsertString(wnd_mode, 2, L"Use system setting");

        m_previous_mode = static_cast<cui::colours::DarkModeStatus>(cui::colours::dark_mode_status.get());
        ComboBox_SetCurSel(wnd_mode, cui::colours::dark_mode_status.get());

        ComboBox_InsertString(wnd_theming, 0, L"No");
        ComboBox_InsertString(wnd_theming, 1, L"Yes");

        m_previous_light_colour_scheme = g_get_global_colour_scheme(false);
        m_previous_dark_colour_scheme = g_get_global_colour_scheme(true);
        const auto active_colour_scheme = g_get_global_colour_scheme();

        size_t select = -1;
        if (active_colour_scheme == cui::colours::ColourSchemeThemed)
            select = 1;
        else if (active_colour_scheme == cui::colours::ColourSchemeSystem)
            select = 0;

        ComboBox_SetCurSel(wnd_theming, select);

        m_previous_show_grouping = cui::panels::playlist_view::cfg_grouping;
        m_previous_show_artwork = cui::panels::playlist_view::cfg_show_artwork;

        ComboBox_InsertString(wnd_grouping, 0, L"Disabled");
        ComboBox_InsertString(wnd_grouping, 1, L"Groups (without artwork)");
        ComboBox_InsertString(wnd_grouping, 2, L"Groups (with artwork)");

        select = cui::panels::playlist_view::cfg_grouping ? (cui::panels::playlist_view::cfg_show_artwork ? 2 : 1) : 0;
        ComboBox_SetCurSel(wnd_grouping, select);

        // ListView_SetItemState(wnd_lv, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
        SendMessage(wnd_lv, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(wnd_lv, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
        return TRUE;
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
        SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
        return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
    case WM_PAINT:
        uih::handle_modern_background_paint(wnd, GetDlgItem(wnd, IDCANCEL));
        return TRUE;
    case WM_COMMAND:
        switch (wp) {
        case IDCANCEL: {
            if (m_preset_changed)
                cfg_layout.set_preset(cfg_layout.get_active(), m_previous_layout.get_ptr());
            cui::colours::dark_mode_status.set(WI_EnumValue(m_previous_mode));
            g_set_global_colour_scheme(m_previous_light_colour_scheme, false);
            g_set_global_colour_scheme(m_previous_dark_colour_scheme, true);
            cui::panels::playlist_view::cfg_show_artwork = m_previous_show_artwork;
            cui::panels::playlist_view::cfg_grouping = m_previous_show_grouping;
            cui::panels::playlist_view::PlaylistView::g_on_show_artwork_change();
            cui::panels::playlist_view::PlaylistView::g_on_groups_change();

            DestroyWindow(wnd);
            return 0;
        }
        case IDOK:
            DestroyWindow(wnd);
            return 0;
        case CBN_SELCHANGE << 16 | IDC_THEMING: {
            const size_t selection = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            if (selection == 1)
                g_set_global_colour_scheme(cui::colours::ColourSchemeThemed);
            else if (selection == 0)
                g_set_global_colour_scheme(cui::colours::ColourSchemeSystem);
            break;
        }
        case CBN_SELCHANGE << 16 | IDC_DARK_MODE: {
            const auto index = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            cui::colours::dark_mode_status.set(index);
            break;
        }
        case CBN_SELCHANGE << 16 | IDC_GROUPING: {
            size_t selection = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            if (selection >= 2)
                cui::panels::playlist_view::cfg_show_artwork = true;
            if (selection >= 1)
                cui::panels::playlist_view::cfg_grouping = true;
            if (selection <= 1)
                cui::panels::playlist_view::cfg_show_artwork = false;
            if (selection == 0)
                cui::panels::playlist_view::cfg_grouping = false;

            cui::panels::playlist_view::PlaylistView::g_on_show_artwork_change();
            cui::panels::playlist_view::PlaylistView::g_on_groups_change();
            break;
        }
        }
        break;
    case WM_CLOSE:
        DestroyWindow(wnd);
        return 0;
    case WM_NOTIFY: {
        auto lpnm = (LPNMHDR)lp;
        switch (lpnm->idFrom) {
        case IDC_COLUMNS: {
            switch (lpnm->code) {
            case LVN_ITEMCHANGED: {
                auto lpnmlv = (LPNMLISTVIEW)lp;
                if (lpnmlv->iItem != -1 && lpnmlv->iItem >= 0 && (size_t)lpnmlv->iItem < m_presets.get_count()) {
                    if ((lpnmlv->uNewState & LVIS_SELECTED) && !(lpnmlv->uOldState & LVIS_SELECTED)) {
                        uie::splitter_item_ptr ptr;
                        m_presets[lpnmlv->iItem].get(ptr);
                        cfg_layout.set_preset(cfg_layout.get_active(), ptr.get_ptr());
                        m_preset_changed = true;
                    }
                }
            }
                return 0;
            }
        } break;
        }
        break;
    } break;
    case WM_DESTROY:
        std::erase(s_instances, this);
        modeless_dialog_manager::g_remove(wnd);
        break;
    case WM_NCDESTROY:
        return FALSE;
    }

    return FALSE;
}

void QuickSetupDialog::s_run()
{
    uih::modeless_dialog_box(
        IDD_QUICK_SETUP, cui::main_window.get_wnd(), [dialog = std::make_shared<QuickSetupDialog>()](auto&&... args) {
            return dialog->SetupDialogProc(std::forward<decltype(args)>(args)...);
        });
}

void QuickSetupDialog::s_refresh()
{
    for (auto instance : s_instances) {
        ComboBox_SetCurSel(GetDlgItem(instance->m_wnd, IDC_DARK_MODE), cui::colours::dark_mode_status.get());
    }
}
