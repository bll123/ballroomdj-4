import std.c.stdio;
import std.c.stdlib;

import tcl.tcl;
import tk.tk;

int main(char[][] args)
{
  Tcl_Interp *tcl_interp;
  Tk_Window mainWindow;

  tcl_interp = Tcl_CreateInterp();
  if(Tcl_Init(tcl_interp) != TCL_OK || Tk_Init(tcl_interp) != TCL_OK) {
    if(*tcl_interp.result) {
      fprintf(stderr,"%s: Tcl/Tk_Init: %s\n", args[0] ~ "\0", tcl_interp.result);
    }
    exit(1);
  }

  mainWindow = Tk_MainWindow(tcl_interp);
  if (mainWindow == null) {
    fprintf(stderr, "%s: Tk_MainWindow: %s\n", args[0] ~ "\0", tcl_interp.result);
    exit(1);
  }

  Tcl_Eval(tcl_interp, "label .w -text \"Hello World\"");
  Tcl_Eval(tcl_interp, "pack .w");

  Tk_MainLoop();

  return 0;
}
