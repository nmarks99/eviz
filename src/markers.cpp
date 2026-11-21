#include "markers.hpp"
#include "raymath.h"
#include "rlgl.h"
#include "urdf.hpp"
#include <pace/pace.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

constexpr Color DEFAULT_COLOR{255, 0, 0, 255};
constexpr int RINGS = 16;
constexpr int SLICES = 24;

pvxs::shared_array<const double> read_doubles(const pvxs::Value& m, const char* field) {
    const pvxs::Value f = m[field];
    if (!f) {
        return {};
    }
    auto arr = f.as<pvxs::shared_array<const double>>();
    for (const double v : arr) {
        if (!std::isfinite(v)) {
            throw std::runtime_error(std::string(field) + " contains a non-finite value");
        }
    }
    return arr;
}

Vector3 read_vec3(const pvxs::Value& m, const char* field) {
    auto arr = read_doubles(m, field);
    if (arr.empty()) {
        return Vector3{0.0f, 0.0f, 0.0f};
    }
    if (arr.size() != 3) {
        throw std::runtime_error(std::string(field) + " must have 3 elements");
    }
    return Vector3{static_cast<float>(arr[0]), static_cast<float>(arr[1]), static_cast<float>(arr[2])};
}

Color read_color(const pvxs::Value& m) {
    auto arr = read_doubles(m, "color");
    if (arr.empty()) {
        return DEFAULT_COLOR;
    }
    if (arr.size() != 4) {
        throw std::runtime_error("color must have 4 elements");
    }
    auto to_byte = [](double v) {
        return static_cast<unsigned char>(std::lround(std::clamp(v, 0.0, 1.0) * 255.0));
    };
    return Color{to_byte(arr[0]), to_byte(arr[1]), to_byte(arr[2]), to_byte(arr[3])};
}

} // namespace

MarkerDisplay::MarkerDisplay(const std::string& pv, pace::Context& ctx, Matrix root_tf)
    : pv_(pv), root_tf_(root_tf) {
    ctx.connect(pv).bind(value_);
}

MarkerDisplay::Marker MarkerDisplay::parse_marker(const pvxs::Value& m, Matrix root_tf) {
    const pvxs::Value type_field = m["type"];
    if (!type_field) {
        throw std::runtime_error("missing type");
    }
    const std::string type = type_field.as<std::string>();

    Marker marker{};
    size_t ndims = 0;
    if (type == "box") {
        marker.type = Type::Box;
        ndims = 3;
    } else if (type == "sphere") {
        marker.type = Type::Sphere;
        ndims = 1;
    } else if (type == "cylinder") {
        marker.type = Type::Cylinder;
        ndims = 2;
    } else {
        throw std::runtime_error("unknown type '" + type + "'");
    }

    auto dims = read_doubles(m, "dimensions");
    if (dims.size() != ndims) {
        throw std::runtime_error(type + " requires " + std::to_string(ndims) + " dimensions, got " +
                                 std::to_string(dims.size()));
    }
    for (const double d : dims) {
        if (d <= 0.0) {
            throw std::runtime_error("dimensions must be positive");
        }
    }
    float* out[3] = {&marker.dims.x, &marker.dims.y, &marker.dims.z};
    for (size_t i = 0; i < ndims; i++) {
        *out[i] = static_cast<float>(dims[i]);
    }

    urdf::Pose pose;
    pose.xyz = read_vec3(m, "position");
    pose.rpy = read_vec3(m, "orientation");
    marker.tf = MatrixMultiply(urdf::pose_to_matrix(pose), root_tf);
    marker.color = read_color(m);
    return marker;
}

void MarkerDisplay::update() {
    if (!value_ || value_.equalInst(last_)) {
        return;
    }
    last_ = value_;
    markers_.clear();

    const pvxs::Value field = value_["markers"];
    if (!field) {
        std::cerr << pv_ << ": missing 'markers' field" << std::endl;
        return;
    }
    pvxs::shared_array<const pvxs::Value> arr;
    try {
        arr = field.as<pvxs::shared_array<const pvxs::Value>>();
    } catch (const std::exception& e) {
        std::cerr << pv_ << ": 'markers' must be a structure array: " << e.what() << std::endl;
        return;
    }

    for (size_t i = 0; i < arr.size(); i++) {
        if (!arr[i]) {
            continue;
        }
        try {
            markers_.push_back(parse_marker(arr[i], root_tf_));
        } catch (const std::exception& e) {
            std::cerr << pv_ << ": markers[" << i << "]: " << e.what() << std::endl;
        }
    }
}

void MarkerDisplay::draw() const {
    const Vector3 origin{0.0f, 0.0f, 0.0f};
    for (const Marker& m : markers_) {
        const Color wire = ColorBrightness(m.color, -0.5f);
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(m.tf));
        switch (m.type) {
        case Type::Box:
            DrawCube(origin, m.dims.x, m.dims.y, m.dims.z, m.color);
            DrawCubeWires(origin, m.dims.x, m.dims.y, m.dims.z, wire);
            break;
        case Type::Sphere:
            DrawSphereEx(origin, m.dims.x, RINGS, SLICES, m.color);
            DrawSphereWires(origin, m.dims.x, RINGS, SLICES, wire);
            break;
        case Type::Cylinder: {
            const Vector3 bottom{0.0f, 0.0f, -m.dims.y / 2.0f};
            const Vector3 top{0.0f, 0.0f, m.dims.y / 2.0f};
            DrawCylinderEx(bottom, top, m.dims.x, m.dims.x, SLICES, m.color);
            DrawCylinderWiresEx(bottom, top, m.dims.x, m.dims.x, SLICES, wire);
            break;
        }
        }
        rlPopMatrix();
    }
}
