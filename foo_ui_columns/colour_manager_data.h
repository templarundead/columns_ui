#pragma once

namespace cui::colours {

struct ColourSet {
    colour_mode_t colour_mode{};
    COLORREF text{};
    COLORREF selection_text{};
    COLORREF inactive_selection_text{};
    COLORREF background{};
    COLORREF selection_background{};
    COLORREF inactive_selection_background{};
    COLORREF active_item_frame{};
    bool use_custom_active_item_frame{};

    bool operator==(const ColourSet&) const = default;

    void read(uint32_t version, stream_reader* stream, abort_callback& aborter);
    void write(stream_writer* stream, abort_callback& aborter) const;
};

ColourSet create_default_colour_set(bool is_dark, colour_mode_t mode = colour_mode_global);

} // namespace cui::colours

class ColourManagerData : public cfg_var {
public:
    static const GUID g_cfg_guid;
    enum { cfg_version = 0 };
    void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override;
    void set_data_raw(stream_reader* p_stream, size_t p_sizehint, abort_callback& p_abort) override;

    class Entry {
    public:
        enum ExportItemID {
            identifier_guid,
            identifier_mode,
            identifier_light_background,
            identifier_light_selection_background,
            identifier_light_inactive_selection_background,
            identifier_light_text,
            identifier_light_selection_text,
            identifier_light_inactive_selection_text,
            identifier_light_custom_active_item_frame,
            identifier_light_use_custom_active_item_frame,
            identifier_dark_background,
            identifier_dark_selection_background,
            identifier_dark_inactive_selection_background,
            identifier_dark_text,
            identifier_dark_selection_text,
            identifier_dark_inactive_selection_text,
            identifier_dark_custom_active_item_frame,
            identifier_dark_use_custom_active_item_frame,
        };
        GUID guid{};
        cui::colours::ColourSet light_mode_colours{};
        cui::colours::ColourSet dark_mode_colours{};

        cui::colours::ColourSet& active_colour_set()
        {
            return cui::colours::is_dark_mode_active() ? dark_mode_colours : light_mode_colours;
        }

        void write(stream_writer* stream, abort_callback& aborter);
        void write_extra_data(stream_writer* stream, abort_callback& aborter);
        void _export(stream_writer* p_stream, abort_callback& p_abort);
        virtual void import(stream_reader* p_reader, size_t stream_size, uint32_t type, abort_callback& p_abort);
        void read(uint32_t version, stream_reader* stream, abort_callback& aborter);
        void read_extra_data(uint32_t version, stream_reader* p_stream, abort_callback& p_abort);
        explicit Entry(bool b_global = false);
    };
    using entry_ptr_t = std::shared_ptr<Entry>;
    pfc::list_t<entry_ptr_t> m_entries;
    entry_ptr_t m_global_entry;

    void find_by_guid(const GUID& p_guid, entry_ptr_t& p_out);

    ColourManagerData();

    void register_common_callback(cui::colours::common_callback* p_callback);
    void deregister_common_callback(cui::colours::common_callback* p_callback);

    void g_on_common_colour_changed(uint32_t mask);

    void g_on_common_bool_changed(uint32_t mask);

    std::set<cui::colours::common_callback*> m_callbacks;
};
