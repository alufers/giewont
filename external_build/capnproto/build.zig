const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    const capnp_dep = b.dependency("capnproto_src", .{});

    //
    // kj library
    //

    const kj_lib_module = b.addModule("kj", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    kj_lib_module.addIncludePath(capnp_dep.path("c++/src"));
    kj_lib_module.addCSourceFiles(.{
        .root = capnp_dep.path("c++/src/kj"),
        .files = &.{
            // Lite
            "array.c++",
            "cidr.c++",
            "list.c++",
            "common.c++",
            "debug.c++",
            "exception.c++",
            "io.c++",
            "memory.c++",
            "mutex.c++",
            "string.c++",
            "source-location.c++",
            "hash.c++",
            "table.c++",
            "thread.c++",
            "main.c++",
            "arena.c++",
            "test-helpers.c++",
            "units.c++",
            "encoding.c++",
            // Heavy
            "refcount.c++",
            "string-tree.c++",
            "time.c++",
            "filesystem.c++",
            "filesystem-disk-unix.c++",
            "filesystem-disk-win32.c++",
            "parse/char.c++",
        },
    });

    const kj_lib = b.addLibrary(.{
        .name = "kj",
        .linkage = .static,
        .root_module = kj_lib_module,
    });

    const kj_headers = .{
        "cidr.h",
        "common.h",
        "units.h",
        "memory.h",
        "refcount.h",
        "array.h",
        "list.h",
        "vector.h",
        "string.h",
        "string-tree.h",
        "source-location.h",
        "hash.h",
        "table.h",
        "map.h",
        "encoding.h",
        "exception.h",
        "debug.h",
        "arena.h",
        "io.h",
        "tuple.h",
        "one-of.h",
        "function.h",
        "mutex.h",
        "thread.h",
        "threadlocal.h",
        "filesystem.h",
        "time.h",
        "main.h",
        "win32-api-version.h",
        "windows-sanity.h",
    };

    b.installArtifact(kj_lib);

    //
    // capnproto library
    //

    const capnp_lib_module = b.addModule("capnp", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    capnp_lib_module.addIncludePath(capnp_dep.path("c++/src"));
    capnp_lib_module.addCSourceFiles(.{
        .root = capnp_dep.path("c++/src/capnp"),
        .files = &.{
            // Lite
            "c++.capnp.c++",
            "blob.c++",
            "arena.c++",
            "layout.c++",
            "list.c++",
            "any.c++",
            "message.c++",
            "schema.capnp.c++",
            "stream.capnp.c++",
            "serialize.c++",
            "serialize-packed.c++",
            // Heavy
            "schema.c++",
            "schema-loader.c++",
            "dynamic.c++",
            "stringify.c++",
        },
    });
    capnp_lib_module.linkLibrary(kj_lib);

    const capnp_lib = b.addLibrary(.{
        .name = "capnp",
        .linkage = .static,
        .root_module = capnp_lib_module,
    });

    const capnp_headers = .{
        "c++.capnp.h",
        "common.h",
        "blob.h",
        "endian.h",
        "layout.h",
        "orphan.h",
        "list.h",
        "any.h",
        "message.h",
        "capability.h",
        "membrane.h",
        "dynamic.h",
        "schema.h",
        "schema.capnp.h",
        "stream.capnp.h",
        "schema-lite.h",
        "schema-loader.h",
        "schema-parser.h",
        "pretty-print.h",
        "serialize.h",
        "serialize-async.h",
        "serialize-packed.h",
        "serialize-text.h",
        "pointer-helpers.h",
        "generated-header-support.h",
        "raw-schema.h",
    };

    inline for (capnp_headers) |s| {
        capnp_lib.installHeader(capnp_dep.path("c++/src/capnp/" ++ s), "capnp/" ++ s);
    }

    inline for (kj_headers) |h| {
        capnp_lib.installHeader(capnp_dep.path("c++/src/kj/" ++ h), "kj/" ++ h);
    }

    b.installArtifact(capnp_lib);

    // capnp_lib_module
    const capnp_json_lib_module = b.addModule("capnp-json", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    capnp_json_lib_module.addIncludePath(capnp_dep.path("c++/src"));
    capnp_json_lib_module.addCSourceFiles(.{
        .root = capnp_dep.path("c++/src/capnp"),
        .files = &.{ "compat/json.c++", "compat/json.capnp.c++" },
    });
    capnp_json_lib_module.linkLibrary(capnp_lib);
    capnp_json_lib_module.linkLibrary(kj_lib);

    const capnp_json_lib = b.addLibrary(.{
        .name = "capnp-json",
        .linkage = .static,
        .root_module = capnp_json_lib_module,
    });

    //
    // capnpc library
    //

    const capnpc_lib_module = b.addModule("capnpc", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });
    capnpc_lib_module.addIncludePath(capnp_dep.path("c++/src"));
    capnpc_lib_module.addCSourceFiles(.{
        .root = capnp_dep.path("c++/src/capnp"),
        .files = &.{
            "compiler/type-id.c++",
            "compiler/error-reporter.c++",
            "compiler/lexer.capnp.c++",
            "compiler/lexer.c++",
            "compiler/grammar.capnp.c++",
            "compiler/parser.c++",
            "compiler/generics.c++",
            "compiler/node-translator.c++",
            "compiler/compiler.c++",
            "schema-parser.c++",
            "serialize-text.c++",
        },
    });
    capnpc_lib_module.linkLibrary(capnp_lib);
    capnpc_lib_module.linkLibrary(kj_lib);

    const capnpc_lib = b.addLibrary(.{
        .name = "capnpc",
        .linkage = .static,
        .root_module = capnpc_lib_module,
    });

    //
    // capnp_tool executable
    //

    const capnp_tool_module = b.addModule("capnp_tool", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    capnp_tool_module.addIncludePath(capnp_dep.path("c++/src"));
    capnp_tool_module.addCSourceFiles(.{ .root = capnp_dep.path("c++/src/capnp/compiler"), .files = &.{ "capnp.c++", "module-loader.c++" } });
    capnp_tool_module.linkLibrary(capnp_lib);
    capnp_tool_module.linkLibrary(kj_lib);
    capnp_tool_module.linkLibrary(capnpc_lib);
    capnp_tool_module.linkLibrary(capnp_json_lib);

    const capnp_tool_exe = b.addExecutable(.{
        .name = "capnp_tool",
        .root_module = capnp_tool_module,
    });
    b.installArtifact(capnp_tool_exe);
    // const write_files = b.addWriteFiles();

    // const install_protos = b.addInstallFileWithDir(capnp_dep.path("c++/src/capnp/c++.capnp"), .{ .custom = "capnp" }, "c++.capnp");
    // const install_protos2 = b.addInstallFileWithDir(capnp_dep.path("c++/src/capnp/schema.capnp"), .{ .custom = "capnp" }, "schema.capnp");

    // // const install_protos = b.addInstallFileWithDir(capnp_dep.path("c++/src/capnp/c++.capnp"), .{ .custom = "capnp" }, "c++.capnp");
    // b.getInstallStep().dependOn(&install_protos.step);
    // b.getInstallStep().dependOn(&install_protos2.step);

    b.addNamedLazyPath("capnp_include_path", capnp_dep.path("c++/src"));
}
