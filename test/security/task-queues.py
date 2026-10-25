#!/usr/bin/env python3
"""Compile the actual task dispatch functions with a minimal queue context."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
def function(file, name):
    text = (root / file).read_text()
    start = text.index('static ', text.index(name) - 25)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]
source = r'''
#include <assert.h>
#include <pthread.h>
#include <util/deque.h>
struct obs_task_info { void (*task)(void *); void *param; };
struct obs_core_audio { pthread_mutex_t task_mutex; struct deque tasks; };
struct obs_core_video { pthread_mutex_t task_mutex; struct deque tasks; };
static struct core { struct obs_core_audio audio; struct obs_core_video video; } core;
static struct core *obs = &core;
'''
source += function('libobs/obs-audio.c', 'execute_audio_tasks')
source += function('libobs/obs-video.c', 'execute_graphics_tasks')
source += r'''
struct context { pthread_mutex_t *mutex; struct deque *tasks; int calls; };
static void child(void *param) { ((struct context *)param)->calls++; }
static void callback(void *param) {
    struct context *ctx = param;
    /* A task must be able to enqueue another task without deadlocking. */
    assert(pthread_mutex_trylock(ctx->mutex) == 0);
    struct obs_task_info next = {child, ctx};
    deque_push_back(ctx->tasks, &next, sizeof(next));
    pthread_mutex_unlock(ctx->mutex);
    ctx->calls++;
}
int main(void) {
    pthread_mutex_init(&obs->audio.task_mutex, NULL);
    pthread_mutex_init(&obs->video.task_mutex, NULL);
    struct context audio = {&obs->audio.task_mutex, &obs->audio.tasks, 0};
    struct context video = {&obs->video.task_mutex, &obs->video.tasks, 0};
    struct obs_task_info a = {callback, &audio}, v = {callback, &video};
    deque_push_back(audio.tasks, &a, sizeof(a));
    deque_push_back(video.tasks, &v, sizeof(v));
    execute_audio_tasks(); execute_graphics_tasks();
    assert(audio.calls == 2 && video.calls == 2);
    assert(!audio.tasks->size && !video.tasks->size);
    deque_free(audio.tasks); deque_free(video.tasks);
    pthread_mutex_destroy(audio.mutex); pthread_mutex_destroy(video.mutex);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path/'queues.c').write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I'+str(root/'libobs'),
                    str(path/'queues.c'), '-lobs', '-pthread', '-o', str(path/'queues')], check=True)
    subprocess.run([str(path/'queues')], check=True, timeout=5)
print('PASS: audio and graphics callbacks can enqueue nested tasks without holding the queue mutex')
