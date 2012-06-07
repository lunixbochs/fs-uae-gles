from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

class Group:
    
    def __init__(self, parent):
        self.parent = parent
        self.position = (0, 0)

    def get_window(self):
        return self.parent.get_window()
    
    def get_container(self):
        return self.parent.get_container()

    def get_min_width(self):
        return self.layout.get_min_width()

    def get_min_height(self):
        return self.layout.get_min_height()

    def set_position(self, position):
        self.position = position
        if self.layout:
            self.layout.set_position(position)

    def set_size(self, size):
        if self.layout:
            self.layout.set_size(size)

    def set_position_and_size(self, position, size):
        self.position = position
        if self.layout:
            self.layout.set_position_and_size(position, size)
