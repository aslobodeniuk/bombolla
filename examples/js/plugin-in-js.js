const GObject = imports.gi.GObject;

const SubclassExample = GObject.registerClass({
    GTypeName: 'SubclassExample',
    Properties: {
        'example-property': GObject.ParamSpec.boolean(
            'example-property',
            'Example Property',
            'A read-write boolean property',
            GObject.ParamFlags.READWRITE,
            true
        ),
    },
    Signals: {
        'example-signal': {},
    },
}, class SubclassExample extends GObject.Object {
    _init(constructProperties = {}) {
        super._init(constructProperties);
    }

    get example_property() {
        if (this._example_property === undefined)
            this._example_property = null;

        return this._example_property;
    }

    set example_property(value) {
        if (this.example_property === value)
            return;

        this._example_property = value;
        this.notify('example-property');
    }
});


log("registered class " + SubclassExample.$gtype.name);
