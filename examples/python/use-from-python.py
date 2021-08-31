# la Bombolla GObject shell.
# Copyright (C) 2021 Aleksandr Slobodeniuk
#
#   This file is part of bombolla.
#
#   Bombolla is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Bombolla is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.
#

from gi.repository import GObject
import ctypes
import importlib
import os

exlibpath = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), '..', 'build', 'lib')
b = ctypes.CDLL(os.path.join(exlibpath, 'libbombolla-core.so.0'))

b.lba_core_get_type()

bombolla = GObject.new ("LbaCore")
bombolla.set_property ("plugins_path", exlibpath)

bombolla.emit ("execute",
"create LbaCoglWindow w\n\
create LbaCoglLabel l\n\
set l.drawing-scene w\n\
set l.x 0.1\n\
set l.y 0.1\n\
set l.text Hello from bombolla\n\
set l.font-size 40\n\
set l.font-name Chilanka\n\
call w.open\n\
")

while True:
    next_command = input("Enter next command: ")
    bombolla.emit ("execute", next_command)
