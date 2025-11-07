const std = @import("std");

fn generateCapnprotoSchema(b: *std.Build, capnp_schema_source: std.Build.LazyPath, module_to_add_to: *std.Build.Module) void {
    const capnp_dep = b.dependency("capnproto", .{});

    //
    // kj library
    //

    const kj_lib_module = b.addModule("kj", .{
        .target = b.graph.host,
        .optimize = .Debug,
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

    //
    // capnproto library
    //

    const capnp_lib_module = b.addModule("capnp", .{
        .target = b.graph.host,
        .optimize = .Debug,
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

    // capnp_lib_module
    const capnp_json_lib_module = b.addModule("capnp-json", .{
        .target = b.graph.host,
        .optimize = .Debug,
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
        .target = b.graph.host,
        .optimize = .Debug,
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
    // capnpc_lib_module.linkLibrary(kj_lib);

    const capnpc_lib = b.addLibrary(.{
        .name = "capnpc",
        .linkage = .static,
        .root_module = capnpc_lib_module,
    });

    //
    // capnp_tool executable
    //

    const capnp_tool_module = b.addModule("capnp_tool", .{
        .target = b.graph.host,
        .optimize = .Debug,
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
    const capnp_tool_run = b.addRunArtifact(capnp_tool_exe);

    capnp_tool_run.addArg("compile");
    capnp_tool_run.addPrefixedDirectoryArg("--src-prefix=", capnp_schema_source.dirname());
    capnp_tool_run.addFileArg(capnp_schema_source);

    const output_dir = capnp_tool_run.addPrefixedOutputDirectoryArg("-oc++:", "capnp-generated");

    module_to_add_to.addIncludePath(output_dir);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    const giewont_server_module = b.addModule("giewont_server", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    const common_sources = [_][]const u8{
        "CameraEntity.cpp",
        "Entity.cpp",
        "Game.cpp",
        "LevelLoader.cpp",
        "nbnet.cpp",
        "PhysEntity.cpp",
        "ResourceManager.cpp",
        "SpawnEntity.cpp",
        "TilemapEntity.cpp",
        "ParticleSystemEntity.cpp",
        "BackdropEntity.cpp",
        "FlagEntity.cpp",
        "DataBinder.cpp",
        "TeamBaseEntity.cpp",
        "GameplayManager.cpp",
        "GrenadeEntity.cpp",
        "entities/character/CharacterEntity.cpp",
        "entities/character/KeyboardCharacterController.cpp",
        "entities/character/CharacterController.cpp",
        "entities/character/DumbAICharacterController.cpp",
        "entities/character/SmartAICharacterController.cpp",
        "entities/character/GoapSensors.cpp",
        "entities/character/GoapInterfaces.cpp",
        "entities/character/GoapSelectors.cpp",
        "entities/character/GoapGoals.cpp",
        "entities/character/GoapActions.cpp",
        "entities/character/GoapGoToEntityAction.cpp",
        "Util.cpp",
        "entities/gui/MainMenuGUIEntity.cpp",
        "entities/gui/TeamChoiceGUIEntity.cpp",
        "entities/decorative/TombstoneEntity.cpp",
        "math/RandUtil.cpp",
        "entities/gameplay/ProjectileEntity.cpp",
        "entities/gameplay/BonusEntity.cpp",
        "entities/debug/DebugMarkerEntity.cpp",
    };

    giewont_server_module.addIncludePath(b.path("src/"));
    giewont_server_module.addIncludePath(b.path("src/math"));
    giewont_server_module.addIncludePath(b.path("src/net"));
    giewont_server_module.addIncludePath(b.path("src/server"));
    giewont_server_module.addIncludePath(b.path("libs/nbnet")); // TODO: add a dependency for this
    generateCapnprotoSchema(b, b.path("src/net/schema.capnp"), giewont_server_module);
    giewont_server_module.addCSourceFiles(.{
        .root = b.path("src/"),
        .files = &(common_sources ++ .{
            "server/main_server.cpp",
            "server/ServerGame.cpp",
        }),
        .flags = &.{"-std=c++20"},
    });

    const giewontServerExe = b.addExecutable(.{
        .name = "giewont_server",
        .root_module = giewont_server_module,
    });

    b.installArtifact(giewontServerExe);
}
