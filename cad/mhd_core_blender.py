"""Overby Industries -- MHD core starter geometry (Blender bpy script).

Builds a simple cylindrical core with a hollow MHD flow channel, a set of
coil-wrap rings around the outside, and a conical exhaust nozzle -- a rough
visual/volumetric starting point for CAD iteration, not a manufacturing
drawing.

Usage: open Blender's Scripting workspace, paste this file's contents (or
open it directly), and run. Dimensions below default to the same benchtop
design point used in sim/src/main.cpp -- change PARAMETERS and re-run to
match whatever point Phase I simulation settles on (see
docs/testing-roadmap.md).
"""

import bpy
from math import pi

# ---------------------------------------------------------------------------
# Parameters (all meters). Mirrors the benchtop candidate in sim/src/main.cpp:
# channel height 0.03 m, width 0.10 m, length 0.40 m.
# ---------------------------------------------------------------------------
R_OUTER = 0.09          # outer shell radius
R_CHANNEL = 0.05        # MHD flow channel radius (hollow core)
CORE_LENGTH = 0.40      # channel length, matches sim/src/main.cpp
COIL_COUNT = 6          # number of coil-wrap rings along the core
COIL_THICKNESS = 0.01   # torus minor radius
NOZZLE_LENGTH = 0.15
NOZZLE_EXIT_RADIUS = 0.12  # throat-to-exit ratio ~1:2.4, within the 1:3-1:7
                            # hybrid range noted in the whitepaper as a
                            # conservative starting point


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def build_outer_shell():
    bpy.ops.mesh.primitive_cylinder_add(
        radius=R_OUTER, depth=CORE_LENGTH, location=(0, 0, 0)
    )
    shell = bpy.context.active_object
    shell.name = "MHD_OuterShell"
    return shell


def build_channel_cavity():
    # A slightly larger cylinder used as a boolean cutter to hollow out the
    # flow channel from the outer shell.
    bpy.ops.mesh.primitive_cylinder_add(
        radius=R_CHANNEL, depth=CORE_LENGTH * 1.1, location=(0, 0, 0)
    )
    cavity = bpy.context.active_object
    cavity.name = "MHD_ChannelCavity"
    return cavity


def hollow_out(shell, cavity):
    mod = shell.modifiers.new(name="ChannelBoolean", type="BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.object = cavity
    bpy.context.view_layer.objects.active = shell
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cavity, do_unlink=True)


def build_coil_wraps():
    spacing = CORE_LENGTH / (COIL_COUNT + 1)
    for i in range(COIL_COUNT):
        z = -CORE_LENGTH / 2 + spacing * (i + 1)
        bpy.ops.mesh.primitive_torus_add(
            major_radius=R_OUTER + COIL_THICKNESS,
            minor_radius=COIL_THICKNESS,
            location=(0, 0, z),
        )
        coil = bpy.context.active_object
        coil.name = f"MHD_Coil_{i:02d}"
        coil.rotation_euler = (pi / 2, 0, 0)


def build_nozzle():
    bpy.ops.mesh.primitive_cone_add(
        radius1=R_CHANNEL,
        radius2=NOZZLE_EXIT_RADIUS,
        depth=NOZZLE_LENGTH,
        location=(0, 0, CORE_LENGTH / 2 + NOZZLE_LENGTH / 2),
    )
    nozzle = bpy.context.active_object
    nozzle.name = "MHD_Nozzle"
    return nozzle


def main():
    bpy.context.scene.unit_settings.system = "METRIC"
    clear_scene()

    shell = build_outer_shell()
    cavity = build_channel_cavity()
    hollow_out(shell, cavity)

    build_coil_wraps()
    build_nozzle()

    print(
        f"MHD core geometry built: outer R={R_OUTER} m, channel R={R_CHANNEL} m, "
        f"length={CORE_LENGTH} m, {COIL_COUNT} coil wraps, nozzle exit R={NOZZLE_EXIT_RADIUS} m."
    )


if __name__ == "__main__":
    main()
