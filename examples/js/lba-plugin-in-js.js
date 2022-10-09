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
        'open': {},
	'clicked': {}
    },
}, class MyWindow extends Gtk.Window {
    _init() {
        super._init({ title: "Hello World" });
        this.button = new Gtk.Button({ label: "Click here" });
        this.button.connect("clicked", this.onButtonClicked);
        this.add(this.button);
    }

    onButtonClicked() {
        print("Hello World");
	// this.emit('clicked'); why doesn't it work??
    }

    on_open() {
	this.show_all();
    }
});
