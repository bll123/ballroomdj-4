/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiclass.h"
#include "uiwcont.h"

#if BDJ4_UI_GTK3
# include "ui-gtk3.h"
#endif
#if BDJ4_UI_NULL
# include "ui-null.h"
#endif
#if BDJ4_UI_MACOS
# include "ui-macos.h"
#endif
#if BDJ4_UI_NCURSES
# include "ui-ncurses.h"
#endif

#include "ui/uibox.h"
#include "ui/uibutton.h"
#include "ui/uichgind.h"
#include "ui/uiclipboard.h"
#include "ui/uidialog.h"
#include "ui/uidragdrop.h"
#include "ui/uientry.h"
#include "ui/uievents.h"
#include "ui/uiimage.h"
#include "ui/uilabel.h"
#include "ui/uilink.h"
#include "ui/uimenu.h"
#include "ui/uimiscbutton.h"
#include "ui/uinotebook.h"
#include "ui/uipanedwindow.h"
#include "ui/uipbar.h"
#include "ui/uiscale.h"
#include "ui/uiscrollbar.h"
#include "ui/uisep.h"
#include "ui/uisizegrp.h"
#include "ui/uispinbox.h"
#include "ui/uiswitch.h"
#include "ui/uitextbox.h"
#include "ui/uitoggle.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"
#include "uigeneral.h"

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

