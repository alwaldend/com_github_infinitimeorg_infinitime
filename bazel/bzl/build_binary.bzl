load("@bazel_skylib//lib:paths.bzl", "paths")
load("@rules_pkg//pkg:providers.bzl", "PackageFilegroupInfo")

def _impl(ctx):
    runfiles = ctx.runfiles()
    exec = ctx.actions.declare_file("{}.script.sh".format(ctx.label.name))
    root = "${{0}}.runfiles/{}".format(ctx.workspace_name)

    args = []
    args.extend(["build"])
    args.extend(["--src_dir", "{}/{}".format(root, "srcs")])
    args.extend(["--gcc_dir", "{}/{}".format(root, "gcc")])
    args.extend(["--nrfsdk_dir", "{}/{}".format(root, "nrfsdk")])
    args.extend(["--build_dir", "{}/{}".format(root, "build")])
    args.extend(["--build_type", ctx.attr.build_type])

    cmake = ctx.toolchains["@rules_foreign_cc//toolchains:cmake_toolchain"].data
    cmake_files = cmake.target[DefaultInfo].files.to_list()
    args.extend(
        [
            "--cmake",
            "{}/{}/{}".format(root, paths.dirname(cmake_files[0].short_path), cmake.path),
        ],
    )
    runfiles = runfiles.merge(cmake.target[DefaultInfo].default_runfiles)
    runfiles = runfiles.merge(ctx.runfiles(files = cmake_files))

    for tool in ctx.attr.tools:
        args.extend(
            [
                "--tool",
                "{}/{}".format(root, tool[DefaultInfo].files_to_run.executable.short_path),
            ],
        )
        runfiles = runfiles.merge(tool[DefaultInfo].default_runfiles)

    src_symlinks = {}
    for src in ctx.attr.srcs:
        for files, _ in src[PackageFilegroupInfo].pkg_files:
            src_symlinks.update(files.dest_src_map)
    runfiles = runfiles.merge(ctx.runfiles(symlinks = src_symlinks))

    runfiles = runfiles.merge(ctx.attr.build_tool[DefaultInfo].default_runfiles)
    script_content = """\
        #!/usr/bin/env sh
        set -eu
        exec "{root}/{build_tool}" {arguments} "${{@}}"
    """.format(
        root = root,
        build_tool = ctx.executable.build_tool.short_path,
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
        "srcs": attr.label_list(
            providers = [PackageFilegroupInfo],
            mandatory = True,
            doc = "Srcs",
        ),
        "tools": attr.label_list(
            doc = "Binaries that should be on $PATH",
            mandatory = True,
            cfg = "exec",
        ),
        "build_type": attr.string(
            mandatory = True,
            doc = "Cmake build type",
        ),
    },
)
