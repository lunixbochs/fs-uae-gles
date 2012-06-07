from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import os
import fs_uae_launcher.fs as fs

class Settings:
    irc_lobby_server = "fengestad.no"
    irc_game_server = "fengestad.no"
    #nick = "FrodeSolheim"
    #nick.
    
    #lobby_nick_number = 0

    @classmethod
    def get_nick(cls):
        return fs.get_user_name()

    @classmethod
    def get_base_dir(cls):
        path = os.path.join(fs.get_documents_dir(True), "FS-UAE")
        if not os.path.exists(path):
            os.makedirs(path)
        return path

    @classmethod
    def get_kickstarts_dir(cls):
        path = os.path.join(cls.get_base_dir(), "Kickstarts")
        if not os.path.exists(path):
            os.makedirs(path)
        return path

    @classmethod
    def get_floppies_dir(cls):
        path = os.path.join(cls.get_base_dir(), "Floppies")
        if not os.path.exists(path):
            os.makedirs(path)
        return path

    @classmethod
    def get_launcher_dir(cls):
        path = os.path.join(cls.get_base_dir(), "Launcher")
        if not os.path.exists(path):
            os.makedirs(path)
        return path
