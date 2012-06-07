from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import traceback
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings

class KickstartGroup(fsui.Group):
    
    def __init__(self, parent):
        fsui.Group.__init__(self, parent)
        self.layout = fsui.HorizontalLayout()
        self.layout.add_spacer(20)
        
        image = fsui.Image("fs_uae_launcher:res/kickstart.png")
        self.image_view = fsui.ImageView(self, image)
        self.layout.add(self.image_view)
        
        self.layout.add_spacer(20)
        
        self.layout2 = fsui.VerticalLayout()
        self.layout.add(self.layout2, fill=True, expand=True)
        
        label = fsui.BoldLabel(self, "Kickstart")
        self.layout2.add(label)#, expand=True, fill=True)
        self.layout2.add_spacer(6)

        self.layout3 = fsui.HorizontalLayout()
        self.layout2.add(self.layout3, fill=True)

        self.text_field = fsui.TextField(self, "", read_only=True)
        self.layout3.add(self.text_field, expand=True)#, expand=True, fill=True)

        self.layout3.add_spacer(12)

        self.browse_button = fsui.Button(self, "Browse")
        self.browse_button.on_activate = self.on_browse
        self.layout3.add(self.browse_button)

        Config.add_listener(self)

    def on_browse(self):
        default_dir = Settings.get_kickstarts_dir()
        dialog = fsui.FileDialog(self.get_window(), "Choose Kickstart ROM",
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

        Config.set("kickstart_file", path)
        Config.set("x_kickstart_sha1", sha1)
        Config.set("x_kickstart_name", file)

    def on_config(self, key, value):
        if key != "kickstart_file":
            return
        if value:
            dir, file = os.path.split(value)
            self.text_field.set_text(file)
        else:
            self.text_field.set_text(value)
