"""PlatformIO pre script: OS-specific flags for env:native-coverage."""
Import("env")
import os
import sys

build_dir = env.subst("$BUILD_DIR")
env.Append(CPPDEFINES=["BALANSUN_NATIVE_TEST"])
env.Append(CCFLAGS=["-O0"])
if sys.platform == "darwin":
    env.Append(CCFLAGS=["-fprofile-instr-generate", "-fcoverage-mapping"])
    env.Append(LINKFLAGS=["-fprofile-instr-generate"])
    env.Append(
        ENV={
            "LLVM_PROFILE_FILE": os.path.join(build_dir, "%m.profraw"),
        }
    )
else:
    env.Append(CCFLAGS=["-fprofile-arcs", "-ftest-coverage"])
    env.Append(LINKFLAGS=["--coverage", "-lgcov"])
