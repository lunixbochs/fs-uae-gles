from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx

class Dialog(wx.Dialog):

    def __init__(self, parent=None, title=""):
        wx.Dialog.__init__(self, parent, -1, title)
        self.container = wx.Panel(self)
        self.container.get_window = self.get_window
        self.Bind(wx.EVT_SIZE, self.__resize_event)
        self.Bind(wx.EVT_WINDOW_DESTROY, self.__destroy_event)

    def get_window(self):
        return self
    
    def get_container(self):
        return self.container
    
    def show(self):
        self.Show()

    def close(self):
        self.Close()

    def show_modal(self):
        return self.ShowModal()

    def end_modal(self, value):
        self.EndModal(value)

    def destroy(self):
        self.Destroy()

    def set_title(self, title):
        self.SetTitle(title)

    def get_size(self):
        return self.GetClientSize()

    def set_size(self, size):
        self.SetClientSize(size)

    def on_create(self):
        pass

    def on_resize(self):
        if self.layout:
            #print("calling layout.set_size", self.get_size())
            self.layout.set_size(self.get_size())
            self.layout.update()

    def __resize_event(self, event):
        self.on_resize()
        event.Skip()

    def on_destroy(self):
        pass

    def __destroy_event(self, event):
        self.on_destroy()
