#include "work.hpp"
#include "constants.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "util/assert.hpp"

namespace hitomi {
auto Work::get_display_name() const -> const std::string& {
    return !japanese_title.empty() ? japanese_title : title;
}

auto Work::get_thumbnail() -> std::optional<std::vector<std::byte>> {
    assert_o(!images.empty());
    return impl::download_binary(images[0].get_thumbnail_url().data(), {.referer = impl::hitomi_referer, .timeout = 15});
}

auto Work::init(const GalleryID id) -> bool {
    this->id = id;

    const auto url = build_string(impl::galleries_url, "/", id, ".js");
    unwrap_ob(buffer, impl::download_binary(url, {.referer = impl::hitomi_referer}));
    const auto json_head = std::find(buffer.begin(), buffer.end(), std::byte('='));
    assert_b(json_head != buffer.end(), "invalid json");
    const auto json_str = std::string_view(std::bit_cast<char*>(json_head + 1), std::bit_cast<char*>(buffer.end()));
    const auto json_r   = json::parse(json_str);
    if(!json_r) {
        WARN(json_r.as_error().cstr());
        WARN("json:", json_str);
        return false;
    }
    const auto& json = json_r.as_value();
    for(auto& [key, value] : json.children) {
        if(key == "title" && value.get_index() == json::Value::index_of<json::String>) {
            title = value.as<json::String>().value;
        } else if(key == "japanese_title" && value.get_index() == json::Value::index_of<json::String>) {
            japanese_title = value.as<json::String>().value;
        } else if(key == "language" && value.get_index() == json::Value::index_of<json::String>) {
            language = value.as<json::String>().value;
        } else if(key == "date" && value.get_index() == json::Value::index_of<json::String>) {
            date = value.as<json::String>().value;
        } else if(key == "tags" && value.get_index() == json::Value::index_of<json::Array>) {
            auto& array = value.as<json::Array>().value;
            for(const auto& t : array) {
                unwrap_pb(obj, t.get<json::Object>());
                unwrap_pb(tag, obj.find<json::String>("tag"));
                auto tag_str = tag.value;

                const auto is_one = [](const json::Value& v) -> bool {
                    if(const auto q = v.get<json::String>()) {
                        return q->value == "1";
                    } else if(const auto q = v.get<json::Number>()) {
                        return q->value == 1;
                    } else {
                        WARN("unknown tag");
                        return false;
                    }
                };

                if(const auto p = obj.find("male"); p && is_one(*p)) {
                    tag_str = "male:" + tag_str;
                } else if(const auto p = obj.find("female"); p && is_one(*p)) {
                    tag_str = "female:" + tag_str;
                }
                tags.emplace_back(tag_str);
            }
        } else if(key == "files" && value.get_index() == json::Value::index_of<json::Array>) {
            auto& array = value.as<json::Array>().value;
            for(const auto& i : array) {
                unwrap_pb(obj, i.get<json::Object>());
                auto image = Image();
                assert_b(image.init(id, obj));
                images.emplace_back(std::move(image));
            }
        } else if(key == "type" && value.get_index() == json::Value::index_of<json::String>) {
            type = value.as<json::String>().value;
        } else if(key == "artists" && value.get_index() == json::Value::index_of<json::Array>) {
            auto& array = value.as<json::Array>().value;
            for(const auto& a : array) {
                unwrap_pb(obj, a.get<json::Object>());
                if(const auto v = obj.find<json::String>("artist")) {
                    artists.emplace_back(v->value);
                }
            }
        } else if(key == "groups" && value.get_index() == json::Value::index_of<json::Array>) {
            auto& array = value.as<json::Array>().value;
            for(auto& g : array) {
                unwrap_pb(obj, g.get<json::Object>());
                if(const auto v = obj.find<json::String>("group")) {
                    groups.emplace_back(v->value);
                }
            }
        } else if(key == "parodys" && value.get_index() == json::Value::index_of<json::Array>) {
            auto& array = value.as<json::Array>().value;
            for(auto& p : array) {
                unwrap_pb(obj, p.get<json::Object>());
                if(const auto v = obj.find<json::String>("parody")) {
                    series.emplace_back(v->value);
                }
            }
        }
    }
    return true;
}
} // namespace hitomi
