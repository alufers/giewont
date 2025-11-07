const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    const imgui_src_dep = b.dependency("imgui_src", .{});

    const imgui_lib_mod = b.addModule("imgui", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    imgui_lib_mod.addCSourceFiles(.{
        .root = imgui_src_dep.path("./"),
        .files = &.{
            "imgui.cpp",
            "imgui_draw.cpp",
            "imgui_tables.cpp",
            "imgui_widgets.cpp",
            "misc/cpp/imgui_stdlib.cpp",
        },
    });

    imgui_lib_mod.addIncludePath(imgui_src_dep.path("./"));

    const imgui_lib = b.addLibrary(.{
        .name = "imgui",
        .root_module = imgui_lib_mod,
        .linkage = .static,
    });

    imgui_lib.installHeader(imgui_src_dep.path("imgui.h"), "imgui.h");
    imgui_lib.installHeader(imgui_src_dep.path("imconfig.h"), "imconfig.h");
    imgui_lib.installHeader(imgui_src_dep.path("misc/cpp/imgui_stdlib.h"), "misc/cpp/imgui_stdlib.h");

    b.installArtifact(imgui_lib);
}
