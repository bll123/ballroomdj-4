/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UI_H
#define INC_UI_H

#include "uiclass.h"
#include "uiwcont.h"

#if BDJ4_USE_GTK3
# include "ui-gtk3.h"
#endif
#if BDJ4_USE_NULLUI
# include "ui-null.h"
#endif

#include "ui/uiadjustment.h"
#include "ui/uibox.h"
#include "ui/uibutton.h"
#include "ui/uichgind.h"
#include "ui/uiclipboard.h"
#include "ui/uidialog.h"
#include "ui/uidragdrop.h"
#include "ui/uidropdown.h"
#include "ui/uientry.h"
#include "ui/uiimage.h"
#include "ui/uikeys.h"
#include "ui/uilabel.h"
#include "ui/uilink.h"
#include "ui/uimenu.h"
#include "ui/uimiscbutton.h"
#include "ui/uinotebook.h"
#include "ui/uipbar.h"
#include "ui/uiscale.h"
#include "ui/uiscrollbar.h"
#include "ui/uisep.h"
#include "ui/uisizegrp.h"
#include "ui/uispinbox.h"
#include "ui/uiswitch.h"
#include "ui/uitextbox.h"
#include "ui/uitoggle.h"
#include "ui/uitreeview.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

#endif /* INC_UI_H */
