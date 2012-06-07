from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx

class FileDialog():
    def __init__(self, parent, message="", directory="", file="",
            pattern="*.*"):
        #if parent:
        #    p = parent.get_real_parent()
        #    parent = p._container
        self._window = wx.FileDialog(parent, message, directory, file,
                pattern, wx.FD_DEFAULT_STYLE)
    
    def get_path(self):
        return self._window.GetPath()
    
    def show(self):
        if self._window.ShowModal() == wx.ID_OK:
            return True
        return False

    #def destroy(self):
    #    self._window.Destroy()
