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

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "thread_poll.h"

#include <stdio.h>

#define THREAD_SIZE      8
static pthread_t * thread_begin_list = NULL;
static pthread_t * thread_end_list = NULL;
static int thread_list_size = THREAD_SIZE;
static pthread_t * thread_current = NULL;

static int check_thread_poll(void)
{
	int rc;
	pthread_t * temp = thread_begin_list;
	for (;temp != thread_current;) {
		rc = pthread_kill(*temp, 0);
		if (rc == 0)
			++temp;
		else {
			--thread_current;
			if (temp == thread_current)
				break;
			else {
				*temp = *(thread_current);
				++temp;
			}
		}
	}
	return THREAD_SUCCESS;
}

static int add_thread_poll(void)
{
	pthread_t * temp;
	int size = thread_list_size << 1;
	int len_list = thread_current - thread_begin_list;

	temp = malloc(size * sizeof(pthread_t));
	memmove(temp, thread_begin_list, (thread_list_size*sizeof(pthread_t)));
	free(thread_begin_list);

	thread_list_size = size;
	thread_begin_list = temp;
	thread_end_list = thread_begin_list + thread_list_size;
	thread_current = thread_begin_list + len_list;
	return THREAD_SUCCESS;
}

int init_thread_poll(void)
{
	if (thread_begin_list != NULL) {
		return THREAD_ERROR_REPEAT_INIT;
	}

	thread_begin_list = malloc(thread_list_size * sizeof(pthread_t));
	thread_end_list = thread_begin_list + thread_list_size;
	thread_current = thread_begin_list;

	return THREAD_SUCCESS;
}

int deinit_thread_poll(void)
{
	int rc;
	pthread_t * temp = thread_begin_list;
	if (temp == NULL)
		return THREAD_SUCCESS;

	check_thread_poll();

	for (;temp != thread_current;) {
		rc = pthread_cancel(*temp);
		if (rc == 0)
			pthread_join(*temp, NULL);
		++temp;
	}

	free(thread_begin_list);
	thread_begin_list = NULL;
	return THREAD_SUCCESS;
}

int add_thread(void *(*routen)(void*), void * arg)
{
	int rc;
	pthread_attr_t attr;

	check_thread_poll();

	if (thread_current == thread_end_list) {
		rc = add_thread_poll();
		if (rc != THREAD_SUCCESS)
			return rc;
	}

	pthread_attr_init(&attr);
	rc = pthread_create(thread_current, &attr, routen, arg);
	if (rc != 0) {
		return THREAD_ERROR_CREAT;
	}

	++thread_current;
	return THREAD_SUCCESS;
}
