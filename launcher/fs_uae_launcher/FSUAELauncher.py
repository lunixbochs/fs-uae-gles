from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import fs_uae_launcher.fsui as fsui
from .MainWindow import MainWindow
import fs_uae_launcher.fs as fs

class FSUAELauncher(fsui.Application):

    def on_create(self):
        print("FSUUAELauncherApplication.on_create")
        window = MainWindow()
        window.show()
