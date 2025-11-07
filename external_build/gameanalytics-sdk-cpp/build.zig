const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardOptimizeOption(.{});

    const gameanalytics_sdk_cpp_src_dep = b.dependency("gameanalytics_sdk_cpp_src", .{});

    const gameanalytics_lib_mod = b.addModule("gameanalytics", .{
        .target = target,
        .optimize = mode,
        .link_libc = true,
        .link_libcpp = true,
    });

    gameanalytics_lib_mod.addCSourceFiles(.{
        .root = gameanalytics_sdk_cpp_src_dep.path("./source"),
        .files = &.{
            "gameanalytics/GADevice.cpp",
            "gameanalytics/GAEvents.cpp",
            "gameanalytics/GAHTTPApi.cpp",
            "gameanalytics/GAHealth.cpp",
            "gameanalytics/GALogger.cpp",
            "gameanalytics/GAState.cpp",
            "gameanalytics/GAStore.cpp",
            "gameanalytics/GAThreading.cpp",
            "gameanalytics/GAUtilities.cpp",
            "gameanalytics/GAValidator.cpp",
            "gameanalytics/GameAnalytics.cpp",
            "gameanalytics/GameAnalyticsExtern.cpp",
            "gameanalytics/Platform/GALinux.cpp",
            "gameanalytics/Platform/GAMacOS.cpp",
            "gameanalytics/Platform/GAPlatform.cpp",
            "gameanalytics/Platform/GAUwp.cpp",
            "gameanalytics/Platform/GAWin32.cpp",

            // Dependencies
            "dependencies/crossguid/guid.cpp",
            "dependencies/crypto/aes.cpp",
            "dependencies/crypto/md5.cpp",
            "dependencies/miniz/GA_Zip.cpp",
            "dependencies/stacktrace/stacktrace/call_stack_gcc.cpp",
            "dependencies/stacktrace/stacktrace/call_stack_msvc.cpp",
            "dependencies/stackwalker/StackWalker.cpp",

            // C dependencies
            "dependencies/crypto/hmac_sha2.c",
            "dependencies/crypto/sha2.c",
            "dependencies/sqlite/sqlite3.c",
            "dependencies/zf_log/zf_log.c",
        },
    });

    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./include"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/crossguid"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/crypto"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/sqlite"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/stackwalker"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/stacktrace"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/zf_log"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies/miniz"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/dependencies"));
    gameanalytics_lib_mod.addIncludePath(gameanalytics_sdk_cpp_src_dep.path("./source/gameanalytics"));

    if (target.result.os.tag == .linux) {
        gameanalytics_lib_mod.addCMacro("GUID_STDLIB", "1");
    } else if (target.result.os.tag == .macos) {
        gameanalytics_lib_mod.addCMacro("GUID_CFUUID", "1");
        gameanalytics_lib_mod.linkFramework("CoreFoundation", .{});
        gameanalytics_lib_mod.linkFramework("Foundation", .{});
        gameanalytics_lib_mod.linkFramework("CoreServices", .{});
        gameanalytics_lib_mod.linkFramework("SystemConfiguration", .{});
        gameanalytics_lib_mod.linkFramework("Metal", .{});
        gameanalytics_lib_mod.linkFramework("MetalKit", .{});
        gameanalytics_lib_mod.addCSourceFiles(.{
            .root = gameanalytics_sdk_cpp_src_dep.path("./source"),
            .files = &.{
                "gameanalytics/Platform/GADeviceOSX.mm",
            },
        });
    }

    const gameanalytics_lib = b.addLibrary(.{
        .name = "gameanalytics",
        .root_module = gameanalytics_lib_mod,
        .linkage = .static,
    });

    gameanalytics_lib.installHeader(gameanalytics_sdk_cpp_src_dep.path("include/GameAnalytics/GameAnalytics.h"), "GameAnalytics/GameAnalytics.h");
    gameanalytics_lib.installHeader(gameanalytics_sdk_cpp_src_dep.path("include/GameAnalytics/GATypes.h"), "GameAnalytics/GATypes.h");

    b.installArtifact(gameanalytics_lib);
}
