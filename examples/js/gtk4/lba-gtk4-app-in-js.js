// la Bombolla GObject shell.
// Copyright (C) 2024 Aleksandr Slobodeniuk
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

imports.gi.versions.Gtk = "4.0"
const Gtk = imports.gi.Gtk;

Gtk.init();

let app = new Gtk.Application({ application_id: 'org.gtk.ExampleApp' });

app.connect('activate', () => {
    let win = new Gtk.ApplicationWindow({ application: app });
    let btn = new Gtk.Button({ label: 'Hello, World!' });
    let box = new Gtk.Box()
    btn.connect('clicked', () => { win.close(); });
    box.append(btn);
    win.set_child(box);
    win.present();
});

app.run([]);
