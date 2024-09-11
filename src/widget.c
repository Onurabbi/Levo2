#include "widget.h"
#include <stdio.h>

void update_widget(entity_t *e, float delta_time, double elapsed_sec)
{
    widget_t *widget = (widget_t*)e->data;
    switch(widget->type)
    {
        case(WIDGET_TYPE_FPS):
        {
            static char buf[16];
            text_label_t *text_label = widget->text_label;

            if (text_label->timer == 0.0f)
            {
                sprintf(buf, "FPS: %.1lf", 1.0/elapsed_sec);
            }
            text_label->timer += delta_time;
            if (text_label->timer >= text_label->duration)
            {
                text_label->timer = 0.0f;
            }
            text_label->text = buf;
            break;
        }
        default:
        {

            break;
        }
    }
}
