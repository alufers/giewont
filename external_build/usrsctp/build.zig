const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const usrsctp_src_dep = b.dependency("usrsctp_src", .{});

    const usrsctp_lib = b.addLibrary(.{
        .name = "usrsctp",
        .root_module = b.addModule("usrsctp", .{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
        .linkage = .static,
    });

    usrsctp_lib.root_module.addCSourceFiles(.{
        .root = usrsctp_src_dep.path("./usrsctplib"),
        .files = &.{
            "netinet/sctp_asconf.c",
            "netinet/sctp_auth.c",
            "netinet/sctp_bsd_addr.c",
            "netinet/sctp_callout.c",
            "netinet/sctp_cc_functions.c",
            "netinet/sctp_crc32.c",
            "netinet/sctp_indata.c",
            "netinet/sctp_input.c",
            "netinet/sctp_output.c",
            "netinet/sctp_pcb.c",
            "netinet/sctp_peeloff.c",
            "netinet/sctp_sha1.c",
            "netinet/sctp_ss_functions.c",
            "netinet/sctp_sysctl.c",
            "netinet/sctp_timer.c",
            "netinet/sctp_userspace.c",
            "netinet/sctp_usrreq.c",
            "netinet/sctputil.c",
            "netinet6/sctp6_usrreq.c",
            "user_environment.c",
            "user_mbuf.c",
            "user_recv_thread.c",
            "user_socket.c",
        },
    });
    usrsctp_lib.root_module.addCMacro("HAVE_SYS_QUEUE_H", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_LINUX_IF_ADDR_H", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_LINUX_RTNETLINK_H", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_NETINET_IP_ICMP_H", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_NET_ROUTE_H", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_STDATOMIC_H", "1");
    // usrsctp_lib.root_module.addCMacro("HAVE_SA_LEN", "1"); // Not actually present on linux
    usrsctp_lib.root_module.addCMacro("HAVE_SIN_LEN", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_SIN6_LEN", "1");
    usrsctp_lib.root_module.addCMacro("HAVE_SCONN_LEN", "1");
    usrsctp_lib.root_module.addCMacro("__Userspace__", "1");
    usrsctp_lib.root_module.addCMacro("SCTP_SIMPLE_ALLOCATOR", "1");
    usrsctp_lib.root_module.addCMacro("SCTP_PROCESS_LEVEL_LOCKS", "1");

    usrsctp_lib.root_module.addIncludePath(usrsctp_src_dep.path("./usrsctplib"));

    const all_headers = comptime &.{
        "user_atomic.h",
        "user_environment.h",
        "user_inpcb.h",
        "user_ip_icmp.h",
        "user_ip6_var.h",
        "user_malloc.h",
        "user_mbuf.h",
        "user_queue.h",
        "user_recv_thread.h",
        "user_route.h",
        "user_socketvar.h",
        "user_uma.h",
        "usrsctp.h",
        "netinet/sctp_asconf.h",
        "netinet/sctp_auth.h",
        "netinet/sctp_bsd_addr.h",
        "netinet/sctp_callout.h",
        "netinet/sctp_constants.h",
        "netinet/sctp_crc32.h",
        "netinet/sctp_header.h",
        "netinet/sctp_indata.h",
        "netinet/sctp_input.h",
        "netinet/sctp_lock_userspace.h",
        "netinet/sctp_os_userspace.h",
        "netinet/sctp_os.h",
        "netinet/sctp_output.h",
        "netinet/sctp_pcb.h",
        "netinet/sctp_peeloff.h",
        "netinet/sctp_process_lock.h",
        "netinet/sctp_sha1.h",
        "netinet/sctp_structs.h",
        "netinet/sctp_sysctl.h",
        "netinet/sctp_timer.h",
        "netinet/sctp_uio.h",
        "netinet/sctp_var.h",
        "netinet/sctputil.h",
        "netinet/sctp.h",
        "netinet6/sctp6_var.h",
    };

    inline for (all_headers) |h| {
        usrsctp_lib.installHeader(usrsctp_src_dep.path("./usrsctplib/" ++ h), h);
    }

    b.installArtifact(usrsctp_lib);
}
