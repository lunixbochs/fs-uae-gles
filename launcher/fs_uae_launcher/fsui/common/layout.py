from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

#from .element import LightElement
from .spacer import Spacer

DEBUG = 0

class LayoutChild:
    def __init__(self):
        self.element = None
        self.spacing = 0
        self.expand = False
        self.fill = False

class Layout:
    
    def __init__(self):
        #self.min_size = (0, 0)
        self.position = (0, 0)
        self.size = (0, 0)
        #self.origin = (0, 0)
        self.children = []

    def get_min_size(self):
        return (self.get_min_width(), self.get_min_height())

    def add(self, element, spacing=0, expand=False, fill=False):
        child = LayoutChild()
        child.element = element
        child.spacing = spacing
        child.expand = expand
        child.fill = fill
        self.children.append(child)

    def add_spacer(self, size, size2=None, expand=False):
        self.add(Spacer(size, size2), expand=expand)

    def get_position(self):
        return self.position

    def set_position(self, position):
        self.position = position
        #self.origin = position
        # FIXME: avoid calling update after both set_position and set_size
        self.update()

    def set_size(self, size):
        if DEBUG:
            print("Layout.set_size", size)
        self.size = size
        self.update()

    def set_position_and_size(self, position, size):
        self.position = position
        self.size = size
        self.update()

    def update(self):
        pass

class LinearLayout(Layout):
    
    def __init__(self, vertical):
        Layout.__init__(self)
        self.vertical = vertical

    def update(self):
        available = self.size[self.vertical] 
        if DEBUG:
            print("update, available =", available)
        expanding = 0
        for child in self.children:
            
            child.min_size = [child.element.get_min_width(),
                              child.element.get_min_height()]
            
            child.size = child.min_size[self.vertical]
            #child.size = child.min_size
            available -= child.size
            expanding += child.expand
        if DEBUG:
            print("available", available, "expanding", expanding) 
        if available > 0 and expanding > 0:
            if DEBUG:
                print("distributing extra pixels:", available)
            available2 = available
            for child in self.children:
                extra = int(available2 * (child.expand / expanding))
                if DEBUG:
                    print(child.expand, expanding, extra)
                child.size += extra
                available -= extra
            # some more pixels could be available due to rounding
            if available > 0:
                #print("distributing extra pixels:", available)
                for child in self.children:
                    if child.expand:
                        child.size += 1
                        available -= 1
                        if available == 0:
                            break
        x = 0
        y = 0
        for child in self.children:
            size = [child.min_size[0], child.min_size[1]]

            size[self.vertical] = child.size
            if child.fill:
                size[not self.vertical] = self.size[not self.vertical]

            if DEBUG:
                print(child.element, size)
            
            self_pos = self.get_position()
            position = [self_pos[0] + x, self_pos[1] + y]
            
            if not child.fill:
                # center child
                if self.vertical:
                    #position[0] += (self.size[0] - size[0]) // 2
                    pass
                else:
                    position[1] += (self.size[1] - size[1]) // 2

            child.element.set_position_and_size(position, size)

            if self.vertical:
                y += size[1]
            else:
                x += size[0]

class HorizontalLayout(LinearLayout):
    
    def __init__(self):
        LinearLayout.__init__(self, False)

    def get_min_width(self):
        min_width = 0
        for child in self.children:
            min_width += child.element.get_min_width()
        return min_width
    
    def get_min_height(self):
        min_height = 0
        for child in self.children:
            h = child.element.get_min_height()
            if h > min_height:
                min_height = h
        return min_height

class VerticalLayout(LinearLayout):
    
    def __init__(self):
        LinearLayout.__init__(self, True)
        
    def get_min_width(self):
        min_width = 0
        for child in self.children:
            w = child.element.get_min_width()
            if w > min_width:
                min_width = w
        return min_width

    def get_min_height(self):
        min_height = 0
        for child in self.children:
            min_height += child.element.get_min_height()
        return min_height
