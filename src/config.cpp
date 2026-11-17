#include "config.hpp"
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <toml++/toml.hpp>

namespace {

[[noreturn]] void fail(const std::string& context, const std::string& msg) {
    throw std::runtime_error(context + ": " + msg);
}

void check_keys(const toml::table& tbl, std::initializer_list<std::string_view> allowed,
                const std::string& context) {
    for (auto&& [key, node] : tbl) {
        bool ok = false;
        for (std::string_view a : allowed) {
            if (key.str() == a) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            fail(context, "unknown key '" + std::string(key.str()) + "'");
        }
    }
}

Vector3 parse_vec3(const toml::table& tbl, std::string_view key, const std::string& context) {
    Vector3 v{0.0f, 0.0f, 0.0f};
    const toml::node* node = tbl.get(key);
    if (node == nullptr) {
        return v;
    }
    const toml::array* arr = node->as_array();
    if (arr == nullptr || arr->size() != 3) {
        fail(context, "'" + std::string(key) + "' must be an array of 3 numbers");
    }
    float* out[3] = {&v.x, &v.y, &v.z};
    for (size_t i = 0; i < 3; i++) {
        auto val = (*arr)[i].value<double>();
        if (!val) {
            fail(context, "'" + std::string(key) + "' must be an array of 3 numbers");
        }
        *out[i] = static_cast<float>(*val);
    }
    return v;
}

JointBinding parse_joint_binding(const std::string& joint, const toml::node& node,
                                 const std::string& context) {
    const toml::table* tbl = node.as_table();
    if (tbl == nullptr) {
        fail(context, "binding must be a table");
    }
    check_keys(*tbl, {"pv", "units", "scale", "offset"}, context);

    JointBinding b;
    b.joint = joint;

    auto pv = (*tbl)["pv"].value<std::string>();
    if (!pv || pv->empty()) {
        fail(context, "missing 'pv'");
    }
    b.pv = *pv;

    if (tbl->contains("units") && tbl->contains("scale")) {
        fail(context, "'units' and 'scale' are mutually exclusive");
    }
    if (tbl->contains("units")) {
        auto units = (*tbl)["units"].value<std::string>();
        if (!units) {
            fail(context, "'units' must be a string");
        }
        b.units = *units;
    }
    if (tbl->contains("scale")) {
        auto scale = (*tbl)["scale"].value<double>();
        if (!scale) {
            fail(context, "'scale' must be a number");
        }
        b.scale = *scale;
    }
    if (tbl->contains("offset")) {
        auto offset = (*tbl)["offset"].value<double>();
        if (!offset) {
            fail(context, "'offset' must be a number");
        }
        b.offset = *offset;
    }
    return b;
}

RobotConfig parse_robot(const toml::table& tbl, const std::filesystem::path& base_dir,
                        const std::string& context) {
    check_keys(tbl, {"urdf", "xyz", "rpy", "joints"}, context);

    RobotConfig robot;
    auto urdf = tbl["urdf"].value<std::string>();
    if (!urdf || urdf->empty()) {
        fail(context, "missing 'urdf'");
    }
    std::filesystem::path urdf_path(*urdf);
    robot.urdf = urdf_path.is_absolute() ? urdf_path : base_dir / urdf_path;
    robot.placement.xyz = parse_vec3(tbl, "xyz", context);
    robot.placement.rpy = parse_vec3(tbl, "rpy", context);

    if (const toml::node* joints_node = tbl.get("joints")) {
        const toml::table* joints = joints_node->as_table();
        if (joints == nullptr) {
            fail(context, "'joints' must be a table");
        }
        for (auto&& [key, node] : *joints) {
            const std::string name(key.str());
            robot.joints.push_back(parse_joint_binding(name, node, context + ".joints." + name));
        }
    }
    return robot;
}

} // namespace

SceneConfig load_scene_config(const std::filesystem::path& path) {
    toml::table root;
    try {
        root = toml::parse_file(path.string());
    } catch (const toml::parse_error& e) {
        std::ostringstream ss;
        ss << path.string() << ": " << e.description() << " (" << e.source().begin << ")";
        throw std::runtime_error(ss.str());
    }

    check_keys(root, {"robot"}, path.string());

    SceneConfig scene;
    const toml::node* robots_node = root.get("robot");
    if (robots_node == nullptr) {
        return scene;
    }
    const toml::array* robots = robots_node->as_array();
    if (robots == nullptr) {
        fail(path.string(), "'robot' must be an array of tables ([[robot]])");
    }

    const std::filesystem::path base_dir = path.parent_path();
    for (size_t i = 0; i < robots->size(); i++) {
        const std::string context = path.string() + ": robot[" + std::to_string(i) + "]";
        const toml::table* tbl = (*robots)[i].as_table();
        if (tbl == nullptr) {
            fail(context, "must be a table");
        }
        scene.robots.push_back(parse_robot(*tbl, base_dir, context));
    }
    return scene;
}
