/*****************************************************************************/
/*                                                                           */
/* miniproxy : mini proxy server                                             */
/*                                                                           */
/*  Copyright (C) 2021 Kuzmin Yaroslav <kuzmin.yaroslav@gmail.com>           */
/*                                                                           */
/* miniproxy is free software: you can redistribute it and/or modify it      */
/* under the terms of the GNU General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or             */
/* (at your option) any later version.                                       */
/*                                                                           */
/* miniproxy is distributed in the hope that it will be useful, but          */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                      */
/* See the GNU General Public License for more details.                      */
/*                                                                           */
/* You should have received a copy of the GNU General Public License along   */
/* with this program.  If not, see <http://www.gnu.org/licenses/>.           */
/*                                                                           */
/*****************************************************************************/


#ifndef THREAD_POLL_H
#define THREAD_POLL_H

#define THREAD_SUCCESS               0
#define THREAD_ERROR_REPEAT_INIT    -1
#define THREAD_ERROR_CREAT          -2

int init_thread_poll(void);
int deinit_thread_poll(void);
int add_thread(void *(*routen)(void*), void * arg);

#endif
