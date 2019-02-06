#!/bin/env python3

import json
import glob
import os.path

made_changes = False

def update_file_contents(fn, contents):
    """Set file contents, only writing if needed to reduce recompilation"""
    global made_changes
    with open(fn, "r+", newline="\n") as f:
        old_contents = f.read()
        if old_contents != contents:
            made_changes = True
            f.seek(0)
            f.truncate()
            f.write(contents)

with open("_defaultConfigs/_newDefault.js") as f:
    default_config = f.read()

update_file_contents(
    "COVER_CONFIG_DEF_CONTENT.h",
    '#define COVER_CONFIG_DEF_CONTENT {}\n'.format(
        json.dumps(default_config.replace("\n", "\r\n"))))

configs = {}
for fn in glob.glob("_defaultConfigs/*.js"):
    basename = os.path.basename(fn)
    if basename[0] == '_':
        continue
    with open(fn) as f:
        configs[basename[:-3]] = f.read()

out = """static const char* buildInCoverConfigs[] = {{
    {}
    ""
}};
static const int buildInCoverConfigCount = {};
""".format(
    "\n".join("{}, {},".format(json.dumps(k + " (build-in)"),
                               json.dumps(v.replace("\n", "\r\n").replace("   ", "\t")))
              for k, v in sorted(configs.items())),
    len(configs))

update_file_contents("BUILD_IN_COVERCONFIGS.h", out);

if made_changes:
    print("cover configs updated")
else:
    print("cover configs unchanged")
