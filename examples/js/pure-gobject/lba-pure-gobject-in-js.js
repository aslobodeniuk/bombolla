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

const GObject = imports.gi.GObject;

const MyCustomObject = GObject.registerClass({
    GTypeName: 'MyCustomObject',
    Properties: {
        'something': GObject.ParamSpec.int(
            'something',
            'Something',
            'A read-write integer property',
            GObject.ParamFlags.READWRITE,
	    0, 1234,
            123
        ),
    },
    Signals: {
        'hello': {},
    },
}, class MyCustomObject extends GObject.Object {
    constructor(constructProperties = {}) {
        super(constructProperties);
    }

    get something() {
        if (this._something === undefined)
            this._something = null;

        return this._something;
    }

    set something(value) {
        if (this.something === value)
            return;

        this._something = value;
        this.notify('something');
    }

    on_hello() {
	print("Hello from JS object! The property value is " + this._something);
    }

    _init() {
        super._init();
	this._something = 123;
    }
});
