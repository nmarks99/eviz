#include "urdf.hpp"
#include "raymath.h"
#include <sstream>
#include <stdexcept>
#include <tinyxml2.h>

namespace urdf {

namespace {

using tinyxml2::XMLElement;

const char* required_attr(const XMLElement* elem, const char* name) {
    const char* value = elem->Attribute(name);
    if (value == nullptr) {
        throw std::runtime_error(std::string("URDF: <") + elem->Name() + "> missing attribute '" + name + "'");
    }
    return value;
}

Vector3 parse_vec3(const char* str, Vector3 fallback) {
    if (str == nullptr) {
        return fallback;
    }
    std::istringstream ss(str);
    Vector3 v;
    if (!(ss >> v.x >> v.y >> v.z)) {
        throw std::runtime_error(std::string("URDF: invalid vector '") + str + "'");
    }
    return v;
}

Pose parse_origin(const XMLElement* parent) {
    Pose pose;
    if (const XMLElement* origin = parent->FirstChildElement("origin")) {
        pose.xyz = parse_vec3(origin->Attribute("xyz"), pose.xyz);
        pose.rpy = parse_vec3(origin->Attribute("rpy"), pose.rpy);
    }
    return pose;
}

std::filesystem::path resolve_mesh(const std::string& uri, const std::filesystem::path& base_dir) {
    if (uri.rfind("package://", 0) == 0) {
        throw std::runtime_error("URDF: package:// URIs are not supported: " + uri);
    }
    std::filesystem::path path = uri.rfind("file://", 0) == 0 ? uri.substr(7) : uri;
    return path.is_absolute() ? path : base_dir / path;
}

JointType parse_joint_type(const std::string& type) {
    if (type == "fixed") {
        return JointType::Fixed;
    }
    if (type == "revolute") {
        return JointType::Revolute;
    }
    if (type == "continuous") {
        return JointType::Continuous;
    }
    if (type == "prismatic") {
        return JointType::Prismatic;
    }
    throw std::runtime_error("URDF: unsupported joint type '" + type + "'");
}

Link parse_link(const XMLElement* elem, const std::filesystem::path& base_dir) {
    Link link;
    link.name = required_attr(elem, "name");
    for (const XMLElement* vis = elem->FirstChildElement("visual"); vis != nullptr;
         vis = vis->NextSiblingElement("visual")) {
        const XMLElement* geom = vis->FirstChildElement("geometry");
        const XMLElement* mesh = geom ? geom->FirstChildElement("mesh") : nullptr;
        if (mesh == nullptr) {
            continue;
        }
        Visual visual;
        visual.origin = parse_origin(vis);
        visual.mesh = resolve_mesh(required_attr(mesh, "filename"), base_dir);
        visual.scale = parse_vec3(mesh->Attribute("scale"), visual.scale);
        link.visuals.push_back(visual);
    }
    return link;
}

Joint parse_joint(const XMLElement* elem) {
    Joint joint;
    joint.name = required_attr(elem, "name");
    joint.type = parse_joint_type(required_attr(elem, "type"));

    const XMLElement* parent = elem->FirstChildElement("parent");
    const XMLElement* child = elem->FirstChildElement("child");
    if (parent == nullptr || child == nullptr) {
        throw std::runtime_error("URDF: joint '" + joint.name + "' missing <parent> or <child>");
    }
    joint.parent = required_attr(parent, "link");
    joint.child = required_attr(child, "link");
    joint.origin = parse_origin(elem);

    if (const XMLElement* axis = elem->FirstChildElement("axis")) {
        joint.axis = parse_vec3(axis->Attribute("xyz"), joint.axis);
    }
    if (const XMLElement* limit = elem->FirstChildElement("limit")) {
        joint.lower = limit->FloatAttribute("lower", 0.0f);
        joint.upper = limit->FloatAttribute("upper", 0.0f);
    }
    return joint;
}

} // namespace

Robot parse(const std::filesystem::path& path) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.string().c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("URDF: failed to load '" + path.string() + "': " + doc.ErrorStr());
    }

    const XMLElement* root = doc.FirstChildElement("robot");
    if (root == nullptr) {
        throw std::runtime_error("URDF: no <robot> element in '" + path.string() + "'");
    }

    const std::filesystem::path base_dir = path.parent_path();
    Robot robot;
    robot.name = required_attr(root, "name");
    for (const XMLElement* e = root->FirstChildElement("link"); e != nullptr;
         e = e->NextSiblingElement("link")) {
        robot.links.push_back(parse_link(e, base_dir));
    }
    for (const XMLElement* e = root->FirstChildElement("joint"); e != nullptr;
         e = e->NextSiblingElement("joint")) {
        robot.joints.push_back(parse_joint(e));
    }
    return robot;
}

Matrix pose_to_matrix(const Pose& pose) {
    // URDF rpy is fixed-axis X, then Y, then Z: R = Rz(yaw) * Ry(pitch) * Rx(roll)
    Matrix rot = MatrixMultiply(MatrixMultiply(MatrixRotateX(pose.rpy.x), MatrixRotateY(pose.rpy.y)),
                                MatrixRotateZ(pose.rpy.z));
    return MatrixMultiply(rot, MatrixTranslate(pose.xyz.x, pose.xyz.y, pose.xyz.z));
}

} // namespace urdf
