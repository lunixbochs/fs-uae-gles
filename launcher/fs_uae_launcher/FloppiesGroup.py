from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings

class FloppiesGroup(fsui.Group):
    
    def __init__(self, parent):
        fsui.Group.__init__(self, parent)
        self.layout = fsui.HorizontalLayout()
        self.layout.add_spacer(20)
        
        image = fsui.Image("fs_uae_launcher:res/floppy.png")
        self.image_view = fsui.ImageView(self, image)
        self.layout.add(self.image_view)
        
        self.layout.add_spacer(20)
        
        self.layout2 = fsui.VerticalLayout()
        self.layout.add(self.layout2, fill=True, expand=True)
        
        label = fsui.BoldLabel(self, "Floppy Drives")
        self.layout2.add(label)
        self.layout2.add_spacer(6)

        self.floppy_selectors = []
        for i in range(4):
            from .FloppySelector import FloppySelector
            selector = FloppySelector(parent, i)
            self.floppy_selectors.append(selector)
            self.layout2.add(selector, fill=True)
            if i < 3:
                self.layout2.add_spacer(8)
