from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import hashlib
import fs_uae_launcher.fsui as fsui

class ChecksumDialog(fsui.Window):
    
    def __init__(self, parent, path):
        fsui.Window.__init__(self, parent, "Checksumming")
        self.layout = fsui.VerticalLayout()

        label = fsui.BoldLabel(self, "Checksumming file...")
        self.layout.add(label, fill=True)
        self.layout.add_spacer(6)

    def checksum(self, path):
        s = hashlib.sha1()
        with open(path, "rb") as f:
            data = f.read()
            while data:
                s.update(data)
                data = f.read()
        #aa = bb
        return s.hexdigest()
