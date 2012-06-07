#!/usr/bin/env python

from fs_uae_launcher.FileScanner import FileScanner
FileScanner().scan()

from fs_uae_launcher.FSUAELauncher import FSUAELauncher
application = FSUAELauncher()
application.run()
