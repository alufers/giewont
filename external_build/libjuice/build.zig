const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libjuice_src_dep = b.dependency("libjuice_src", .{});

    const libjuice_lib = b.addLibrary(.{
        .name = "libjuice",
        .root_module = b.addModule("libjuice", .{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
        .linkage = .static,
    });

    libjuice_lib.root_module.addCSourceFiles(.{
        .root = libjuice_src_dep.path("./src"),
        .files = &.{
            "addr.c",
            "agent.c",
            "crc32.c",
            "const_time.c",
            "conn.c",
            "conn_poll.c",
            "conn_thread.c",
            "conn_mux.c",
            "base64.c",
            "hash.c",
            "hmac.c",
            "ice.c",
            "juice.c",
            "log.c",
            "random.c",
            "server.c",
            "stun.c",
            "timestamp.c",
            "turn.c",
            "udp.c",
        },
    });

    libjuice_lib.root_module.addIncludePath(libjuice_src_dep.path("./include/juice"));
    libjuice_lib.installHeader(libjuice_src_dep.path("./include/juice/juice.h"), "juice/juice.h");

    b.installArtifact(libjuice_lib);
}
