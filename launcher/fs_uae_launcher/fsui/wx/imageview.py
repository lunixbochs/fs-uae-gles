from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx
from .common import update_class

class ImageView(wx.StaticBitmap):

    def __init__(self, parent, image):
        wx.StaticBitmap.__init__(self, parent.get_container(), -1,
                image.bitmap)

update_class(ImageView)
