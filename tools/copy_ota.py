# PlatformIO post-build: salin firmware.bin ke firmware/firmware_ota.bin
# (siap di-upload lewat tab Firmware di halaman OTA) dan gagalkan build bila
# ukurannya melebihi slot OTA di partitions.csv.
import os
import re
import shutil

Import("env")  # noqa: F821  (disediakan PlatformIO)

PROJECT_DIR = env.subst("$PROJECT_DIR")  # noqa: F821
OUT_NAME = os.path.join("firmware", "firmware_ota.bin")


def read_version():
    path = os.path.join(PROJECT_DIR, "src", "version.h")
    m = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', open(path, encoding="utf-8").read())
    return m.group(1) if m else "?"


def read_slot_size():
    # Ukuran partisi app0 (ota_0) dari partitions.csv
    path = os.path.join(PROJECT_DIR, "partitions.csv")
    for line in open(path, encoding="utf-8"):
        cols = [c.strip() for c in line.split("#")[0].split(",")]
        if len(cols) >= 5 and cols[0] == "app0":
            return int(cols[4], 0)
    return 0x180000


def copy_ota(source, target, env):
    src = str(target[0])
    size = os.path.getsize(src)
    slot = read_slot_size()
    version = read_version()
    if size > slot:
        print(f"ERROR: firmware {size} byte melebihi slot OTA {slot} byte - tidak bisa di-update via OTA!")
        env.Exit(1)
    os.makedirs(os.path.join(PROJECT_DIR, "firmware"), exist_ok=True)
    shutil.copyfile(src, os.path.join(PROJECT_DIR, OUT_NAME))
    print(f"OTA siap: {OUT_NAME} v{version} - {size} byte ({size * 100 // slot}% slot)")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_ota)  # noqa: F821
