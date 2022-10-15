# la Bombolla GObject shell.
# Copyright (C) 2022 Aleksandr Slobodeniuk
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

import feedparser

class LbaFeedParser(GObject.GObject):
    __name__ = 'lba_feedparser'

    __gproperties__ = {
        'uri' : (GObject.TYPE_STRING, 'feed uri',
            'uri of the feed to parse',
                 'https://planet.gnome.org/rss20.xml',
                 GObject.ParamFlags.READWRITE),
        
        'entries' : (GObject.TYPE_UINT, 'number of entries',
                     'number of entries in the feed', 0,
                     10000, 0, GObject.ParamFlags.READABLE),

        'entry' : (GObject.TYPE_UINT, 'selected entry',
                     'selected entry in the feed', 0,
                     10000, 0, GObject.ParamFlags.READWRITE),

        'title' : (GObject.TYPE_STRING, 'title',
                   'title', '', GObject.ParamFlags.READABLE),
        
        'published' : (GObject.TYPE_STRING, 'published',
                       'published', '', GObject.ParamFlags.READABLE),

        'summary' : (GObject.TYPE_STRING, 'summary',
                     'summary', '', GObject.ParamFlags.READABLE),

        'link' : (GObject.TYPE_STRING, 'link',
                   'link', '', GObject.ParamFlags.READABLE)
    }

    __gsignals__ = {
        'check-for-updates' : (GObject.SignalFlags.RUN_LAST, GObject.TYPE_NONE,
                            ())
    }
    
    def __init__(self):
        GObject.__init__(self)
        self.uri = "https://planet.gnome.org/rss20.xml"
        self.entry = 0
          
    def do_get_property(self, property):
        print ('getting property %s' % property.name)
        if property.name == 'uri':
            return self.uri
        if property.name == 'entries':
            return self.entries
        if property.name == 'entry':
            return self.entry     
        elif property.name == 'title':
            return self.title
        elif property.name == 'published':
            return self.published
        elif property.name == 'summary':
            return self.summary
        elif property.name == 'link':
            return self.link
        else:
            print ('unknown property %s' % property.name)
            raise (AttributeError, 'unknown property %s' % property.name)
          
    def do_set_property(self, property, value):
        if property.name == 'uri':
            self.uri = value
            self.do_check_for_updates()
        if property.name == 'entry':
            self.entry = value
            if self.uri:
                self.do_check_for_updates()
        else:
            print ('unknown property %s' % property.name)
            raise (AttributeError, 'unknown property %s' % property.name)
          
    def do_check_for_updates(self):
        if self.uri is None:
            print ("no uri")
            return

        self.feed = feedparser.parse(self.uri)
        if len(self.feed.entries) <= self.entry:
            print("wrong entry: %d" % self.entry)
            return
        entry = self.feed.entries[self.entry]
        self.title = entry.title
        self.published = entry.published
        self.summary = entry.summary
        self.link = entry.link

        self.notify("title")
        self.notify("published")
        self.notify("summary")
        self.notify("link")

        print ('Post Title :' + self.title)

# "lba_plugin" is a magic variable, please always set it
lba_plugin = GObject.type_register(LbaFeedParser)
