import argparse
import os
import subprocess
import sys
import typing
import os.path


def cmd_build(ctx: argparse.Namespace) -> None:
    env = os.environ.copy()
    for tool in ctx.tool:
        env["PATH"] = ":".join((env.get("PATH", ""), os.path.abspath(os.path.dirname(tool))))
    src_dir = os.path.abspath(ctx.src_dir)
    build_dir = os.path.abspath(ctx.build_dir)
    gcc_dir = os.path.abspath(ctx.gcc_dir)
    nrfsdk_dir = os.path.abspath(ctx.nrfsdk_dir)
    executable = os.path.abspath(sys.executable)
    cmake = os.path.abspath(ctx.cmake)

    subprocess.run(
        (
            cmake,
            "-G",
            "Unix Makefiles",
            "-S",
            src_dir,
            "-B",
            build_dir,
            f"-DCMAKE_BUILD_TYPE={ctx.build_type}",
            f"-DARM_NONE_EABI_TOOLCHAIN_PATH={gcc_dir}",
            f"-DNRF5_SDK_PATH={nrfsdk_dir}",
            f"-DPython3_EXECUTABLE={executable}",
            "-DBUILD_DFU=1",
            "-DBUILD_RESOURCES=1",
        ),
        env=env,
        check=True,
    )
    subprocess.run(
        (
            cmake,
            "--build",
            build_dir,
            "--config",
            f"{ctx.build_type}",
            "--target",
            "pinetime-mcuboot-app",
        ),
        env=env,
        check=True,
    )


def main(argv: typing.Sequence[str]) -> None:
    parser = argparse.ArgumentParser()
    parser.set_defaults(func=lambda _: parser.print_help)
    subparsers = parser.add_subparsers()
    build = subparsers.add_parser(name="build", help="Build")
    build.add_argument("--cmake", required=True, help="Path to the cmake executable")
    build.add_argument("--src_dir", required=True, help="Source directory")
    build.add_argument("--gcc_dir", required=True, help="Gcc directory")
    build.add_argument("--nrfsdk_dir", required=True, help="Nrfsdk directory")
    build.add_argument("--build_dir", required=True, help="Build directory")
    build.add_argument(
        "--tool",
        default=[],
        action="append",
        required=True,
        help="Tools to add to $PATH",
    )
    build.add_argument("--build_type", required=True, help="Build type")
    build.set_defaults(func=cmd_build)
    args = parser.parse_args(argv[1:])
    args.func(args)


if __name__ == "__main__":
    main(sys.argv)
