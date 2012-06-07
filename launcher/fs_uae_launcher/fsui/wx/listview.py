from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx
from .common import update_class

class ListView(wx.ListCtrl):
    def __init__(self, parent):
        style = wx.LC_REPORT
        #style = style | wx.LC_NO_HEADER
        wx.ListCtrl.__init__(self, parent.get_container(), -1,
                wx.DefaultPosition, wx.DefaultSize, style)
        #self.Bind(wx.EVT_KEY_DOWN, self.__key_down_event)
        #self.Bind(wx.EVT_TEXT, self.__text_event)
        self.InsertColumn(0, "")

    def remove_items(self):
        self.DeleteAllItems()

    def get_item_count(self):
        return self.GetItemCount()

    def insert_item(self, index, text):
        self.InsertStringItem(index, text)

    def remove_item(self, index):
        self.DeleteItem(index)

update_class(ListView)
