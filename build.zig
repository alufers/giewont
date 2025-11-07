const std = @import("std");

fn generateCapnprotoSchema(b: *std.Build, capnp_schema_source: std.Build.LazyPath, module_to_add_to: *std.Build.Module) void {
    const capnp_dep = b.dependency("capnproto", .{});

    const capnp_tool_run = b.addRunArtifact(capnp_dep.artifact("capnp_tool"));

    capnp_tool_run.addArg("compile");
    capnp_tool_run.addPrefixedDirectoryArg("--src-prefix=", capnp_schema_source.dirname());
    capnp_tool_run.addFileArg(capnp_schema_source);

    capnp_tool_run.addPrefixedDirectoryArg("--import-path=", capnp_dep.namedLazyPath("capnp_include_path"));

    const output_dir = capnp_tool_run.addPrefixedOutputDirectoryArg("-oc++:", "capnp-generated");

    module_to_add_to.addIncludePath(output_dir);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    const capnp_dep = b.dependency("capnproto", .{});
    const imgui_dep = b.dependency("imgui", .{});
    const rlImGui_dep = b.dependency("rlImGui", .{});
    const gameanalytics_sdk_cpp_dep = b.dependency("gameanalytics_sdk_cpp", .{});
    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = mode,
        .linkage = .dynamic,
    });
    const curl_dep = b.dependency("curl", .{});
    const mbedtls_dep = b.dependency("mbedtls", .{});
    const zlib_dep = b.dependency("zlib", .{});
    const nlohmann_json_dep = b.dependency("nlohmann_json", .{});

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

    //
    // Giewont server
    //

    const giewont_server_module = b.addModule("giewont_server", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    giewont_server_module.addIncludePath(b.path("src/"));
    giewont_server_module.addIncludePath(b.path("src/math"));
    giewont_server_module.addIncludePath(b.path("src/net"));
    giewont_server_module.addIncludePath(b.path("src/server"));
    giewont_server_module.addIncludePath(b.path("libs/nbnet")); // TODO: add a dependency for this
    giewont_server_module.addIncludePath(nlohmann_json_dep.path("single_include"));
    giewont_server_module.linkLibrary(capnp_dep.artifact("capnp"));
    giewont_server_module.addCMacro("GIEWONT_IS_SERVER", "1");

    generateCapnprotoSchema(b, b.path("src/net/schema.capnp"), giewont_server_module);
    giewont_server_module.addCSourceFiles(.{
        .root = b.path("src/"),
        .files = &(common_sources ++ .{
            "server/main_server.cpp",
            "server/ServerGame.cpp",
        }),
        .flags = &.{"-std=c++20"},
    });

    const giewont_server_exe = b.addExecutable(.{
        .name = "giewont_server",
        .root_module = giewont_server_module,
    });

    b.installArtifact(giewont_server_exe);

    //
    // Giewont client
    //

    const giewont_client_module = b.addModule("giewont_client", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    giewont_client_module.addIncludePath(b.path("src/"));
    giewont_client_module.addIncludePath(b.path("src/math"));
    giewont_client_module.addIncludePath(b.path("src/net"));
    giewont_client_module.addIncludePath(b.path("libs/nbnet")); // TODO: add a dependency for this
    giewont_client_module.addIncludePath(b.path("src/client"));
    giewont_client_module.addIncludePath(b.path("single_include"));
    giewont_client_module.linkLibrary(capnp_dep.artifact("capnp"));
    giewont_client_module.linkLibrary(capnp_dep.artifact("kj"));
    giewont_client_module.linkLibrary(imgui_dep.artifact("imgui"));
    giewont_client_module.linkLibrary(rlImGui_dep.artifact("rlImGui"));
    giewont_client_module.linkLibrary(gameanalytics_sdk_cpp_dep.artifact("gameanalytics"));
    giewont_client_module.linkLibrary(raylib_dep.artifact("raylib"));
    giewont_client_module.linkLibrary(curl_dep.artifact("curl"));
    giewont_client_module.linkLibrary(mbedtls_dep.artifact("mbedtls"));
    giewont_client_module.linkLibrary(zlib_dep.artifact("z"));

    giewont_client_module.addCMacro("GIEWONT_IS_CLIENT", "1");
    giewont_client_module.addCMacro("GIEWONT_HAS_GRAPHICS", "1");

    generateCapnprotoSchema(b, b.path("src/net/schema.capnp"), giewont_client_module);
    giewont_client_module.addCSourceFiles(.{
        .root = b.path("src/"),
        .files = &(common_sources ++ .{
            "client/main_client.cpp",
            "client/ClientGame.cpp",
            "client/DrawableGame.cpp",
            "client/DebugGUI.cpp",
        }),
        .flags = &.{"-std=c++20"},
    });

    const giewont_client_exe = b.addExecutable(.{
        .name = "giewont_client",
        .root_module = giewont_client_module,
    });

    b.installArtifact(giewont_client_exe);
}
