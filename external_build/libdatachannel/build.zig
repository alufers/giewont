const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libdatachannel_src_dep = b.dependency("libdatachannel_src", .{});
    const plog_dep = b.dependency("plog", .{
        .target = target,
        .optimize = optimize,
    });
    const usrsctp_dep = b.dependency("usrsctp", .{
        .target = target,
        .optimize = optimize,
    });
    const libsrtp_dep = b.dependency("libsrtp", .{
        .target = target,
        .optimize = optimize,
    });
    const mbedtls_dep = b.dependency("mbedtls", .{
        .target = target,
        .optimize = optimize,
    });
    const libjuice_dep = b.dependency("libjuice", .{
        .target = target,
        .optimize = optimize,
    });

    const libdatachannel_lib = b.addLibrary(.{
        .name = "libdatachannel",
        .root_module = b.addModule("libdatachannel", .{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
        .linkage = .static,
    });

    libdatachannel_lib.root_module.addCMacro("MBEDTLS_SSL_DTLS_SRTP", "1");
    libdatachannel_lib.root_module.addCMacro("MBEDTLS_SSL_PROTO_DTLS", "1");

    libdatachannel_lib.root_module.addCSourceFiles(.{
        .root = libdatachannel_src_dep.path("./"),
        .files = &.{
            "src/candidate.cpp",
            "src/channel.cpp",
            "src/configuration.cpp",
            "src/datachannel.cpp",
            "src/dependencydescriptor.cpp",
            "src/description.cpp",
            "src/iceudpmuxlistener.cpp",
            "src/mediahandler.cpp",
            "src/global.cpp",
            "src/message.cpp",
            "src/peerconnection.cpp",
            "src/rtcpreceivingsession.cpp",
            "src/track.cpp",
            "src/websocket.cpp",
            "src/websocketserver.cpp",
            "src/rtppacketizationconfig.cpp",
            "src/rtcpsrreporter.cpp",
            "src/rtppacketizer.cpp",
            "src/rtpdepacketizer.cpp",
            "src/h264rtppacketizer.cpp",
            "src/h264rtpdepacketizer.cpp",
            "src/nalunit.cpp",
            "src/h265rtppacketizer.cpp",
            "src/h265rtpdepacketizer.cpp",
            "src/h265nalunit.cpp",
            "src/av1rtppacketizer.cpp",
            "src/rtcpnackresponder.cpp",
            "src/rtp.cpp",
            "src/capi.cpp",
            "src/plihandler.cpp",
            "src/pacinghandler.cpp",
            "src/rembhandler.cpp",

            // Impl headers
            "src/impl/certificate.cpp",
            "src/impl/channel.cpp",
            "src/impl/datachannel.cpp",
            "src/impl/dtlssrtptransport.cpp",
            "src/impl/dtlstransport.cpp",
            "src/impl/icetransport.cpp",
            "src/impl/iceudpmuxlistener.cpp",
            "src/impl/init.cpp",
            "src/impl/peerconnection.cpp",
            "src/impl/logcounter.cpp",
            "src/impl/sctptransport.cpp",
            "src/impl/threadpool.cpp",
            "src/impl/tls.cpp",
            "src/impl/track.cpp",
            "src/impl/utils.cpp",
            "src/impl/processor.cpp",
            "src/impl/sha.cpp",
            "src/impl/pollinterrupter.cpp",
            "src/impl/pollservice.cpp",
            "src/impl/http.cpp",
            "src/impl/httpproxytransport.cpp",
            "src/impl/tcpserver.cpp",
            "src/impl/tcptransport.cpp",
            "src/impl/tlstransport.cpp",
            "src/impl/transport.cpp",
            "src/impl/verifiedtlstransport.cpp",
            "src/impl/websocket.cpp",
            "src/impl/websocketserver.cpp",
            "src/impl/wstransport.cpp",
            "src/impl/wshandshake.cpp",
        },
    });
    libdatachannel_lib.root_module.addCMacro("RTC_ENABLE_MEDIA", "1");
    libdatachannel_lib.root_module.addCMacro("RTC_ENABLE_WEBSOCKET", "1");

    libdatachannel_lib.root_module.addCMacro("USE_MBEDTLS", "1");

    libdatachannel_lib.linkLibrary(plog_dep.artifact("plog"));
    libdatachannel_lib.linkLibrary(usrsctp_dep.artifact("usrsctp"));
    libdatachannel_lib.linkLibrary(libsrtp_dep.artifact("libsrtp"));
    libdatachannel_lib.linkLibrary(mbedtls_dep.artifact("mbedtls"));
    libdatachannel_lib.linkLibrary(libjuice_dep.artifact("libjuice"));

    libdatachannel_lib.root_module.addIncludePath(libdatachannel_src_dep.path("./include/rtc"));
    libdatachannel_lib.root_module.addIncludePath(libdatachannel_src_dep.path("./src/impl"));
    libdatachannel_lib.root_module.addIncludePath(libdatachannel_src_dep.path("./src"));

    b.installArtifact(libdatachannel_lib);
}
