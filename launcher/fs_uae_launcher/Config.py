from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import hashlib

class Config:

    default_config = {
        "accuracy": "1",
        "amiga_model": "A500",
        "joystick_port_0": "mouse",
        "joystick_port_0_mode": "mouse",
        "joystick_port_1": "keyboard",
        "joystick_port_1_mode": "joystick",
        "kickstart_file": "",
        "x_kickstart_name": "",
        "x_kickstart_sha1": "",
        #"x_check": "",
        "x_ready": "0",
        }

    config = default_config.copy()

    sync_keys = set([
        "accuracy",
        "amiga_model",
        "joystick_port_0",
        "joystick_port_0_mode",
        "joystick_port_1",
        "joystick_port_1_mode",
        "x_kickstart_name",
        "x_kickstart_sha1",
        #"x_check",
        "x_ready",
    ])
    
    checksum_keys = [
        "accuracy",
        "amiga_model",
        "joystick_port_0_mode",
        "joystick_port_1_mode",
        "x_kickstart_sha1",
    ]

    config_listeners = []

    @classmethod
    def get(cls, key):
        return cls.config.setdefault(key, "")

    @classmethod
    def add_listener(cls, listener):
        cls.config_listeners.append(listener)

    @classmethod
    def set(cls, key, value):
        if cls.get(key) == value:
            print(u"set {0} to {1} - no change".format(key, value))
            return
        cls.config[key] = value
        for listener in cls.config_listeners:
            listener.on_config(key, value)
        if key != "x_ready":
            cls.set("x_ready", "0")

    @classmethod
    def sync_items(cls):
        for key, value in cls.config.iteritems():
            if key in cls.sync_keys:
                yield key, value

    @classmethod
    def checksum(cls):
        return cls.checksum_config(cls.config)

    @classmethod
    def checksum_config(cls, config):
        s = hashlib.sha1()
        for key in cls.checksum_keys:
            value = config[key]
            s.update(unicode(value).encode("UTF-8"))
        return s.hexdigest()
