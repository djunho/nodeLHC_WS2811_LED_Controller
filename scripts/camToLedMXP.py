import socket
import pygame
import pygame.camera
import pygame.transform
from pygame.locals import *
import sys

UDP_IP = "192.168.1.142"
if len(sys.argv) > 1:
    UDP_IP = sys.argv[1]
else:
    print(f"Using default panel IP {UDP_IP}")

UDP_PORT = 2711
SIZE = 8

pygame.init()
pygame.camera.init()

camlist = pygame.camera.list_cameras()
if camlist:
    cam = pygame.camera.Camera(camlist[0],(640,480), "RGB")
    cam.start()
else:
    print("No camera detected")
    sys.exit(1)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def yield_bytes(image):
    for x in range(SIZE):
        for y in range(SIZE):
            color = image.get_at((x, y))
            yield chr(color.r)
            yield chr(color.g)
            yield chr(color.b)

while True:
    image = pygame.transform.smoothscale(cam.get_image(), (SIZE, SIZE))
    message = bytes('\x55\x00\x00\x00\x40' + ''.join(yield_bytes(image)), 'iso-8859-1')
    sock.sendto(message, (UDP_IP, UDP_PORT))

