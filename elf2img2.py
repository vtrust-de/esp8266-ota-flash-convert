Import("env")

# Custom BIN from ELF
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(" ".join([
        "esptool.py elf2image --version 2",
        "$BUILD_DIR/${PROGNAME}.elf"
    ]), "Building ${PROGNAME} using image version 2")
)

