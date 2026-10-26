/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <util/bmem.h>

/* File-system latency must never block the video tick. One pending query per
 * source; the worker only owns a copied path, never a source or graphics object. */
struct image_file_watch {
	pthread_mutex_t mutex;
	pthread_cond_t changed;
	pthread_t thread;
	bool busy, ready, stopping;
	char *path;
	time_t timestamp;
	time_t (*read_timestamp)(const char *);
};

static void *image_file_watch_worker(void *data)
{
	struct image_file_watch *watch = data;
	pthread_mutex_lock(&watch->mutex);
	for (;;) {
		while (!watch->busy && !watch->stopping)
			pthread_cond_wait(&watch->changed, &watch->mutex);
		if (watch->stopping)
			break;
		const char *path = watch->path;
		pthread_mutex_unlock(&watch->mutex);
		time_t timestamp = watch->read_timestamp(path);
		pthread_mutex_lock(&watch->mutex);
		watch->timestamp = timestamp;
		watch->ready = true;
		watch->busy = false;
	}
	pthread_mutex_unlock(&watch->mutex);
	return NULL;
}

static struct image_file_watch *image_file_watch_create(time_t (*read_timestamp)(const char *))
{
	struct image_file_watch *watch = bzalloc(sizeof(*watch));
	watch->read_timestamp = read_timestamp;
	if (pthread_mutex_init(&watch->mutex, NULL) != 0) {
		bfree(watch);
		return NULL;
	}
	if (pthread_cond_init(&watch->changed, NULL) != 0) {
		pthread_mutex_destroy(&watch->mutex);
		bfree(watch);
		return NULL;
	}
	if (pthread_create(&watch->thread, NULL, image_file_watch_worker, watch) != 0) {
		pthread_cond_destroy(&watch->changed);
		pthread_mutex_destroy(&watch->mutex);
		bfree(watch);
		return NULL;
	}
	return watch;
}

static void image_file_watch_request(struct image_file_watch *watch, const char *path)
{
	if (!watch || !path || !*path)
		return;
	pthread_mutex_lock(&watch->mutex);
	if (!watch->busy) {
		bfree(watch->path);
		watch->path = bstrdup(path);
		watch->ready = false;
		watch->busy = true;
		pthread_cond_signal(&watch->changed);
	}
	pthread_mutex_unlock(&watch->mutex);
}

static bool image_file_watch_take(struct image_file_watch *watch, const char *path, time_t *timestamp)
{
	if (!watch)
		return false;
	pthread_mutex_lock(&watch->mutex);
	bool matches = watch->ready && path && strcmp(path, watch->path) == 0;
	if (matches)
		*timestamp = watch->timestamp;
	watch->ready = false;
	pthread_mutex_unlock(&watch->mutex);
	return matches;
}

static void image_file_watch_destroy(struct image_file_watch *watch)
{
	if (!watch)
		return;
	pthread_mutex_lock(&watch->mutex);
	watch->stopping = true;
	pthread_cond_signal(&watch->changed);
	pthread_mutex_unlock(&watch->mutex);
	/* Join before module unload. No graphics lock is held by the worker. */
	pthread_join(watch->thread, NULL);
	pthread_cond_destroy(&watch->changed);
	pthread_mutex_destroy(&watch->mutex);
	bfree(watch->path);
	bfree(watch);
}
