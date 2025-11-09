const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const libsrtp_src_dep = b.dependency("libsrtp_src", .{
        .target = target,
        .optimize = optimize,
    });
    const mbedtls_dep = b.dependency("mbedtls", .{
        .target = target,
        .optimize = optimize,
    });

    const libsrtp_lib = b.addLibrary(.{
        .name = "libsrtp",
        .root_module = b.addModule("libsrtp", .{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
        .linkage = .static,
    });

    libsrtp_lib.root_module.addCSourceFiles(.{
        .root = libsrtp_src_dep.path("./"),
        .files = &.{
            "srtp/srtp.c",

            // Ciphers
            "crypto/cipher/cipher.c",
            "crypto/cipher/cipher_test_cases.c",
            // "crypto/cipher/cipher_test_cases.h",

            "crypto/cipher/null_cipher.c",

            // Ciphers mbedtls
            "crypto/cipher/aes_icm_mbedtls.c",
            "crypto/cipher/aes_gcm_mbedtls.c",

            // Hashes
            "crypto/hash/auth.c",
            "crypto/hash/null_auth.c",
            "crypto/hash/auth_test_cases.c",
            // "crypto/hash/auth_test_cases.h",

            // hashes mbedtls
            "crypto/hash/hmac_mbedtls.c",

            // Kernel sources
            "crypto/kernel/alloc.c",
            "crypto/kernel/crypto_kernel.c",
            "crypto/kernel/err.c",
            "crypto/kernel/key.c",

            // Math sources
            "crypto/math/datatypes.c",

            // Replay sources
            "crypto/replay/rdb.c",
            "crypto/replay/rdbx.c",
        },
    });

    const config_header = b.addConfigHeader(.{
        .style = .{
            .cmake = libsrtp_src_dep.path("./config_in_cmake.h"),
        },
    }, .{
        .PACKAGE_VERSION = "2.7.0",
        .PACKAGE_STRING = "libsrtp 2.7.0",
        .ERR_REPORTING_FILE = "",
        .SIZEOF_UNSIGNED_LONG_CODE = b.fmt("#define SIZEOF_UNSIGNED_LONG {}\n", .{target.result.cTypeByteSize(.ulong)}),
        .SIZEOF_UNSIGNED_LONG_LONG_CODE = b.fmt("#define SIZEOF_UNSIGNED_LONG_LONG {}\n", .{target.result.cTypeByteSize(.ulonglong)}),

        .HAVE_STDLIB_H = 1,
        .HAVE_NETINET_IN_H = 1,
        .HAVE_UNISTD_H = 1,
        .HAVE_INTTYPES_H = 1,
        .HAVE_SYS_TYPES_H = 1,
        .HAVE_STDINT_H = 1,
        .MBEDTLS = 1,
        .GCM = 1,
    });
    libsrtp_lib.root_module.addConfigHeader(config_header);
    libsrtp_lib.root_module.addCMacro("HAVE_CONFIG_H", "1");

    libsrtp_lib.root_module.addIncludePath(libsrtp_src_dep.path("./include"));
    libsrtp_lib.root_module.addIncludePath(libsrtp_src_dep.path("./crypto/include"));
    libsrtp_lib.linkLibrary(mbedtls_dep.artifact("mbedtls"));
    const all_headers = comptime &.{
        "crypto/include/aes.h",
        "crypto/include/aes_icm.h",
        "crypto/include/alloc.h",
        "crypto/include/auth.h",
        "crypto/include/cipher.h",
        "crypto/include/cipher_types.h",
        "crypto/include/crypto_kernel.h",
        "crypto/include/crypto_types.h",
        "crypto/include/datatypes.h",
        "crypto/include/err.h",
        "crypto/include/hmac.h",
        "crypto/include/integers.h",
        "crypto/include/key.h",
        "crypto/include/null_auth.h",
        "crypto/include/null_cipher.h",
        "crypto/include/rdb.h",
        "crypto/include/rdbx.h",
        "crypto/include/sha1.h",
        "include/srtp.h",
        "include/srtp_priv.h",
    };

    inline for (all_headers) |h| {
        libsrtp_lib.installHeader(libsrtp_src_dep.path(h), std.fs.path.basename(h));
    }

    libsrtp_lib.installHeader(config_header.getOutputFile(), "config.h");

    b.installArtifact(libsrtp_lib);
}
