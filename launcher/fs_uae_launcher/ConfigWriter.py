from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

from .Config import Config

class ConfigWriter:

    @classmethod
    def create_fsuae_config(cls):
        c = []
        c.append("[config]")
        for key in sorted(Config.config.keys()):
            value = Config.config[key]
            c.append(u"{0} = {1}".format(key, value))
        return c
