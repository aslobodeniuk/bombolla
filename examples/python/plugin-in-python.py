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

class MyFooClass(GObject.GObject):
    # FIXME: __name__ is set without read understanding about what we're doing
    __name__ = 'myfooclass'

    __gproperties__ = {
        'angryness' : (GObject.TYPE_FLOAT, 'how angry the class is',
            'amount of angryness of the class',
                0, 100, 10, GObject.ParamFlags.READWRITE),
        
        'secret' : (GObject.TYPE_STRING, 'a secret to whisper',
            'some secret', '', GObject.ParamFlags.READWRITE)

    }

    __gsignals__ = {
        'say-hello' : (GObject.SignalFlags.RUN_LAST, GObject.TYPE_NONE,
                            ())
    }
    
    def __init__(self):
        GObject.__init__(self)
        self.angryness = 10
        self.secret = 'I dont really like snakes'
          
    def do_get_property(self, property):
        if property.name == 'angryness':
            return self.angryness
        elif property.name == 'secret':
            return self.secret
        else:
            raise (AttributeError, 'unknown property %s' % property.name)
          
    def do_set_property(self, property, value):
        if property.name == 'angryness':
            self.angryness = value
        elif property.name == 'secret':
            self.secret = value

        else:
            raise (AttributeError, 'unknown property %s' % property.name)
          
    def do_say_hello(self):
        print ('Hello my friend, Im a super MyFooClass.\nIm {} percent angry.\nMy secret is "{}"'.format(self.angryness, self.secret))
      

gtype = GObject.type_register(Car)
# FIXME: Yeah, that's a weak point here,
# and the name... __main__+MyFooClass ??
# Would you call your dog this way ?
print ('new class: %s' % GObject.GType(gtype).name)
