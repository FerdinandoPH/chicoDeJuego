#pragma once
#include "utils.h"
#include "input.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <deque>
class Memory;
class SaveStateManager;
class Emu_sync;
enum class Key{UP, DOWN, LEFT, RIGHT, A, B, START, SELECT};
enum class Extra_key{TURBO, SAVESTATE, LOADSTATE};
enum class Poll_mode{DIRECTION, BUTTON, NONE};
class Controller {
    private:
        Memory& mem;
        Emu_sync* sync_controller;
        Poll_mode poll_mode;
        u8 keys_state = 0xFF; // First 4 bits for buttons, last 4 for directions. 0 means pressed, 1 means released
        std::unordered_map<Host_key, Key> key_dict;
        std::unordered_set<Host_key> key_set;
        std::unordered_map<Host_key, Extra_key> extra_key_dict;
        std::unordered_set<Host_key> extra_key_set;
        std::unordered_map<Key, u8> key_to_bit = {
            {Key::RIGHT, 0}, {Key::LEFT, 1}, {Key::UP, 2}, {Key::DOWN, 3},
            {Key::A, 0}, {Key::B, 1}, {Key::SELECT, 2}, {Key::START, 3}
        };
        std::unordered_map<Key, Poll_mode> key_to_poll_mode = {
            {Key::RIGHT, Poll_mode::DIRECTION}, {Key::LEFT, Poll_mode::DIRECTION}, {Key::UP, Poll_mode::DIRECTION}, {Key::DOWN, Poll_mode::DIRECTION},
            {Key::A, Poll_mode::BUTTON}, {Key::B, Poll_mode::BUTTON}, {Key::SELECT, Poll_mode::BUTTON}, {Key::START, Poll_mode::BUTTON}
        };
        std::deque<Input_event> event_queue;
        std::mutex event_queue_mutex;
        SaveStateManager* save_state_manager;
        void define_keys();
        void key_down(Host_key key);
        void key_up(Host_key key);
    public:
        Controller(Memory& mem);
        void set_sync_controller(Emu_sync* sync_controller);
        void reset();
        u8 joyp_change(u8 data);
        void enqueue_event(Host_key key, Controller_event_type type);
        void process_events();
        void set_save_state_manager(SaveStateManager* manager);
};