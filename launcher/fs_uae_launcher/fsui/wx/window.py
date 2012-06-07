from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx
from ..common.element import Element
from .common import update_class

class Window(wx.Frame):

    def __init__(self, parent=None, title=""):
        wx.Frame.__init__(self, parent, -1, title)
        #Element.init(self)
        if parent:
            wx_parent = parent.container
        else:
            wx_parent = None
        self.container = wx.Panel(self)
        self.container.get_window = self.get_window
        #self._window = wx.Frame(wx_parent, -1, title)
        #self._container = wx.Panel(self._window)
        #self.on_create()
        self.Bind(wx.EVT_SIZE, self.__resize_event)
        self.Bind(wx.EVT_WINDOW_DESTROY, self.__destroy_event)

    def get_window(self):
        return self
    
    def get_container(self):
        return self.container
    
    def show(self):
        self.Show()

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
            #self.layout.update()

    def __resize_event(self, event):
        self.on_resize()
        event.Skip()

    def on_destroy(self):
        pass

    def __destroy_event(self, event):
        self.on_destroy()

update_class(Window)
