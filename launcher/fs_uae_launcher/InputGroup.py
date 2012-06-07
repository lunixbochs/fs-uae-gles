from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings
from .DeviceManager import DeviceManager

joystick_mode_values = ["nothing", "mouse", "joystick"]
joystick_mode_titles = ["Nothing", "Mouse", "Joystick"]

joystick_values = ["none", "mouse", "keyboard"]

class InputGroup(fsui.Group):
    
    def __init__(self, parent, port):
        self.port = port
        self.device_option_key = "joystick_port_{0}".format(port)
        self.mode_option_key = "joystick_port_{0}_mode".format(port)
        
        fsui.Group.__init__(self, parent)
        self.layout = fsui.HorizontalLayout()
        self.layout.add_spacer(20)
        
        image = fsui.Image("fs_uae_launcher:res/joystick.png")
        self.image_view = fsui.ImageView(self, image)
        self.layout.add(self.image_view)
        
        self.layout.add_spacer(20)
        
        self.layout2 = fsui.VerticalLayout()
        self.layout.add(self.layout2, fill=True, expand=True)
        
        if port == 0:
            heading = "Mouse Port (Port {0})".format(port)
        else:
            heading = "Joystick Port (Port {0})".format(port)
        label = fsui.BoldLabel(self, heading)
            
        self.layout2.add(label)#, expand=True, fill=True)
        self.layout2.add_spacer(6)

        self.layout3 = fsui.HorizontalLayout()
        self.layout2.add(self.layout3, fill=True)

        #self.text_field = fsui.TextField(self, "")
        #self.layout3.add(self.text_field, expand=True)#, expand=True, fill=True)
        #self.layout3.add_spacer(12)
        #self.browse_button = fsui.Button(self, "Browse")
        #self.browse_button.on_activate = self.on_browse
        #self.layout3.add(self.browse_button)

        self.mode_choice = fsui.Choice(self, joystick_mode_titles)
        self.mode_choice.on_change = self.on_mode_change
        if port == 0:
            self.mode_choice.set_index(1)
        else:
            self.mode_choice.set_index(2)
            
        self.layout3.add(self.mode_choice)
        
        self.layout3.add_spacer(12)

        devices = ["No Host Device", "Mouse",
                "Joystick (Emulated by Keyboard)"]
        for i, name in enumerate(DeviceManager.get_joystick_names()):
            devices.append(name)
            joystick_values.append(DeviceManager.device_ids[i])
        
        self.device_choice = fsui.ComboBox(self, devices, read_only=True)
        self.device_choice.on_change = self.on_device_change
        if port == 0:
            self.device_choice.set_index(1)
        else:
            self.device_choice.set_index(2)

        self.layout3.add(self.device_choice, expand=True)
        
        Config.add_listener(self)

    def on_mode_change(self):
        index = self.mode_choice.get_index()
        Config.set(self.mode_option_key, joystick_mode_values[index])

    def on_device_change(self):
        index = self.device_choice.get_index()
        Config.set(self.device_option_key, joystick_values[index])

    def on_config(self, key, value):
        if key == self.mode_option_key:
            for i, config in enumerate(joystick_mode_values):
                if config == value:
                    self.mode_choice.set_index(i)
                    break
            else:
                print("FIXME: could not set model")
