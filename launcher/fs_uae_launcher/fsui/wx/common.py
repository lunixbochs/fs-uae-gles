
def enable(self, enable=True):
    self.Enable(enable)

def disable(self):
    self.Enable(False)

def set_position_and_size(self, position, size):
    self.SetDimensions(position[0], position[1], size[0], size[1])
    #self.SetPosition(position)
    #self.SetSize(size)
    #self.set_position(position)
    #self.set_size(size)
    if hasattr(self, "layout"):
        #self.layout.set_position_and_size((0, 0))
        #self.layout.set_position_and_size(size)
        self.layout.set_size(size)


#def get_min_width(self):
#    return self.GetBestSize()[0]

#def get_min_height(self):
#    return self.GetBestSize()[1]

def set_min_height(self, height):
    self.min_height = height

def get_min_width(self):
    if hasattr(self, "layout"):
        return self.layout.get_min_width()
    return self.GetBestSize()[0]

def get_min_height(self):
    height = 0
    if hasattr(self, "min_height"):
        if self.min_height:
            height = max(self.min_height, height)
    if hasattr(self, "layout"):
        height = max(self.layout.get_min_height(), height)
        return height
    return max(height, self.GetBestSize()[1])

def focus(self):
    self.SetFocus()

names = [
    "disable",
    "enable",
    "focus",
    "get_min_height",
    "get_min_width",
    "set_min_height",
    "set_position_and_size",
]

def update_class(klass):
    for name in names:
        if not hasattr(klass, name):
            setattr(klass, name, globals()[name])
