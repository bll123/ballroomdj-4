The *-base files have the original alpha text.

===> Any text must be converted using the object->path function
before the icons can be shipped.

Use inkscape to edit bdj4_icon.svg.
run ./mkicon.sh to create the icons.

# macos tahoe forces(!) the icons to be a squircle shape,
# therefore there are macos specific icons
run ./mkmacicon.sh to create the macos specific icons

Any images stored here must use PATHBLD_MP_DIR_IMG / SV_BDJ4_DIR_IMG (absolute)
Any images stored in img/profileNN
    must use PATHBLD_MP_DREL_IMG / SV_BDJ4_DREL_IMG (relative).
