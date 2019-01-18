#!/bin/env python3

import json
import glob
import os.path

with open("_defaultConfigs/_newDefault.js") as f:
    default_config = f.read()

with open("COVER_CONFIG_DEF_CONTENT.h", "w") as f:
    f.write('#define COVER_CONFIG_DEF_CONTENT {}\n'.format(
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

with open("BUILD_IN_COVERCONFIGS.h", "w") as f:
    f.write(out)
