import sys
import cv2 as cv
from casadi import *
from numpy import random

opti = casadi.Opti()

x = opti.variable()
y = opti.variable()

opti.minimize(  cos(x) + sin(x)   )
opti.subject_to( x>=1 )
opti.subject_to( x<=5 )

opti.subject_to( y>=0 )
opti.subject_to( y<=4 )

opti.solver('ipopt')

sol = opti.solve()

print(sol.value(x))
print(sol.value(y))

print(cos(sol.value(x)) + sin(sol.value(x)))

A  = SX.eye(4)
#A = mtimes(0.1,A)
rand_error = random.rand( 1,  4)
quadratic_form = mtimes(rand_error,mtimes(A,rand_error.transpose()))

print(A)
print(rand_error)
print(quadratic_form)
print(rand_error.transpose())

