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

#define _DEFAULT_SOURCE     1

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <netinet/in.h>

#include <pthread.h>

#include "miniproxy.h"
#include "thread_poll.h"

static void * thread_test(void * arg)
{
	unsigned int * array = arg;
	unsigned int number = array[0];
	unsigned int timeout = array[1];
	unsigned int count = array[2];

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	for(;;) {
		printf("run : %d :%d\n", number, count);
		fflush(stdout);
		usleep(timeout);
		--count;
		if (count == 0)
			break;
	}
	pthread_exit(NULL);
}

int main(int argc, char * argv [])
{
	int rc;

	printf("Test thread_poll\n");
	rc = init_thread_poll();
	if (rc != THREAD_SUCCESS)
		printf("Can not init thread poll\n");

	unsigned int array[3]; // 1 sec
	array[0]=1;
	array[1]=1000000;
	array[2]= 3;
	printf("start : %d : %d\n", array[0], array[2]);
	add_thread(thread_test, array);

	array[0]=2;
	array[1]=500000;
	array[2]= 20;
	printf("start : %d : %d\n", array[0], array[2]);
	add_thread(thread_test, array);

	array[0]=3;
	array[1]=1500000;
	array[2]= 10;
	printf("start : %d : %d\n", array[0], array[2]);
	add_thread(thread_test, array);

	sleep(7);

	rc = deinit_thread_poll();
	if (rc != THREAD_SUCCESS)
		printf("Can not deinit thread poll\n");

	printf("Done\n");
	return SUCCESS;
}
