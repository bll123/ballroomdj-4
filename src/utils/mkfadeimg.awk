#!/usr/bin/gawk -f

function points(type) {
  tag = "M";
  range = 700;
  ovol = 700;
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

  print "<svg height=\"700\" width=\"700\" id=\"fades\" viewbox=\"0 0 700 700\">\n";
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
