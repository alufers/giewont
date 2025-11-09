const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const plog_dep = b.dependency("plog_src", .{});

    const plog_lib = b.addLibrary(.{
        .name = "plog",
        .root_module = b.addModule("plog", .{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
        .linkage = .static,
    });

    plog_lib.root_module.addCSourceFile(.{
        .file = b.path("./dummy.c"),
    });

    plog_lib.installHeadersDirectory(plog_dep.path("./include/plog"), "./plog", .{});

    b.installArtifact(plog_lib);
}
