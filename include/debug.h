#ifndef _GTK_SHOT_DEBUG_H_
#define _GTK_SHOT_DEBUG_H_

#ifdef DEBUG
# define debug(fmt, args...) \
            printf("%s[%d]: "fmt, \
                    __func__, __LINE__, ##args)
#else
# define debug(fmt, args...) \
        printf(GTK_SHOT_NAME"-"GTK_SHOT_VERSION": "fmt \
                  , ##args)
#endif

#endif
