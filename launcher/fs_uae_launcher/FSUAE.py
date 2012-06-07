from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import tempfile
import subprocess
import fs_uae_launcher.fs as fs

class FSUAE:

    @classmethod
    def start_with_config(cls, config):
        print("FSUAE.start_with_config:")
        tf = tempfile.NamedTemporaryFile(suffix=".fs-uae", delete=False)
        with tf:
            for line in config:
                print(line)
                tf.write(line.encode("UTF-8"))
                tf.write("\n")
        args = [tf.name]
        cls.start_with_args(args)

    @classmethod
    def start_with_args(cls, args):
        print("FSUAE.start_with_args:", args)
        exe = cls.find_executable()
        print("using fs-uae executable:", exe)
        args = [exe] + args
        print(args)
        proc = subprocess.Popen(args)
        return proc

    @classmethod
    def find_executable(cls):
        if fs.windows:
            exe = "../FS-UAE.exe"
        elif fs.windows:
            exe = "FS-UAE.app/Contents/MacOS/fs-uae"
        else:
            if os.path.exists("out/fs-uae"):
                # for testing / development
                exe = "out/fs-uae"
            else:
                exe = "fs-uae"
        return exe
