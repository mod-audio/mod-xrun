#include <stdbool.h>
#include <stdio.h>

#include <signal.h>
#include <unistd.h>

#include <jack/jack.h>

static volatile bool           _jack_shutdown = false;
static volatile int            _jack_xruns    = 0;
static volatile jack_client_t* _jack_client   = NULL;
static          uint32_t       _buffer_size   = 0;

static void signal_handler(int frame)
{
    _jack_shutdown = true;
    return;

    // unused
    (void)frame;
}

static void shutdown_callback(void* arg)
{
    _jack_client = NULL;
    _jack_shutdown = true;
    return;

    // unused
    (void)arg;
}

static int bufsize_callback(uint32_t bufsize, void* arg)
{
    _buffer_size = bufsize;
    return 0;

    // unused
    (void)arg;
}

static int xrun_callback(void* arg)
{
    ++_jack_xruns;
    return 0;

    // unused
    (void)arg;
}

int main(void)
{
    jack_client_t* const client = jack_client_open("mod-xrun", JackNoStartServer, NULL);

    if (client == NULL)
    {
        printf("Failed to open jack client\n");
        return 1;
    }

    struct sigaction sterm;
    sterm.sa_handler  = signal_handler;
    sterm.sa_flags    = SA_RESTART;
    sterm.sa_restorer = NULL;
    sigemptyset(&sterm.sa_mask);
    sigaction(SIGINT,  &sterm, NULL);
    sigaction(SIGTERM, &sterm, NULL);

    jack_on_shutdown(client, shutdown_callback, NULL);
    jack_set_buffer_size_callback(client, bufsize_callback, NULL);
    jack_set_xrun_callback(client, xrun_callback, NULL);

    _buffer_size = jack_get_buffer_size(client);
    _jack_client = client;

    if (_buffer_size == 0)
    {
        printf("JACK failed to report buffer size\n");
        jack_client_close(client);
        return 1;
    }

    if (jack_activate(client) != 0)
    {
        printf("Failed to activate jack client\n");
        jack_client_close(client);
        return 1;
    }

    printf("mod-xrun started, listening for xruns...\n");

    int xrun_count, last_xruns = 0;

    while (! _jack_shutdown)
    {
        xrun_count = _jack_xruns - last_xruns;

        if (xrun_count != 0)
        {
            jack_set_buffer_size(client, _buffer_size);
            last_xruns = _jack_xruns;
            printf("xrun! %i of %i total\n", xrun_count, last_xruns);
        }

        usleep(5000);
    }

    if (_jack_client != NULL)
    {
        _jack_client = NULL;
        jack_deactivate(client);
        jack_client_close(client);
    }

    return 0;
}
