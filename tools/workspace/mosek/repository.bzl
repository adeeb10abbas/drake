"""
Downloads and unpacks a MOSEK™ archive and makes its headers and
precompiled shared libraries available to be used as a C/C++
dependency.

Example:
    WORKSPACE:
        load("@drake//tools/workspace/mosek:repository.bzl", "mosek_repository")  # noqa
        mosek_repository(name = "foo")

    BUILD:
        cc_library(
            name = "foobar",
            deps = ["@foo//:mosek"],
            srcs = ["bar.cc"],
        )

Argument:
    name: A unique name for this rule.
"""

load("//tools/workspace:execute.bzl", "which")
load("//tools/workspace:os.bzl", "determine_os")

def _impl(repository_ctx):
    # When these values are updated:
    # - tools/dynamic_analysis/tsan.supp may also need updating
    # - LICENSE.third_party may also need updating to match
    #     https://docs.mosek.com/latest/licensing/license-agreement-info.html
    mosek_major_version = 10
    mosek_minor_version = 0
    mosek_patch_version = 46

    os_result = determine_os(repository_ctx)
    if os_result.is_macos or os_result.is_macos_wheel:
        if os_result.macos_arch_result == "arm64":
            mosek_platform = "osxaarch64"
            sha256 = "85724bd519d5fe120b4e8d2676b65143b9ce6dce666a07ca4f44ec54727b5ab5"  # noqa
        else:
            mosek_platform = "osx64x86"
            sha256 = "16885bbee2c1d86e0a3f9d9a2c60bbab1bb88e6f1b843ac1fb8da0c62292344f"  # noqa
    elif os_result.is_ubuntu or os_result.is_manylinux:
        mosek_platform = "linux64x86"
        sha256 = "a6862954137493b74f55c0f2745b7f1672e602cfe9cd8974a95feaf9993f06bf"  # noqa
    else:
        fail(
            "Operating system is NOT supported",
            attr = repository_ctx.os.name,
        )

    # TODO(jwnimmer-tri) Port to use mirrors.bzl.
    template = "https://download.mosek.com/stable/{}.{}.{}/mosektools{}.tar.bz2"  # noqa
    url = template.format(
        mosek_major_version,
        mosek_minor_version,
        mosek_patch_version,
        mosek_platform,
    )
    root_path = repository_ctx.path("")
    strip_prefix = "mosek/{}.{}".format(
        mosek_major_version,
        mosek_minor_version,
    )

    repository_ctx.download_and_extract(
        url,
        root_path,
        sha256 = sha256,
        stripPrefix = strip_prefix,
    )

    platform_prefix = "tools/platform/{}".format(mosek_platform)

    if repository_ctx.os.name == "mac os x":
        install_name_tool = which(repository_ctx, "install_name_tool")

        files = [
            "bin/libtbb.12.dylib",
            "bin/libtbb.12.5.dylib",
            "bin/libmosek64.{}.{}.dylib".format(
                mosek_major_version,
                mosek_minor_version,
            ),
        ]

        for file in files:
            file_path = repository_ctx.path(
                "{}/{}".format(platform_prefix, file),
            )

            result = repository_ctx.execute([
                install_name_tool,
                "-id",
                file_path,
                file_path,
            ])

            if result.return_code != 0:
                fail(
                    "Could NOT change shared library identification name",
                    attr = result.stderr,
                )

        srcs = []

        bin_path = repository_ctx.path("{}/bin".format(platform_prefix))

        linkopts = [
            "-L{}".format(bin_path),
            "-lmosek64",
        ]
    else:
        files = [
            # We use the the MOSEK™ copy of libtbb. The version of libtbb
            # available in Ubuntu is too old.
            "bin/libtbb.so.12",
            "bin/libtbb.so.12.6",
            "bin/libmosek64.so.{}.{}".format(
                mosek_major_version,
                mosek_minor_version,
            ),
        ]

        linkopts = ["-pthread"]
        srcs = ["{}/{}".format(platform_prefix, file) for file in files]

    hdrs = ["{}/h/mosek.h".format(platform_prefix)]
    includes = ["{}/h".format(platform_prefix)]
    files = ["{}/{}".format(platform_prefix, file) for file in files]
    libraries_strip_prefix = ["{}/bin".format(platform_prefix)]

    file_content = """# DO NOT EDIT: generated by mosek_repository()

load("@drake//tools/install:install.bzl", "install", "install_files")
load("@drake//tools/skylark:cc.bzl", "cc_library")

licenses([
    "by_exception_only",  # MOSEK
    "notice",  # fplib AND Zlib
])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mosek",
    srcs = {},
    hdrs = {},
    includes = {},
    linkopts = {},
)

install_files(
    name = "install_libraries",
    dest = "lib",
    files = {},
    strip_prefix = {},
    visibility = ["//visibility:private"],
)

install(
    name = "install",
    docs = [
        "mosek-eula.pdf",
        "@drake//tools/workspace/mosek:drake_mosek_redistribution.txt",
        "@drake//tools/workspace/mosek:LICENSE.third_party",
    ],
    doc_strip_prefix = ["tools/workspace/mosek"],
    allowed_externals = ["@drake//:.bazelproject"],
    deps = [":install_libraries"],
)
    """.format(srcs, hdrs, includes, linkopts, files, libraries_strip_prefix)

    repository_ctx.file(
        "BUILD.bazel",
        content = file_content,
        executable = False,
    )

mosek_repository = repository_rule(implementation = _impl)
