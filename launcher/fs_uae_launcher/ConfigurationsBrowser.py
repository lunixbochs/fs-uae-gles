from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import traceback
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .Settings import Settings

class ConfigurationsBrowser(fsui.ItemView):
    
    def __init__(self, parent):
        fsui.ItemView.__init__(self, parent)
