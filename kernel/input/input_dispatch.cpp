#include "input_dispatch.h"
#include "process/process.h"
#include "process/scheduler.h"
#include "dwc2.hpp"
#include "xhci.hpp"
#include "hw/hw.h"
#include "std/std.h"
#include "kernel_processes/kprocess_loader.h"
#include "math/math.h"
#include "graph/graphics.h"

process_t* focused_proc;

typedef struct {
    keypress kp;
    int pid;
    bool triggered;
} shortcut;

shortcut shortcuts[16];

uint16_t shortcut_count = 0;

bool secure_mode = false;

USBDriver *input_driver = 0x0;

gpu_point mouse_loc;
gpu_size screen_bounds;

bool mouse_setup;

void register_keypress(keypress kp) {
    if (!secure_mode){
        for (int i = 0; i < shortcut_count; i++){
            if (shortcuts[i].pid != -1 && !is_new_keypress(&shortcuts[i].kp, &kp)){
                shortcuts[i].triggered = true;
                return;
            }
        }
    }

    if (!(uintptr_t)focused_proc) return;

    input_buffer_t* buf = &focused_proc->input_buffer;
    uint32_t next_index = (buf->write_index + 1) % INPUT_BUFFER_CAPACITY;

    buf->entries[buf->write_index] = kp;
    buf->write_index = next_index;

    if (buf->write_index == buf->read_index)
        buf->read_index = (buf->read_index + 1) % INPUT_BUFFER_CAPACITY;
}

void mouse_config(gpu_point point, gpu_size size){
    gpu_setup_cursor(point);
    mouse_loc = point;
    screen_bounds = size;
}

uint8_t last_cursor_state = 0;
mouse_input last_mouse_in;

mouse_input get_raw_mouse_in(){
    return last_mouse_in;
}

void register_mouse_input(mouse_input *rat){
    last_mouse_in = *rat;
    int32_t dx = rat->x;
    int32_t dy = rat->y;
    mouse_loc.x += dx;
    mouse_loc.y += dy;
    mouse_loc.x = min(max(0, mouse_loc.x), screen_bounds.width);
    mouse_loc.y = min(max(0, mouse_loc.y), screen_bounds.height);
    gpu_update_cursor(mouse_loc, false);
    uint8_t lmb = rat->buttons & 1;
    if (lmb != last_cursor_state){
        last_cursor_state = lmb;
        gpu_set_cursor_pressed(last_cursor_state);
        gpu_update_cursor(mouse_loc, true);
    }
}

gpu_point get_mouse_pos(){
    return mouse_loc;
}

bool mouse_button_pressed(mouse_button mb){
    return (last_cursor_state & (1 << mb)) == (1 << mb);
}

uint16_t sys_subscribe_shortcut_current(keypress kp){
    return sys_subscribe_shortcut(get_current_proc_pid(),kp);
}

uint16_t sys_subscribe_shortcut(uint16_t pid, keypress kp){
    shortcuts[shortcut_count] = (shortcut){
        .kp = kp,
        .pid = pid,
        .triggered = false
    };
    return shortcut_count++;
}

void sys_focus_current(){
    sys_set_focus(get_current_proc_pid());
}

void sys_set_focus(int pid){
    focused_proc = get_proc_by_pid(pid);
    focused_proc->focused = true;
}

void sys_unset_focus(){
    focused_proc->focused = false;
    focused_proc = 0;
}

void sys_set_secure(bool secure){
    secure_mode = secure;
}

bool sys_read_input_current(keypress *out){
    return sys_read_input(get_current_proc_pid(), out);
}

bool is_new_keypress(keypress* current, keypress* previous) {
    if (current->modifier != previous->modifier) return true;

    for (int i = 0; i < 6; i++)
        if (current->keys[i] != previous->keys[i]) return true;

    return false;
}

void remove_double_keypresses(keypress* current, keypress* previous){
    for (int i = 0; i < 6; i++)
        if (keypress_contains(previous, current->keys[i], previous->modifier)) current->keys[i] = 0;
}

bool keypress_contains(keypress *kp, char key, uint8_t modifier){
    if (kp->modifier != modifier) return false;//TODO: This is not entirely accurate, some modifiers do not change key

    for (int i = 0; i < 6; i++)
        if (kp->keys[i] == key)
            return true;
    return false;
}

bool sys_read_input(int pid, keypress *out){
    process_t *process = get_proc_by_pid(pid);
    if (process->input_buffer.read_index == process->input_buffer.write_index) return false;

    *out = process->input_buffer.entries[process->input_buffer.read_index];
    process->input_buffer.read_index = (process->input_buffer.read_index + 1) % INPUT_BUFFER_CAPACITY;
    return true;
}

bool sys_shortcut_triggered_current(uint16_t sid){
    bool value = sys_shortcut_triggered(get_current_proc_pid(), sid);
    return value;
}

bool sys_shortcut_triggered(uint16_t pid, uint16_t sid){
    if (shortcuts[sid].pid == pid && shortcuts[sid].triggered){
        shortcuts[sid].triggered = false;
        return true;
    } 
    return false;
}

// TODO: discard input before DOOM

bool input_init(){
    for (int i = 0; i < 16; i++) shortcuts[i] = {};
    if (BOARD_TYPE == 2 && RPI_BOARD != 5){
        input_driver = new DWC2Driver();//TODO: QEMU & 3 Only
        return input_driver->init();
    } else {
        input_driver = new XHCIDriver();
        return input_driver->init();
    }
}

int input_process_poll(int argc, char* argv[]){
    while (1){
        if (input_driver) input_driver->poll_inputs();
    }
    return 1;
}

void input_start_polling(){
    if (input_driver) input_driver->poll_inputs();
}

int input_process_fake_interrupts(int argc, char* argv[]){
    while (1){
        input_driver->handle_interrupt();
    }
    return 1;
}

void init_input_process(){
    if (!input_driver->use_interrupts)
        create_kernel_process("input_poll", &input_process_poll, 0, 0);
    if (input_driver->quirk_simulate_interrupts)
        create_kernel_process("input_int_mock", &input_process_fake_interrupts, 0, 0);
}

void handle_input_interrupt(){
    if (input_driver->use_interrupts) input_driver->handle_interrupt();
}

driver_module input_module = (driver_module){
    .name = "input",
    .mount = "/in",
    .version = VERSION_NUM(0, 1, 0, 1),
    .init = input_init,
    .fini = 0,
    .open = 0,
    .read = 0,
    .write = 0,
    .seek = 0,
    .readdir = 0,
};