from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import traceback
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings

class ConfigurationsGroup(fsui.Group):
    
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
        
        label = fsui.BoldLabel(self, "Configurations")
        self.layout2.add(label)
        self.layout2.add_spacer(6)

        self.layout3 = fsui.HorizontalLayout()
        self.layout2.add(self.layout3, fill=True)

        self.scan_button = fsui.Button(self, "Scan")
        self.scan_button.on_activate = self.on_scan
        self.layout3.add(self.scan_button)

        self.layout3.add_spacer(12)

        self.text_field = fsui.TextField(self, "")
        self.layout3.add(self.text_field, expand=True)

        #self.layout3.add_spacer(12)

        #self.scan_button = fsui.Button(self, "Search")
        #self.scan_button.on_activate = self.on_search
        #self.layout3.add(self.scan_button)

    def on_scan(self):
        pass
