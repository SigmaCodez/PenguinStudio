#!/usr/bin/env python3
"""Exercise the actual ALSA teardown functions without an audio device."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
text = (root/'plugins/linux-alsa/alsa-input.c').read_text()
def function(name):
    match = re.search(r'void '+name+r'\([^;]+?\)\n\{', text)
    start = match.start()
    end = match.end()
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]
source = r'''
#include <assert.h>
#include <pthread.h>
#include <util/bmem.h>
#include <util/threading.h>
struct alsa_data {
    volatile bool shutdown, reopen;
    pthread_t reopen_thread;
    os_event_t *abort_event;
    void *handle;
    char *device;
};
static bool joined, closed;
static void _alsa_stop_reopen(struct alsa_data *);
static void _alsa_close(struct alsa_data *data) {
    assert(os_atomic_load_bool(&data->shutdown));
    assert(!data->reopen_thread && joined);
    closed = true;
}
'''
source += function('alsa_destroy')
source += function('_alsa_stop_reopen')
source += r'''
static void *worker(void *param) {
    struct alsa_data *data = param;
    /* Deliberately leave 'reopen' false, reproducing the startup race. */
    os_event_wait(data->abort_event);
    assert(os_atomic_load_bool(&data->shutdown));
    joined = true;
    return NULL;
}
int main(void) {
    struct alsa_data *data = bzalloc(sizeof(*data));
    assert(os_event_init(&data->abort_event, OS_EVENT_TYPE_MANUAL) == 0);
    assert(pthread_create(&data->reopen_thread, NULL, worker, data) == 0);
    /* No open PCM handle: the reconnect worker must still be joined. */
    alsa_destroy(data);
    assert(joined && closed);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path/'alsa.c').write_text(source)
    subprocess.run(['cc', '-std=c11', '-D_POSIX_C_SOURCE=200809L', '-Wall', '-Wextra', '-Werror', '-I'+str(root/'libobs'),
                    str(path/'alsa.c'), '-lobs', '-pthread', '-o', str(path/'alsa')], check=True)
    subprocess.run([str(path/'alsa')], check=True, timeout=5)
print('PASS: ALSA destroy joins a reconnect worker even before its running flag is set')
