from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import fs_uae_launcher.fsui as fsui
from .Config import Config

amiga_models = [
    "Amiga 500",
    "Amiga 1200",                
]

amiga_models_config = [
    "A500",
    "A1200",
]

class ModelSelector(fsui.Panel):
    
    def __init__(self, parent):
        fsui.Panel.__init__(self, parent)
        self.layout = fsui.VerticalLayout()

        self.choice = fsui.Choice(self, amiga_models)

        def amiga_model_change():
            index = self.choice.get_index()
            Config.set("amiga_model", amiga_models_config[index])
        self.choice.on_change = amiga_model_change

        self.layout.add(self.choice, fill=True)
        Config.add_listener(self)

    def on_config(self, key, value):
        if key == 'amiga_model':
            for i, config in enumerate(amiga_models_config):
                if config == value:
                    self.choice.set_index(i)
                    break
            else:
                print("FIXME: could not set model")
