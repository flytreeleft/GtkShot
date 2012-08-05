/*
 * GtkShot - A screen capture programme using GtkLib
 * Copyright (C) 2012 flytreeleft @ CrazyDan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _GTK_SHOT_DEBUG_H_
#define _GTK_SHOT_DEBUG_H_

#ifdef GTK_SHOT_DEBUG
# define debug(fmt, args...) \
            printf("%s[%d]: "fmt, \
                    __func__, __LINE__, ##args)
#else
# define debug(fmt, args...) \
        printf(GTK_SHOT_NAME"-"GTK_SHOT_VERSION": "fmt \
                  , ##args)
#endif

#endif
