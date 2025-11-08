const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const rlImGui_src_dep = b.dependency("rlImGui_src", .{
        .target = target,
        .optimize = optimize,
    });
    const imgui_dep = b.dependency("imgui", .{
        .target = target,
        .optimize = optimize,
    });
    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
    });

    const rlImGui_mod = b.addModule("rlImGui", .{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    rlImGui_mod.addCSourceFiles(.{
        .root = rlImGui_src_dep.path("./"),
        .files = &.{
            "rlImGui.cpp",
        },
    });

    rlImGui_mod.addIncludePath(rlImGui_src_dep.path("./"));

    const rlImGui_lib = b.addLibrary(.{
        .name = "rlImGui",
        .root_module = rlImGui_mod,
        .linkage = .static,
    });
    rlImGui_lib.linkLibrary(imgui_dep.artifact("imgui"));
    rlImGui_lib.linkLibrary(raylib_dep.artifact("raylib"));

    rlImGui_lib.installHeader(rlImGui_src_dep.path("rlImGui.h"), "rlImGui.h");
    rlImGui_lib.installHeader(rlImGui_src_dep.path("rlImGuiColors.h"), "rlImGuiColors.h");
    rlImGui_lib.installHeader(rlImGui_src_dep.path("extras/IconsFontAwesome6.h"), "extras/IconsFontAwesome6.h");
    rlImGui_lib.installHeader(rlImGui_src_dep.path("extras/FA6FreeSolidFontData.h"), "extras/FA6FreeSolidFontData.h");

    b.installArtifact(rlImGui_lib);
}
