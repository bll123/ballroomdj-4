#!/usr/bin/gawk -f

function points(type) {
  tag = "M";
  range = 256;
  ovol = 256;
  for (i = 0; i < range; ++i) {
    idx = range - i;
    fidx = idx / range;
    if (fidx < 0.0) { fidx = 0.0; }
    if (fidx > 1.0) { fidx = 1.0; }
    switch (type) {
      case "esin" : fidx = 1.0 - cos (M_PI / 4.0 * ((2.0 * fidx - 1.0 ^ 3.0) + 1.0));
      case "hsin" : fidx = (1.0 - cos (fidx * M_PI)) / 2.0;
      case "ipar" : fidx = (1.0 - (1.0 - fidx) * (1.0 - fidx));
      case "qsin" : fidx = sin (fidx * M_PI / 2.0);
      case "qua" : fidx = fidx * fidx;
      case "tri" : ;
    }
    vol = ovol * fidx;
    y = ovol-vol;
    print tag " " i " " y " ";
    tag = "L";
  }
}

BEGIN {
  M_PI = 3.141592653589793;

  print "<svg height=\"256\" width=\"256\" id=\"fades\" viewbox=\"0 0 256 256\">\n";
  tarr ["esin"] = "cyan";
  tarr ["ipar"] = "black";
  tarr ["hsin"] = "green";
  tarr ["qsin"] = "red";
  tarr ["qua"] = "magenta";
  tarr ["tri"] = "blue";
  for (t in tarr) {
    col = tarr [t];
    print "<path id=\"" t "\" stroke=\"" col "\" stroke-width=\"5\" fill=\"none\"\n";
    print "  d=\" ";
    points(t);
    print "\" />\n";
  }
  print "</svg>\n";
}
