#ifndef INC_UI_UINULL_INT_H
#define INC_UI_UINULL_INT_H

/* the structure must contain a member named widget */
/* and a member named packwidget */
typedef struct uispecific {
  void      *packwidget;
  union {
    void    *widget;
  };
} uispecific_t;

#endif /* INC_UI_UINULL_INT_H */
