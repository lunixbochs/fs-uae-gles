from __future__ import division
from __future__ import print_function
from __future__ import absolute_import

import uuid
import fs_uae_launcher.fsui as fsui
from .Config import Config
from .IRC import IRC
from .FSUAE import FSUAE
from .ConfigWriter import ConfigWriter
from .Netplay import Netplay
from .FileDatabase import FileDatabase

bottom_button_height = 32

class MainWindow(fsui.Window):
    
    def __init__(self):
        fsui.Window.__init__(self, None, "FS-UAE Launcher")
        #self.set_size((1000, 600))
        
        self.layout = fsui.HorizontalLayout()
        #self.layout.add_spacer(12)
        
        #from .KickstartGroup import KickstartGroup
        #self.kickstart_group = KickstartGroup(self)
        #self.layout.add(self.kickstart_group, fill=True, expand=True)
        #return
        
        #panel = fsui.Panel(self)
        #self.layout.add(panel, fill=True, expand=True)
        #panel.layout = fsui.HorizontalLayout()
        #panel.layout.add_spacer(12)
        #button = fsui.Button(panel, "Title")
        #panel.layout.add(button, fill=True, expand=True)
        #return
        
        self.left_side = fsui.Panel(self)
        #self.left_side = self
        # FIXME: HACK
        #def get_left_min_width():
        #    return 440
        #self.left_side.get_min_width = get_left_min_width
        #self.left_side.set_size((300, 300))
        self.create_left_side(self.left_side)
        self.layout.add(self.left_side, expand=False, fill=True)
        
        self.layout.add_spacer(20)
        
        self.right_side = fsui.Panel(self)
        self.create_right_side(self.right_side)
        self.layout.add(self.right_side, expand=True, fill=True)

        self.layout.add_spacer(20)
        self.layout.set_size(self.get_size())
        self.layout.update()
        
        self.init_net_play()
        
        self.set_size(self.layout.get_min_size())
        fsui.call_later(500, self.on_timer)
        
        #IRC.add_listener(self)
        
        #self.irc.start()
        
    def init_net_play(self):
        #from fs_uae_launcher.IRCHandler import IRCHandler
        #self.irc = IRCHandler(self)
        #IRC.irc = self.irc # self.lobby_panel.irc
        #self.irc.lobby = self.lobby_panel
        #self.irc.game = self.game_panel
        #self.irc.start()
        pass

    def separator(self, parent):
        parent.layout.add_spacer(14)
        layout = fsui.HorizontalLayout()
        parent.layout.add(layout, fill=True)
        layout.add_spacer(20, 0)
        layout.add(fsui.Separator(parent), expand=True)
        #layout.add_spacer(20)
        parent.layout.add_spacer(14)

    def create_left_side(self, parent):
        parent.layout = fsui.VerticalLayout()
        parent.layout.add_spacer(440, 20)
       
        from .AmigaModelGroup import AmigaModelGroup
        self.model_selector = AmigaModelGroup(parent)
        parent.layout.add(self.model_selector, fill=True)

        self.separator(parent)
        
        from .KickstartGroup import KickstartGroup
        self.kickstart_group = KickstartGroup(parent)
        parent.layout.add(self.kickstart_group, fill=True)

        self.separator(parent)

        from .FloppiesGroup import FloppiesGroup
        self.floppies_group = FloppiesGroup(parent)
        parent.layout.add(self.floppies_group, fill=True)

        self.separator(parent)
        
        from .InputGroup import InputGroup
        self.port0_group = InputGroup(parent, 0)
        parent.layout.add(self.port0_group, fill=True)
        
        self.separator(parent)
        
        from .InputGroup import InputGroup
        self.port1_group = InputGroup(parent, 1)
        parent.layout.add(self.port1_group, fill=True)
        
        parent.layout.add_spacer(20 + 20, expand=1)
        
        layout = fsui.HorizontalLayout()
        parent.layout.add(layout, fill=True)

        layout.add_spacer(20)

        button = fsui.Button(parent, "Settings")
        button.disable()
        button.set_min_height(bottom_button_height)
        layout.add(button)
        layout.add_spacer(10)

        layout.add_spacer(0, expand=True)
        
        #self.join_game_button = fsui.Button(parent, "   Join Game   ")
        #layout.add(self.join_game_button)

        #layout.add_spacer(10)
        #self.create_game_button = fsui.Button(parent, "   Create Game   ")
        #layout.add(self.create_game_button)

        #layout.add_spacer(10, expand=True)

        button = fsui.Button(parent, "Net Play")
        button.disable()
        button.set_min_height(bottom_button_height)
        layout.add(button)
        layout.add_spacer(10)

        button = fsui.Button(parent, "Options")
        button.disable()
        button.set_min_height(bottom_button_height)
        layout.add(button)
        layout.add_spacer(10)

        self.start_button = fsui.Button(parent, "Start")
        self.start_button.set_min_height(bottom_button_height)
        self.start_button.on_activate = self.on_start_button
        layout.add(self.start_button)
        
        parent.layout.add_spacer(20)

    def create_right_side(self, parent):
        parent.layout = fsui.VerticalLayout()
        parent.layout.add_spacer(440, 20)
        
        from .ConfigurationsGroup import ConfigurationsGroup
        self.configurations_group = ConfigurationsGroup(parent)
        parent.layout.add(self.configurations_group, fill=True)

        parent.layout.add_spacer(14)
        
        layout2 = fsui.HorizontalLayout()
        layout2.add_spacer(20)
        
        from .ConfigurationsBrowser import ConfigurationsBrowser
        self.configurations_browser = ConfigurationsBrowser(parent)
        layout2.add(self.configurations_browser, fill=True, expand=True)
        parent.layout.add(layout2, fill=True, expand=True)
        #parent.layout.add(self.configurations_browser, fill=True, expand=True)
        
        parent.layout.add_spacer(20)
        
        """
        from .LobbyPanel import LobbyPanel
        self.lobby_panel = LobbyPanel(parent)
        parent.layout.add(self.lobby_panel, expand=True, fill=True)
        
        parent.layout.add_spacer(20)
        
        from .GamePanel import GamePanel
        self.game_panel = GamePanel(parent)
        parent.layout.add(self.game_panel, expand=True, fill=True)
        
        parent.layout.add_spacer(20)
        
        layout = fsui.HorizontalLayout()
        parent.layout.add(layout, fill=True)

        layout.add_spacer(20, expand=1)

        self.ready_button = fsui.Button(parent, "   Ready   ")
        self.ready_button.on_activate = self.on_ready_button
        layout.add(self.ready_button)

        layout.add_spacer(10)

        self.leave_game_button = fsui.Button(parent, "   Leave Game   ")
        layout.add(self.leave_game_button)
        
        #layout.add_spacer(10)
        #self.join_game_button = fsui.Button(parent, "   Join Game   ")
        #layout.add(self.join_game_button)

        layout.add_spacer(10)

        self.create_join_button = fsui.Button(parent, "   Create/Join Game   ")
        self.create_join_button.on_activate = self.on_join_button
        layout.add(self.create_join_button)

        #layout.add_spacer(10)
        #self.create_game_button = fsui.Button(parent, "   Create Game   ")
        #layout.add(self.create_game_button)
        
        #layout.add_spacer(10)
        #self.start_button = fsui.Button(parent, "   Start Game   ")
        #layout.add(self.start_button)
        
        parent.layout.add_spacer(20)
        """

    def on_ready_button(self):
        Config.set("x_ready", "1")

    def on_join_button(self):
        from .JoinDialog import JoinDialog
        dialog = JoinDialog()
        result = dialog.show_modal()
        if result:
            name = dialog.get_game_name()
            IRC.irc.join(name)
            #print(name)
        dialog.destroy()

    def on_start_button(self):
        # check if local or online game here
        net_play = False
        
        if net_play: # net play
            Config.set("x_ready", "1")
            players = []
            if not Netplay.check_config("x_ready", players):
                message = u"The following players are not ready: " + \
                        repr(players)
                Netplay.notice(message)
                return
            # all players were ready
            Netplay.config_version = str(uuid.uuid4())
            message = "__check {0} {1}".format(Netplay.config_version,
                    Config.checksum())
            Netplay.message(message)
        else:
            self.start_local_game()

    def start_local_game(self):
        config = ConfigWriter.create_fsuae_config()
        FSUAE.start_with_config(config)

    def on_destroy(self):
        print("MainWindow.destroy")
        #from .DeviceManager import DeviceManager
        #DeviceManager.stop()
        #self.irc.stop()

    def on_timer(self):
        fsui.call_later(500, self.on_timer)
        #print("timer")
        if Netplay.config_version:
            for player in Netplay.players.values():
                if player.config_version != Netplay.config_version:
                    break
            else:
                # everyone has acked!
                self.create_server()

    def create_server(self):
        pass
