from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings

class FloppySelector(fsui.Group):
    
    def __init__(self, parent, drive):
        fsui.Group.__init__(self, parent)

        self.drive = drive
        
        self.layout = fsui.HorizontalLayout()

        self.text_field = fsui.TextField(self, "", read_only=True)
        self.layout.add(self.text_field, expand=True)#, expand=True, fill=True)

        self.layout.add_spacer(12)

        self.browse_button = fsui.Button(self, "Browse")
        self.browse_button.on_activate = self.on_browse
        self.layout.add(self.browse_button)

    def on_browse(self):
        default_dir = Settings.get_floppies_dir()
        dialog = fsui.FileDialog(self.get_window(), "Choose Floppy Image",
                directory=default_dir)
        if not dialog.show():
            return
        path = dialog.get_path()
        
        from .ChecksumDialog import ChecksumDialog
        dialog = ChecksumDialog(self.get_window(), path)
        dialog.show()
        try:
            sha1 = dialog.checksum(path)
        except Exception:
            traceback.print_exc()
            dialog.destroy()
            return
        dialog.destroy()
        
        dir, file = os.path.split(path)
        self.text_field.set_text(file)
        if os.path.normcase(os.path.normpath(dir)) == \
                os.path.normcase(os.path.normpath(default_dir)):
            path = file

        Config.set("floppy_drive_{0}".format(self.drive), path)
        Config.set("x_floppy_drive_{0}_sha1".format(self.drive), sha1)
        Config.set("x_floppy_drive_{0}_name".format(self.drive), file)
