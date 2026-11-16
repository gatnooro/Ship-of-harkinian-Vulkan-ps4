#!/usr/bin/env python3
"""Builds a PS4 fake PKG with the plain homebrew *game* identity that RADV (Vulkan on the PS4 GPU) needs.

    make_pkg.py --eboot eboot.bin --title "Name" --title-id ABCD12345 --label SIXTEENCHARLABEL --version 01.00
                --icon icon0.png [--pic pic1.png] [--file pkg/path=local/path ...] --out out_dir

param.sfo, gp4 and staging follow the Wind Waker HD PS4 port's Maker (maker/wwhd_ps4_maker.py, MIT): APP_TYPE 1,
CATEGORY gd, ATTRIBUTE 2, no right.sprx (an application identity, CATEGORY gde, is for OpenGL/Piglet and freezes
RADV's first GPU mapping), the OpenOrbis piglet sample's sce_module/*.prx, and every folder listed in the gp4.
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

OO = os.environ.get("OPENORBIS", "/opt/OpenOrbis/PS4Toolchain")
PKGTOOL = os.path.join(OO, "bin", "linux", "PkgTool.Core")
LIBSSL11 = os.environ.get("PS4_LIBSSL11", "")  # a folder with libssl.so.1.1, where the system only has OpenSSL 3


def run(cmd, cwd=None):
    env = dict(os.environ, DOTNET_SYSTEM_GLOBALIZATION_INVARIANT="1", CLR_OPENSSL_VERSION_OVERRIDE="1.1",
               LD_LIBRARY_PATH=(LIBSSL11 + ":" if LIBSSL11 else "") + os.environ.get("LD_LIBRARY_PATH", ""))
    r = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if r.returncode != 0:
        sys.exit("failed: %s\n%s" % (" ".join(cmd), r.stdout))
    return r.stdout


def write_gp4(stage, path, content_id):
    files = []
    for r, dirs, fs in os.walk(stage):
        dirs.sort()
        for n in sorted(fs):
            rel = os.path.relpath(os.path.join(r, n), stage).replace("\\", "/")
            if rel != os.path.basename(path):
                files.append(rel)
    root = ET.Element("psproject", {"fmt": "gp4", "version": "1000"})
    vol = ET.SubElement(root, "volume")
    ET.SubElement(vol, "volume_type").text = "pkg_ps4_app"
    ET.SubElement(vol, "volume_id").text = "PS4VOLUME"
    ET.SubElement(vol, "volume_ts").text = time.strftime("%Y-%m-%d %H:%M:%S")
    ET.SubElement(vol, "package", {"content_id": content_id, "passcode": "0" * 32, "storage_type": "digital50",
                                   "app_type": "full"})
    ci = ET.SubElement(vol, "chunk_info", {"chunk_count": "1", "scenario_count": "1"})
    ET.SubElement(ET.SubElement(ci, "chunks"), "chunk", {"id": "0", "layer_no": "0", "label": "Chunk #0"})
    sc = ET.SubElement(ET.SubElement(ci, "scenarios", {"default_id": "0"}), "scenario",
                       {"id": "0", "type": "sp", "initial_chunk_count": "1", "label": "Scenario #0"})
    sc.text = "0"
    fe = ET.SubElement(root, "files", {"img_no": "0"})
    for f in files:
        ET.SubElement(fe, "file", {"targ_path": f, "orig_path": f})
    rootdir = ET.SubElement(root, "rootdir")
    nodes = {"": rootdir}
    for f in files:
        parts = f.split("/")[:-1]
        for i in range(len(parts)):
            key = "/".join(parts[:i + 1])
            if key not in nodes:
                nodes[key] = ET.SubElement(nodes["/".join(parts[:i])], "dir", {"targ_name": parts[i]})
    if "sce_module" not in nodes:
        ET.SubElement(rootdir, "dir", {"targ_name": "sce_module"})
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)
    return len(files)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--eboot", required=True)
    ap.add_argument("--title", required=True)
    ap.add_argument("--title-id", required=True)
    ap.add_argument("--label", required=True, help="16 characters, A-Z 0-9")
    ap.add_argument("--version", default="01.00")
    ap.add_argument("--icon", required=True)
    ap.add_argument("--pic")
    ap.add_argument("--file", action="append", default=[], help="path/in/pkg=local/file")
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", help="output file name (default: <content id>.pkg)")
    a = ap.parse_args()
    if len(a.title_id) != 9 or len(a.label) != 16:
        sys.exit("title id must have 9 characters and the label 16")
    content_id = "IV0000-%s_00-%s" % (a.title_id, a.label)

    a.out = os.path.abspath(a.out)
    stage = os.path.join(a.out, "stage")
    shutil.rmtree(stage, ignore_errors=True)
    os.makedirs(os.path.join(stage, "sce_sys"))
    os.makedirs(os.path.join(stage, "sce_module"))
    shutil.copy2(a.eboot, os.path.join(stage, "eboot.bin"))
    shutil.copy2(a.icon, os.path.join(stage, "sce_sys", "icon0.png"))
    if a.pic:
        shutil.copy2(a.pic, os.path.join(stage, "sce_sys", "pic1.png"))
    for prx in glob.glob(os.path.join(OO, "samples", "piglet", "sce_module", "*.prx")):
        shutil.copy2(prx, os.path.join(stage, "sce_module", os.path.basename(prx)))
    for spec in a.file:
        dst, src = spec.split("=", 1)
        full = os.path.join(stage, dst)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        shutil.copy2(src, full)

    sfo = os.path.join(stage, "sce_sys", "param.sfo")
    run([PKGTOOL, "sfo_new", sfo])
    entries = [("APP_TYPE", "Integer", 4, "1"), ("ATTRIBUTE", "Integer", 4, "2"), ("CATEGORY", "Utf8", 4, "gd"),
               ("SYSTEM_VER", "Integer", 4, "0"), ("APP_VER", "Utf8", 8, a.version),
               ("CONTENT_ID", "Utf8", 48, content_id), ("DOWNLOAD_DATA_SIZE", "Integer", 4, "0"),
               ("TITLE", "Utf8", 128, a.title), ("TITLE_ID", "Utf8", 12, a.title_id),
               ("VERSION", "Utf8", 8, a.version)]
    for name, kind, size, value in entries:
        run([PKGTOOL, "sfo_setentry", sfo, name, "--type", kind, "--maxsize", str(size), "--value", value])

    gp4 = os.path.join(stage, "pkg.gp4")
    n = write_gp4(stage, gp4, content_id)
    a.out = os.path.abspath(a.out)
    tmp = os.path.join(a.out, "pkgtool")
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    run([PKGTOOL, "pkg_build", "pkg.gp4", tmp], cwd=stage)
    built = os.path.join(tmp, content_id + ".pkg")
    if not os.path.isfile(built):
        sys.exit("PkgTool wrote no package")
    final = os.path.join(a.out, a.name or (content_id + ".pkg"))
    os.replace(built, final)
    shutil.rmtree(tmp, ignore_errors=True)
    print("%s (%d files, %.1f MB)" % (final, n, os.path.getsize(final) / 1e6))


if __name__ == "__main__":
    main()
