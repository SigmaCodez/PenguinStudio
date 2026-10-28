/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../image-file-watch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static bool entered, released, returned;
static time_t read_timestamp(const char *path)
{
	pthread_mutex_lock(&gate);
	entered = true;
	pthread_cond_broadcast(&changed);
	while (!released)
		pthread_cond_wait(&changed, &gate);
	returned = true;
	pthread_mutex_unlock(&gate);
	return strcmp(path, "current") == 0 ? 42 : -1;
}
static void await_entry(void)
{
	pthread_mutex_lock(&gate);
	while (!entered)
		pthread_cond_wait(&changed, &gate);
	pthread_mutex_unlock(&gate);
}
static void release_query(void)
{
	pthread_mutex_lock(&gate);
	released = true;
	pthread_cond_broadcast(&changed);
	pthread_mutex_unlock(&gate);
}
static void *release_later(void *unused)
{
	(void)unused;
	usleep(20000);
	release_query();
	return NULL;
}
int main(void)
{
	alarm(10);
	struct image_file_watch *watch = image_file_watch_create(read_timestamp);
	assert(watch);
	image_file_watch_request(watch, "current");
	await_entry();
	time_t timestamp = 0;
	/* These calls must return while the simulated slow filesystem is blocked. */
	assert(!image_file_watch_take(watch, "current", &timestamp));
	image_file_watch_request(watch, "ignored while busy");
	release_query();
	while (!image_file_watch_take(watch, "current", &timestamp)) usleep(1000);
	assert(timestamp == 42);
	assert(!image_file_watch_take(watch, "current", &timestamp));
	image_file_watch_request(watch, "missing");
	while (!image_file_watch_take(watch, "missing", &timestamp)) usleep(1000);
	assert(timestamp == -1);
	image_file_watch_request(watch, "old path");
	pthread_mutex_lock(&watch->mutex);
	bool busy = watch->busy;
	pthread_mutex_unlock(&watch->mutex);
	while (busy) {
		usleep(1000);
		pthread_mutex_lock(&watch->mutex);busy = watch->busy;pthread_mutex_unlock(&watch->mutex);
	}
	assert(!image_file_watch_take(watch, "new path", &timestamp));
	image_file_watch_destroy(watch);
	entered = released = returned = false;
	watch = image_file_watch_create(read_timestamp);
	assert(watch);
	image_file_watch_request(watch, "current");
	await_entry();
	pthread_t release;
	assert(pthread_create(&release, NULL, release_later, NULL) == 0);
	image_file_watch_destroy(watch);
	pthread_join(release, NULL);
	assert(returned);
	image_file_watch_destroy(NULL);
	puts("Nonblocking poll, bounded queue, missing/stale paths and in-flight destruction passed");
}
