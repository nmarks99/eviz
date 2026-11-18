# eviz markers v1

eviz renders geometric primitives published as a pvAccess structure. Any
process (IOC, pvxs server, p4p script) can publish it. eviz monitors the PV
named in the scene file:

```toml
[[markers]]
pv = "pva://IOC:Markers"
```

## Structure

```
structure
    structure[] markers
        string   name
        string   type
        double[] position
        double[] orientation
        double[] dimensions
        double[] color
```

| Field         | Required | Description |
|---------------|----------|-------------|
| `name`        | no       | Label only. Need not be unique. Not used by eviz yet. |
| `type`        | yes      | `box`, `sphere` or `cylinder`. |
| `position`    | no       | `[x, y, z]` in meters, world frame. Missing or empty means origin. |
| `orientation` | no       | `[roll, pitch, yaw]` in radians, fixed axis XYZ, `R = Rz(yaw) * Ry(pitch) * Rx(roll)` (same as URDF). Missing or empty means identity. |
| `dimensions`  | yes      | Depends on `type`, see below. All values must be positive, in meters. |
| `color`       | no       | `[r, g, b, a]`, each 0 to 1. Missing or empty means red `[1, 0, 0, 1]`. |

### Dimensions

| Type       | `dimensions`          | Geometry |
|------------|-----------------------|----------|
| `box`      | `[x, y, z]`           | Full edge lengths along local X, Y, Z. Centered on its pose. |
| `sphere`   | `[radius]`            | Centered on its pose. |
| `cylinder` | `[radius, height]`    | Axis along local Z. Centered on its pose. |

## Semantics

- The world frame is Z-up, the same as URDF/ROS.
- Every update is a full snapshot. Markers absent from an update are removed.
- A marker that fails validation (unknown type, wrong number of dimensions,
  non-positive dimension, wrong array length, non-finite value) is skipped
  and a warning is printed. Other markers are still drawn.
- Extra fields are ignored.
