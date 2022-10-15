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
const GtkClutter = imports.gi.GtkClutter;
const Clutter = imports.gi.Clutter;

// This GObject class written in JS can be created and called
// through bombolla, but currently there is one problem: it has to be called
// asynrously, for example:
// async create SubclassExample a
// async call a.example-signal

GtkClutter.init(null);

const FrankNewsWindow = GObject.registerClass({
    GTypeName: 'FrankNewsWindow',
    Signals: {
        'my-open': {},
	'my-clicked': {}
    },
    Properties: {
        'my-summary': GObject.param_spec_string('my-summary', 'my-summary', 'my-summary', 'something',
            GObject.ParamFlags.READWRITE),
    },
}, class FrankNewsWindow extends Gtk.Window {
    _init() {
        super._init({ title: "Frank news :)" });

	// We embed Clutter into this windonw
	this.embed = new GtkClutter.Embed({
            width_request: 640,
            height_request: 480 });
	this.add(this.embed);

	this.text = new Clutter.Text ({ text : "hello"});
	this.embed.get_stage().add_actor(this.text);
    }

    get my_summary() {
        return this.text.get_text();
    }

    set my_summary(value) {
        this.text.set_text(value) ;
    }

    on_my_open() {
	this.show_all();
    }
});
