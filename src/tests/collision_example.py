import engine
import engine_draw
import engine_input
import engine_physics
from engine_physics import RectangleCollisionShape2D
from engine_math import Vector2
from engine_nodes import Rectangle2DNode, Circle2DNode, CameraNode, Physics2DNode
import random
import math

# Imagine we're targeting a top-down game, turn gravity off on each axis
# engine_physics.set_gravity(0, 0)


box_collision_shape = RectangleCollisionShape2D(width=15, height=15)


box_physics_0 = Physics2DNode(collision_shape=box_collision_shape, position=Vector2(0, -25), rotation=math.pi/4, dynamic=False)
box_0 = Rectangle2DNode(width=15, height=15, outline=True)
box_0.color = engine_draw.blue
box_physics_0.add_child(box_0)
 
class Player(Physics2DNode):
    def __init__(self):
        super().__init__(self)
        self.position = Vector2(0, 0)

        self.bounciness = 1.0

        self.collision_shape = box_collision_shape
        self.player = Rectangle2DNode(width=15, height=15, color=engine_draw.green, outline=True)
        self.add_child(self.player)
    
    def tick(self):
        if engine_input.check_pressed(engine_input.DPAD_LEFT):
            self.velocity.x = -0.85
        elif engine_input.check_pressed(engine_input.DPAD_RIGHT):
            self.velocity.x = 0.85

        if engine_input.check_pressed(engine_input.DPAD_UP):
            self.velocity.y = -0.85
        elif engine_input.check_pressed(engine_input.DPAD_DOWN):
            self.velocity.y = 0.85
    
    def collision(self, contact):
        print("Collision!")
        Circle2DNode(position=contact.position, radius=2)



player = Player()


# box_physics_1 = Physics2DNode(collision_shape=box_collision_shape, position=Vector2(0, 25))
# box_1 = Rectangle2DNode(width=15, height=15)
# box_1.color = engine_draw.blue
# box_physics_1.add_child(box_1)


camera = CameraNode()

engine.start()
