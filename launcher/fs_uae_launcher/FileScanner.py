from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import hashlib
from .FileDatabase import FileDatabase
from .Settings import Settings

SCAN_EXTENSIONS = [".adf", ".ipf", ".dms", ".rom"]

class FileScanner:
    
    def get_scan_dirs(self):
        dirs = []
        dirs.append(Settings.get_kickstarts_dir())
        dirs.append(Settings.get_floppies_dir())
        return dirs

    def scan(self):
        file_database = FileDatabase()
        file_database.clear()
        scan_dirs = self.get_scan_dirs()
        for dir in scan_dirs:
            self.scan_dir(file_database, dir)
        file_database.commit()

    def scan_dir(self, file_database, dir):
        for name in os.listdir(dir):
            path = os.path.join(dir, name)
            if os.path.isdir(path):
                self.scan_dir(file_database, dir)
                continue
            name, ext = os.path.splitext(path)
            ext = ext.lower()
            if ext not in SCAN_EXTENSIONS:
                continue
            s = hashlib.sha1()
            with open(path, "rb") as f:
                while True:
                    data = f.read(65536)
                    if not data:
                        break
                    s.update(data)
            sha1 = s.hexdigest()
            file_database.add_file(path, sha1)
