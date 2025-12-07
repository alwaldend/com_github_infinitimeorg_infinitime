def _impl(ctx):
    build_dir = ctx.actions.declare_directory("{}.build".format(ctx.label.name))
    args = ctx.actions.args()
    args.add_all(["--build_dir", build_dir.path])
    args.add_all(ctx.attr.args)
    ctx.actions.run(
        executable = ctx.executable.binary,
        arguments = [args],
        outputs = [build_dir],
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(
            files = depset([build_dir]),
        ),
    ]

build_run_binary = rule(
    implementation = _impl,
    doc = "Run build binary",
    attrs = {
        "args": attr.string_list(
            default = [],
            doc = "Args",
        ),
        "binary": attr.label(
            mandatory = True,
            executable = True,
            cfg = "exec",
            doc = "Binary to run",
        ),
    },
)
