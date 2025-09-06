#include "sntp_daemon.h"
#include "sntp.h"
#include "exceptions/timer.h"
#include "process/scheduler.h"
#include "net/internet_layer/ipv4.h"
#include "process/scheduler.h"

extern void sleep(uint64_t ms);

static uint16_t g_pid_sntp = 0xFFFF;
static socket_handle_t g_sock = 0;

uint16_t sntp_get_pid(void){ return g_pid_sntp; }
bool sntp_is_running(void){ return g_pid_sntp != 0xFFFF; }
void sntp_set_pid(uint16_t p){ g_pid_sntp = p; }
socket_handle_t sntp_socket_handle(void){ return g_sock; }

#define SNTP_POLL_INTERVAL_MS (10u * 60u * 1000u)
#define SNTP_QUERY_TIMEOUT_MS 1200u
#define SNTP_BOOTSTRAP_MAX_RETRY 5u

int sntp_daemon_entry(int argc, char* argv[]){
    (void)argc; (void)argv;
    g_pid_sntp = (uint16_t)get_current_proc_pid();
    g_sock = udp_socket_create(0, g_pid_sntp);
    sntp_set_pid(get_current_proc_pid());
    uint32_t attempts = 0;
    while (attempts < SNTP_BOOTSTRAP_MAX_RETRY){
        const net_cfg_t* cfg = ipv4_get_cfg();
        if (!cfg || cfg->mode == -1 || cfg->ip == 0){ //NET_MODE_DISABLED
            sleep(500);
            continue;
        }

        sntp_result_t r = sntp_poll_once(SNTP_QUERY_TIMEOUT_MS);
        if (r == SNTP_OK){
            break;
        }
        attempts++;
        uint32_t backoff_ms = (1u << (attempts <= 4 ? attempts : 4)) * 250u;
        uint32_t jitter = backoff_ms / 10u;
        sleep(backoff_ms + (jitter / 2u));
    }

    for(;;){
        sntp_poll_once(SNTP_QUERY_TIMEOUT_MS);
        sleep(SNTP_POLL_INTERVAL_MS);
    }
    return 1;
}
