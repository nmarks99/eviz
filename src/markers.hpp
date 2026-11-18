#pragma once
#include "raylib.h"
#include <pvxs/data.h>
#include <string>
#include <vector>

namespace pace {
class Context;
}

class MarkerDisplay {
  public:
    MarkerDisplay(const std::string& pv, pace::Context& ctx, Matrix root_tf);

    void update();
    void draw() const;

  private:
    enum class Type { Box, Sphere, Cylinder };

    struct Marker {
        Type type;
        Matrix tf;
        Vector3 dims;
        Color color;
    };

    static Marker parse_marker(const pvxs::Value& m, Matrix root_tf);

    std::string pv_;
    Matrix root_tf_;
    pvxs::Value value_;
    pvxs::Value last_;
    std::vector<Marker> markers_;
};
