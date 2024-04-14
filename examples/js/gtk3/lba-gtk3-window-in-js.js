// la Bombolla GObject shell.
// Copyright (C) 2022 Aleksandr Slobodeniuk
//
//   This file is part of bombolla.
//
//   Bombolla is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Bombolla is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.

imports.gi.versions.Gtk = "3.0";

const GObject = imports.gi.GObject;
const Gtk = imports.gi.Gtk;

// This GObject class written in JS can be created and called
// through bombolla, but currently there is one problem: it has to be called
// asynrously, for example:
// async create SubclassExample a
// async call a.example-signal

Gtk.init(null);

const MyWindow = GObject.registerClass({
    GTypeName: 'MyJSWindow',
    Signals: {
        'my-open': {},
	'my-clicked': {}
    },
}, class MyWindow extends Gtk.Window {
    _init() {
        super._init({ title: "Hello World" });
        this.button = new Gtk.Button({ label: "Click here" });
        this.button.connect("clicked", this.onButtonClicked);
        this.add(this.button);
    }

    onButtonClicked (widget) {
        print("Hello World");
	widget.get_parent().emit('my-clicked');
    }

    on_my_open() {
	this.show_all();
    }
});
