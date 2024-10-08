import engine_main
import engine_draw
import engine
import math

from engine_animation import Tween, ONE_SHOT, EASE_ELAST_IN_OUT, EASE_SINE_IN_OUT
from engine_nodes import Rectangle2DNode, CameraNode
from engine_math import Vector2, Vector3

cam = CameraNode()
engine.fps_limit(60)

class MyRect(Rectangle2DNode):
    def __init__(self):
        super().__init__(self)
        self.tween = Tween()
        self.tween2 = Tween()

r0 = MyRect()
r0.position.x = -10
r0.position.y = 0
r0.rotation = 0.0

r1 = MyRect()
r1.position.x = 10
r1.position.y = 0
r1.rotation = 0.0

print(str(engine_draw.Color(0.3, 0, 1).value))
r1.tween.start(r1, "rotation", 0.0, 2*math.pi, 3000, 1.0, ONE_SHOT, EASE_ELAST_IN_OUT)
r1.tween2.start(r1, "color", engine_draw.red, 0x481F, 3000, 1.0, ONE_SHOT, EASE_SINE_IN_OUT)
# r1.tween.start(r1, "position", r1.position, Vector2(30, 30), 1000, 1.0, ONE_SHOT, EASE_ELAST_IN_OUT)


engine.start()
