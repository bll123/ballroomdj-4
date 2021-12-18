#include <stdio.h>
#include <stdlib.h>

#include <tcl.h>
#include <tk.h>

int main(int argc, char *argv[])
{
  Tcl_Interp *tcl_interp;
  Tk_Window mainWindow;
  
  tcl_interp = Tcl_CreateInterp();
  if(Tcl_Init(tcl_interp) != TCL_OK || Tk_Init(tcl_interp) != TCL_OK) {
    if(*tcl_interp->result)
      fprintf(stderr,"%s: %s\n", argv[0], tcl_interp->result);
    exit(1);
  }

  mainWindow = Tk_MainWindow(tcl_interp);
  if (mainWindow == NULL) {
    fprintf(stderr, "%s\n", tcl_interp->result);
    exit(1);
  }
  
  Tcl_Eval(tcl_interp, "label .w -text \"Hello World\"");
  Tcl_Eval(tcl_interp, "pack .w");

  Tk_MainLoop();

  return 0;
}
