from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import pygame
from .Config import Config

class DeviceManager:
    
    initialized = False
    device_ids = []
    device_names = []
    device_name_count = {}
    
    @classmethod
    def init(cls):
        if cls.initialized:
            return
        
        #pygame.init()
        pygame.joystick.init()
        count = pygame.joystick.get_count()
        
        for i in range(count):
            joy = pygame.joystick.Joystick(i)
            name = joy.get_name()
            name_count = cls.device_name_count.get(name, 0) + 1
            cls.device_name_count[name] = name_count
            if name_count > 1:
                name = name + u" #" + str(name_count)
            cls.device_ids.append(name)
            for i in range(3):
                name = name.replace("  ", " ")
            cls.device_names.append(name)
        
        #pygame.quit()
        
        cls.initialized = True
    
    @classmethod
    def get_joystick_names(cls):
        cls.init()
        return cls.device_names[:]
