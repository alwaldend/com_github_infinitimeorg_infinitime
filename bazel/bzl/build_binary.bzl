load("@bazel_skylib//lib:paths.bzl", "paths")
load("@rules_pkg//pkg:providers.bzl", "PackageFilegroupInfo")

_SCRIPT = """\
#!/usr/bin/env sh
set -eu
for root in "" "${{0}}.runfiles/{workspace_name}/" "${{RUNFILES_DIR:-}}/{workspace_name}/"; do
    if [ -x "${{root}}{bin}" ]; then
        export BAZEL_BINDIR="."
        exec "${{root}}{bin}" {arguments} "${{@}}"
    fi
done
echo "Could not find the binary"
exit 1
"""

def _impl(ctx):
    runfiles = ctx.runfiles()
    exec = ctx.actions.declare_file("{}.script.sh".format(ctx.label.name))
    root = "${root}"

    args = ["build"]

    for flag, dir, attrs in [
        ("--src_dir", "srcs", ctx.attr.srcs),
        ("--gcc_dir", "gcc", ctx.attr.gcc),
        ("--nrfsdk_dir", "nrfsdk", ctx.attr.nrfsdk),
    ]:
        args.extend([flag, "{}{}".format(root, dir)])
        symlinks = {}
        for attr in attrs:
            for pkg_files, _ in attr[PackageFilegroupInfo].pkg_files:
                for dest, src in pkg_files.dest_src_map.items():
                    symlinks["{}/{}".format(dir, dest)] = src
        runfiles = runfiles.merge(ctx.runfiles(symlinks = symlinks))

    runfiles = runfiles.merge(ctx.runfiles(symlinks = symlinks))
    args.extend(["--build_dir", "{}{}".format(root, "build")])
    args.extend(["--build_type", ctx.attr.build_type])

    cmake = ctx.toolchains["@rules_foreign_cc//toolchains:cmake_toolchain"].data
    cmake_files = cmake.target[DefaultInfo].files.to_list()
    args.extend(
        [
            "--cmake",
            "{}{}/{}".format(root, paths.dirname(cmake_files[0].short_path), cmake.path),
        ],
    )
    runfiles = runfiles.merge(cmake.target[DefaultInfo].default_runfiles)
    runfiles = runfiles.merge(ctx.runfiles(files = cmake_files))

    for tool in ctx.attr.tools:
        args.extend(
            [
                "--tool",
                "{}{}".format(root, tool[DefaultInfo].files_to_run.executable.short_path),
            ],
        )
        runfiles = runfiles.merge(tool[DefaultInfo].default_runfiles)

    runfiles = runfiles.merge(ctx.attr.build_tool[DefaultInfo].default_runfiles)
    args.extend(ctx.attr.arguments)
    script_content = _SCRIPT.format(
        bin = ctx.executable.build_tool.short_path,
        workspace_name = ctx.workspace_name,
        arguments = " ".join(['"{}"'.format(arg) for arg in args]),
    )
    ctx.actions.write(
        output = exec,
        is_executable = True,
        content = script_content,
    )

    return [
        DefaultInfo(
            executable = exec,
            runfiles = runfiles,
        ),
    ]

build_binary = rule(
    implementation = _impl,
    executable = True,
    doc = "Build binary",
    toolchains = [
        "@rules_foreign_cc//toolchains:cmake_toolchain",
    ],
    attrs = {
        "build_tool": attr.label(
            executable = True,
            default = "//bazel/py",
            doc = "Build tool",
            cfg = "exec",
        ),
        "arguments": attr.string_list(
            doc = "Arguments",
        ),
        "srcs": attr.label_list(
            providers = [PackageFilegroupInfo],
            doc = "Srcs",
        ),
        "gcc": attr.label_list(
            providers = [PackageFilegroupInfo],
            default = ["@com_alwaldend_com_github_infinitimeorg_infinitime_com_arm_developer_gcc_arm//:srcs"],
            doc = "Gcc",
        ),
        "nrfsdk": attr.label_list(
            providers = [PackageFilegroupInfo],
            default = ["@com_alwaldend_com_github_infinitimeorg_infinitime_com_nordicsemi_developer_nrfsdk//:srcs"],
            doc = "Nrfsdk",
        ),
        "tools": attr.label_list(
            doc = "Binaries that should be on $PATH",
            default = [
                "//bazel/js:lv_font_conv",
                "//bazel/py:adafruit-nrfutil",
                "//src/resources:lv_img_conv",
                "//tools/mcuboot:imgtool_bin",
            ],
            cfg = "exec",
        ),
        "build_type": attr.string(
            default = "Release",
            doc = "Cmake build type",
        ),
    },
)
