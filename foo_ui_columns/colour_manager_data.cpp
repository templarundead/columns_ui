#include "pch.h"

#include "colour_manager_data.h"

#include "colour_utils.h"
#include "dark_mode.h"

namespace cui::colours {

ColourSet create_default_colour_set(bool is_dark, colour_mode_t mode)
{
    cui::colours::ColourSet colour_set;
    std::initializer_list<std::tuple<COLORREF&, cui::colours::colour_identifier_t>> colour_identifier_pairs
        = {{colour_set.text, cui::colours::colour_text}, {colour_set.background, cui::colours::colour_background},
            {colour_set.selection_text, cui::colours::colour_selection_text},
            {colour_set.selection_background, cui::colours::colour_selection_background},
            {colour_set.inactive_selection_text, cui::colours::colour_inactive_selection_text},
            {colour_set.inactive_selection_background, cui::colours::colour_inactive_selection_background},
            {colour_set.active_item_frame, cui::colours::colour_active_item_frame}};

    for (auto&& [colour, identifier] : colour_identifier_pairs)
        colour = dark::get_system_colour(get_system_colour_id(identifier), is_dark);

    colour_set.colour_mode = mode;

    return colour_set;
}

} // namespace cui::colours

void ColourManagerData::g_on_common_bool_changed(uint32_t mask)
{
    // Copy the list of callbacks in case someone tries to add or remove one
    // while we're iterating through them
    for (const auto callbacks = m_callbacks; auto&& callback : callbacks) {
        if (m_callbacks.contains(callback))
            callback->on_bool_changed(mask);
    }
}

void ColourManagerData::g_on_common_colour_changed(uint32_t mask)
{
    // Copy the list of callbacks in case someone tries to add or remove one
    // while we're iterating through them
    for (const auto callbacks = m_callbacks; auto&& callback : callbacks)
        if (m_callbacks.contains(callback))
            callback->on_colour_changed(mask);
}

void ColourManagerData::deregister_common_callback(cui::colours::common_callback* p_callback)
{
    m_callbacks.erase(p_callback);
}

void ColourManagerData::register_common_callback(cui::colours::common_callback* p_callback)
{
    m_callbacks.emplace(p_callback);
}

ColourManagerData::ColourManagerData() : cfg_var(g_cfg_guid)
{
    m_global_entry = std::make_shared<Entry>(true);
}

void ColourManagerData::find_by_guid(const GUID& p_guid, entry_ptr_t& p_out)
{
    if (p_guid == pfc::guid_null) {
        p_out = m_global_entry;
        return;
    }
    size_t count = m_entries.get_count();
    for (size_t i = 0; i < count; i++) {
        if (m_entries[i]->guid == p_guid) {
            p_out = m_entries[i];
            return;
        }
    }
    p_out = std::make_shared<Entry>();
    p_out->guid = p_guid;
    m_entries.add_item(p_out);
}

void ColourManagerData::set_data_raw(stream_reader* p_stream, size_t p_sizehint, abort_callback& p_abort)
{
    uint32_t version;
    p_stream->read_lendian_t(version, p_abort);
    if (version <= cfg_version) {
        m_global_entry->read(version, p_stream, p_abort);
        const auto count = p_stream->read_lendian_t<uint32_t>(p_abort);
        m_entries.remove_all();
        for (size_t i = 0; i < count; i++) {
            entry_ptr_t ptr = std::make_shared<Entry>();
            ptr->read(version, p_stream, p_abort);
            m_entries.add_item(ptr);
        }

        try {
            m_global_entry->read_extra_data(version, p_stream, p_abort);

            for (auto&& entry : m_entries) {
                entry->read_extra_data(version, p_stream, p_abort);
            }
        } catch (const exception_io_data_truncation&) {
        }
    }
}

void ColourManagerData::get_data_raw(stream_writer* p_stream, abort_callback& p_abort)
{
    pfc::list_t<GUID> clients;
    {
        service_enum_t<cui::colours::client> clientenum;
        cui::colours::client::ptr ptr;
        while (clientenum.next(ptr))
            clients.add_item(ptr->get_client_guid());
    }

    pfc::array_t<bool> mask;
    size_t i;
    size_t count = m_entries.get_count();
    size_t counter = 0;
    mask.set_count(count);
    for (i = 0; i < count; i++)
        if (mask[i] = clients.have_item(m_entries[i]->guid))
            counter++;

    p_stream->write_lendian_t(static_cast<uint32_t>(cfg_version), p_abort);
    m_global_entry->write(p_stream, p_abort);
    p_stream->write_lendian_t(gsl::narrow<uint32_t>(counter), p_abort);
    for (i = 0; i < count; i++)
        if (mask[i])
            m_entries[i]->write(p_stream, p_abort);

    m_global_entry->write_extra_data(p_stream, p_abort);
    for (auto&& [is_valid, entry] : ranges::views::zip(mask, m_entries))
        if (is_valid)
            entry->write_extra_data(p_stream, p_abort);
}

ColourManagerData::Entry::Entry(bool b_global)
    : light_mode_colours(cui::colours::create_default_colour_set(
        false, b_global ? cui::colours::colour_mode_themed : cui::colours::colour_mode_global))
    , dark_mode_colours(cui::colours::create_default_colour_set(
          true, b_global ? cui::colours::colour_mode_themed : cui::colours::colour_mode_global))
{
}

void cui::colours::ColourSet::read(uint32_t version, stream_reader* stream, abort_callback& aborter)
{
    stream->read_lendian_t((uint32_t&)colour_mode, aborter);
    stream->read_lendian_t(text, aborter);
    stream->read_lendian_t(selection_text, aborter);
    stream->read_lendian_t(inactive_selection_text, aborter);
    stream->read_lendian_t(background, aborter);
    stream->read_lendian_t(selection_background, aborter);
    stream->read_lendian_t(inactive_selection_background, aborter);
    stream->read_lendian_t(active_item_frame, aborter);
    stream->read_lendian_t(use_custom_active_item_frame, aborter);
}

void cui::colours::ColourSet::write(stream_writer* stream, abort_callback& aborter) const
{
    stream->write_lendian_t((uint32_t)colour_mode, aborter);
    stream->write_lendian_t(text, aborter);
    stream->write_lendian_t(selection_text, aborter);
    stream->write_lendian_t(inactive_selection_text, aborter);
    stream->write_lendian_t(background, aborter);
    stream->write_lendian_t(selection_background, aborter);
    stream->write_lendian_t(inactive_selection_background, aborter);
    stream->write_lendian_t(active_item_frame, aborter);
    stream->write_lendian_t(use_custom_active_item_frame, aborter);
}

void ColourManagerData::Entry::read(uint32_t version, stream_reader* stream, abort_callback& aborter)
{
    stream->read_lendian_t(guid, aborter);
    light_mode_colours.read(version, stream, aborter);
}

void ColourManagerData::Entry::read_extra_data(uint32_t version, stream_reader* stream, abort_callback& aborter)
{
    uint32_t size{};
    stream->read_lendian_t(size, aborter);
    stream_reader_limited_ref limited_reader(stream, size);

    dark_mode_colours.read(version, &limited_reader, aborter);
}

void ColourManagerData::Entry::import(
    stream_reader* p_reader, size_t stream_size, uint32_t type, abort_callback& p_abort)
{
    fbh::fcl::Reader reader(p_reader, stream_size, p_abort);
    uint32_t element_id;
    uint32_t element_size;

    while (reader.get_remaining()) {
        reader.read_item(element_id);
        reader.read_item(element_size);

        switch (element_id) {
        case identifier_guid:
            reader.read_item(guid);
            break;
        case identifier_mode:
            reader.read_item((uint32_t&)light_mode_colours.colour_mode);
            break;
        case identifier_light_text:
            reader.read_item(light_mode_colours.text);
            break;
        case identifier_light_selection_text:
            reader.read_item(light_mode_colours.selection_text);
            break;
        case identifier_light_inactive_selection_text:
            reader.read_item(light_mode_colours.inactive_selection_text);
            break;
        case identifier_light_background:
            reader.read_item(light_mode_colours.background);
            break;
        case identifier_light_selection_background:
            reader.read_item(light_mode_colours.selection_background);
            break;
        case identifier_light_inactive_selection_background:
            reader.read_item(light_mode_colours.inactive_selection_background);
            break;
        case identifier_light_custom_active_item_frame:
            reader.read_item(light_mode_colours.active_item_frame);
            break;
        case identifier_light_use_custom_active_item_frame:
            reader.read_item(light_mode_colours.use_custom_active_item_frame);
            break;
        default:
            reader.skip(element_size);
            break;
        }
    }
}

void ColourManagerData::Entry::_export(stream_writer* p_stream, abort_callback& p_abort)
{
    fbh::fcl::Writer out(p_stream, p_abort);
    out.write_item(identifier_guid, guid);
    out.write_item(identifier_mode, (uint32_t)light_mode_colours.colour_mode);
    if (light_mode_colours.colour_mode == cui::colours::colour_mode_custom) {
        out.write_item(identifier_light_text, light_mode_colours.text);
        out.write_item(identifier_light_selection_text, light_mode_colours.selection_text);
        out.write_item(identifier_light_inactive_selection_text, light_mode_colours.inactive_selection_text);
        out.write_item(identifier_light_background, light_mode_colours.background);
        out.write_item(identifier_light_selection_background, light_mode_colours.selection_background);
        out.write_item(
            identifier_light_inactive_selection_background, light_mode_colours.inactive_selection_background);
    }
    out.write_item(identifier_light_use_custom_active_item_frame, light_mode_colours.use_custom_active_item_frame);
    out.write_item(identifier_light_custom_active_item_frame, light_mode_colours.active_item_frame);
}

void ColourManagerData::Entry::write(stream_writer* stream, abort_callback& aborter)
{
    stream->write_lendian_t(guid, aborter);
    light_mode_colours.write(stream, aborter);
}

void ColourManagerData::Entry::write_extra_data(stream_writer* stream, abort_callback& aborter)
{
    stream_writer_buffer_simple item_stream;
    dark_mode_colours.write(&item_stream, aborter);

    stream->write_lendian_t(gsl::narrow<uint32_t>(item_stream.m_buffer.get_size()), aborter);
    stream->write(item_stream.m_buffer.get_ptr(), item_stream.m_buffer.get_size(), aborter);
}
