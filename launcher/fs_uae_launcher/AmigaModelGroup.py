from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import fs_uae_launcher.fsui as fsui
from .Config import Config
from .FileDatabase import FileDatabase

amiga_models = [
    "Amiga 500",
    "Amiga 1200",                
]

amiga_models_config = [
    "A500",
    "A1200",
]

class AmigaModelGroup(fsui.Group):
    
    def __init__(self, parent):
        fsui.Group.__init__(self, parent)
        self.layout = fsui.HorizontalLayout()
        self.layout.add_spacer(20)
        
        image = fsui.Image("fs_uae_launcher:res/model.png")
        self.image_view = fsui.ImageView(self, image)
        self.layout.add(self.image_view)
        
        self.layout.add_spacer(20)
        
        self.layout2 = fsui.VerticalLayout()
        self.layout.add(self.layout2, fill=True, expand=True)
        
        #self.layout3 = fsui.HorizontalLayout()
        #self.layout2.add(self.layout3, fill=True, expand=True)

        label = fsui.BoldLabel(self, "Amiga Model")
        self.layout2.add(label)#, expand=True, fill=True)
        self.layout2.add_spacer(6)

        self.layout3 = fsui.HorizontalLayout()
        self.layout2.add(self.layout3, fill=True)

        self.model_choice = fsui.Choice(self, amiga_models)
        self.model_choice.on_change = self.on_model_change

        self.layout3.add(self.model_choice, expand=True)

        self.layout3.add_spacer(12)

        accuracy_levels = ["Accurate", "Less Accurate", "Least Accurate"]
        
        self.accuracy_choice = fsui.Choice(self, accuracy_levels)
        self.accuracy_choice.on_change = self.on_accuracy_change
        self.layout3.add(self.accuracy_choice, expand=False)

        Config.add_listener(self)

    def on_model_change(self):
        index = self.model_choice.get_index()
        model = amiga_models_config[index]
        Config.set("amiga_model", model)
        self.set_kickstart_from_model()

    def set_kickstart_from_model(self):
        model = Config.get("amiga_model")
        if model == "A500":
            checksums = [
                    "891e9a547772fe0c6c19b610baf8bc4ea7fcb785",
            ]
        elif model == "A1200":
            checksums = [
                    "e21545723fe8374e91342617604f1b3d703094f1",
            ]
        for checksum in checksums:            
            path = FileDatabase.get_instance().find_file(sha1=checksum)
            if path:
                Config.set("kickstart_file", path)
                # FIXME: set sha1 and name x_options also
                break

    def on_accuracy_change(self):
        index = self.accuracy_choice.get_index()
        Config.set("accuracy", str(1 - index))

    def on_config(self, key, value):
        if key == 'amiga_model':
            for i, config in enumerate(amiga_models_config):
                if config == value:
                    self.model_choice.set_index(i)
                    break
            else:
                print("FIXME: could not set model")
        elif key == 'accuracy':
            index = 1 - int(value)
            self.accuracy_choice.set_index(index)
