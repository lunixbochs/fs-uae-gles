from __future__ import division
from __future__ import print_function
from __future__ import absolute_import 

import wx
import pkg_resources

class Image:

    def __init__(self, name):
        package, file = name.split(":", 1)
        stream = pkg_resources.resource_stream(package, file)
        
        image = wx.ImageFromStream(stream)
        self.bitmap = wx.BitmapFromImage(image)
