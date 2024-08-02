import os
from system.root_dir import ROOT_DIR
from system.util import is_file, is_dir, basename, thumby_reset
from system.run_on_boot import set_run_on_boot
from system.launcher.direction_icon import DirectionIcon

from engine_nodes import EmptyNode, Rectangle2DNode, Text2DNode, GUIBitmapButton2DNode, Sprite2DNode
from engine_draw import Color
import engine_draw
import engine_io
from engine_math import Vector2
from engine_resources import TextureResource
from engine_animation import Tween, ONE_SHOT, EASE_BACK_OUT, EASE_SINE_OUT

import math


title_font = None                                                               # Font from main launcher.py file
category_background_color = Color(0.157, 0.137, 0.263)                          # Background color for category rows
q_mark_icon = TextureResource("system/launcher/assets/launcher-tile-qmark.bmp") # Default icon for games that don't have one
arrow_direction_icon = TextureResource("system/launcher/assets/arrow.bmp")      # Direction icon for category rows


# Given contents of a directory, check if the contents
# signify that the directory is a game directory and
# return GameInfo
def check_and_get_game_info(directory_path, directory_contents):
    # Game directories need to contain of the following
    if "manifest.ini" not in directory_contents and "main.py" not in directory_contents:
        return False
    
    # This is a game, create default info object
    game_info = GameInfo(directory_path)

    # Mark as legacy by default if inside path that
    # contains keyword `Legacy` (usually in /Games/Legacy)
    game_info.legacy = "Legacy" in directory_path
    
    # Complete `game_info` as fully as possible if `manifest.ini` exists
    if "manifest.ini" in directory_contents:
        # Open, read, and close manifest file
        manifest_file = open(f"{directory_path}/manifest.ini")
        manifest_lines = manifest_file.readlines()
        manifest_file.close()

        # Parse each line of the manifest
        for line in manifest_lines:
            if   "name"    in line: game_info.name      = line[line.find("=")+1:].strip()
            elif "main"    in line: game_info.main_path = f"{directory_path}/{line[line.find("=")+1:].strip()}"
            elif "icon"    in line: game_info.icon_path = f"{directory_path}/{line[line.find("=")+1:].strip()}"
            elif "legacy"  in line:
                extracted = line[line.find("=")+1:].strip()
                if "True" in extracted or "true" in extracted or "1" in extracted:
                    game_info.legacy = True
    
    return game_info


# Finds all GameInfos on device
def find_all_games(game_infos, directory_path):
    directory_contents = os.listdir(directory_path)

    # Stop searching this file tree branch if this is game,
    # add to game info list first
    info_result = check_and_get_game_info(directory_path, directory_contents)
    if info_result != False:
        game_infos.append(info_result)
        return
    
    # If not in a game directory, search all folders in this folder
    for element in directory_contents:
        path = f"{directory_path}/{element}"
        if is_dir(path):
            find_all_games(game_infos, path)


# Represents all the information needed for displaying
# a game in the launcher and playing it
class GameInfo():
    def __init__(self, directory):
        self.directory = directory              # Full path to directory containing the game (ex: `/Games/SpecialGames/AGame`)
        self.name = basename(directory)         # Assume no name given for game in manifest by default, use directory for name
        self.main_path = f"{directory}/main.py" # Assume no main file given in manifest by default, use default `main.py`
        self.icon_path = None                   # Assume no icon given from manifest
        self.legacy = False                     # Assume not a legacy original Thumby game
    
    def __repr__(self):
        return f"{self.directory} {{main={self.main_path}, icon={self.icon_path}}}"


# Tile in category row that uses GUI engine callbacks to launch games
class GameLauncherTile(GUIBitmapButton2DNode):
    def __init__(self, game_info, focused, on_tile_focused_cb, on_tiles_moved_cb):
        super().__init__(self)
        self.on_tile_focused_cb = on_tile_focused_cb
        self.on_tiles_moved_cb = on_tiles_moved_cb

        self.focused = focused
        self.initial_focused = focused
        self.game_info = game_info
        self.font = title_font
        self.text = ""
        self.scale.x = 0.6
        self.scale.y = 0.6

        # Load the default icon for the custom one, if it exists
        if game_info.icon_path == None:
            self.bitmap = q_mark_icon
            self.transparent_color = engine_draw.white
        else:
            self.bitmap = TextureResource(game_info.icon_path)

        # Create title for this tile
        self.title_text_node = Text2DNode(text=game_info.name, font=title_font, position=Vector2(0, 26), opacity=1.0, letter_spacing=1.0)
        self.add_child(self.title_text_node)

        # Setup tween for this tile for when rows are horizontally shifted
        self.tween = Tween()
        self.tween.after = self.on_tiles_moved_cb
    
    def tick(self, dt):
        if self.position.x < -64 or self.position.x > 64:
            self.opacity = 0.0
            self.title_text_node.opacity = 0.0
        else:
            self.opacity = 1.0
            self.title_text_node.opacity = 1.0
    
    def goto_x(self, x):
        self.tween.start(self.position, "x", self.position.x, x, 100, 1.0, ONE_SHOT, EASE_SINE_OUT)
        
    def on_just_focused(self):
        self.on_tile_focused_cb(self)
    
    def on_just_pressed(self):
        # Launch the game if this tile is pressed
        set_run_on_boot(self.game_info.main_path)
        thumby_reset(False)


# Represents a category of games. Games can be organized
# inside `/Games` into folders like `/Games/SpecialGames`
# which will result in a categories `Games` and `SpecialGames`
# on the launcher screen
class GameCategory(EmptyNode):
    def __init__(self, shift_categories):
        super().__init__(self)
        self.shift_categories = shift_categories
        self.name = None
        self.game_infos = []
        self.tiles = []

        # Setup row left/right direction icons
        self.right_direction_icon = DirectionIcon(Sprite2DNode(texture=arrow_direction_icon, rotation=-math.pi/2, transparent_color=engine_draw.white, opacity=0.8, layer=4), engine_io.RIGHT, Vector2(54, 0))
        self.left_direction_icon = DirectionIcon(Sprite2DNode(texture=arrow_direction_icon, rotation=math.pi/2, transparent_color=engine_draw.white, opacity=0.8, layer=4), engine_io.LEFT, Vector2(-54, 0))
        self.add_child(self.right_direction_icon)
        self.add_child(self.left_direction_icon)

        # Setup row background
        self.background_rect = Rectangle2DNode(width=128, height=58, color=category_background_color, opacity=0.35)
        self.background_rect.position.y = 2

        # Setup text and text background
        self.title_text = Text2DNode(font=title_font, layer=2)
        self.title_text.position.y = -33
        self.title_background_rect = Rectangle2DNode(color=category_background_color, opacity=0.35)
        self.title_background_rect.position.y = 1
        self.title_text.add_child(self.title_background_rect)

        # Add row bg and text as children to this category node
        self.add_child(self.background_rect)
        self.add_child(self.title_text)

        self.tween = Tween()
        self.tween.after = self.after_tween_cb
    
    def goto_y(self, y):
        self.tween.start(self.position, "y", self.position.y, y, 100, 1.0, ONE_SHOT, EASE_SINE_OUT)
    
    def update_name(self, name):
        self.name = name
        self.title_text.text = name
        self.title_text.position.x = (-self.background_rect.width/2 + self.title_text.width/2) + 1

        self.title_background_rect.width = self.title_text.width+2
        self.title_background_rect.height = self.title_text.height+2
    
    # Custom callback that's passed down to tiles to
    # be called when they get focused (shifts all tiles)
    def on_tile_focused_cb(self, new_focused_tile):
        new_tile_position = new_focused_tile.position
        new_tile_global_position = new_focused_tile.global_position

        if new_tile_position.x > 0:
            engine_io.gui_focused(False)    # Turn OFF GUI layer focus so you can't interrupt the tween
            for tile in self.tiles:
                tile.goto_x(tile.position.x-80)
        elif new_tile_position.x < 0:
            engine_io.gui_focused(False)    # Turn OFF GUI layer focus so you can't interrupt the tween
            for tile in self.tiles:
                tile.goto_x(tile.position.x+80)
        elif new_tile_global_position.y > 0:
            engine_io.gui_focused(False)    # Turn OFF GUI layer focus so you can't interrupt the tween
            self.shift_categories(-80)
        elif new_tile_global_position.y < 0:
            engine_io.gui_focused(False)    # Turn OFF GUI layer focus so you can't interrupt the tween
            self.shift_categories(80)
    
    # Custom callback that's passed down to tiles to
    # be called after they all get shifted (tweened)
    # or after camera y changes
    def after_tween_cb(self, tween):
        engine_io.gui_focused(True)         # Turn ON GUI layer focus after tweens

    def create_tiles(self):
        pos_x = 0
        for info in self.game_infos:
            tile = GameLauncherTile(info, False, self.on_tile_focused_cb, self.after_tween_cb)
            self.add_child(tile)
            tile.position.x = pos_x
            self.tiles.append(tile)
            pos_x += 80
        
        # Don't show left/right arrows if only one game in category/row
        if(len(self.game_infos) == 1):
            self.right_direction_icon.opacity = 0.0
            self.left_direction_icon.opacity = 0.0


    def __repr__(self):
        return f"name={self.name} [len={len(self.game_infos)}]"
    

class GamesScreen():
    def __init__(self, font):
        global title_font
        title_font = font
        self.categories = []
        self._get_categories_and_fill()

    def shift_categories(self, offset):
        for category in self.categories:
            category.goto_y(category.position.y+offset)
    
    def _get_categories_and_fill(self):
        game_infos = []
        find_all_games(game_infos, "Games")
        
        for info in game_infos:
            # Extract the game `category` which is the folder
            # containing the game folder (i.e. `Games`)
            category_end = info.directory.rfind("/")
            category_start = info.directory.rfind("/", 0, category_end)+1
            game_category_name = info.directory[category_start:category_end]
            
            # Check to see if this game belongs to any
            # of the already collected categories
            found = False
            for category in self.categories:
                if game_category_name == category.name:
                    category.game_infos.append(info)
                    found = True
                    break
            
            # It did not belong, create a new category and add
            # the game info to it
            if found == False:
                category = GameCategory(self.shift_categories)
                category.update_name(game_category_name)
                category.game_infos.append(info)

                category.position.y += len(self.categories) * (80)
                self.categories.append(category)
        
        # Have each category spawn its tiles
        for category in self.categories:
            category.create_tiles()
    
    # Do not want other page UI navigation inputs
    # to switch to this page when not focused,
    # disable all tile launcher UIs if not
    def tell_page(self, page):
        are_tiles_disabled = False

        if page == 0:
            are_tiles_disabled = False
        else:
            are_tiles_disabled = True
        
        for category in self.categories:
            for tile in category.tiles:
                tile.disabled = are_tiles_disabled