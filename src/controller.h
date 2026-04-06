#pragma once
#include <utils.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <SDL2/SDL.h>
class Memory;
enum class Key{UP, DOWN, LEFT, RIGHT, A, B, START, SELECT};
enum class Poll_mode{DIRECTION, BUTTON, NONE};
class Controller {
    private:
        Memory& mem;
        Poll_mode poll_mode;
        u8 keys_state = 0xFF; // First 4 bits for buttons, last 4 for directions. 0 means pressed, 1 means released
        std::unordered_map<SDL_Scancode, Key> key_dict;
        std::unordered_set<SDL_Scancode> sdl_key_set;
        std::unordered_map<Key, u8> key_to_bit = {
            {Key::RIGHT, 0}, {Key::LEFT, 1}, {Key::UP, 2}, {Key::DOWN, 3},
            {Key::A, 0}, {Key::B, 1}, {Key::SELECT, 2}, {Key::START, 3}
        };
        std::unordered_map<Key, Poll_mode> key_to_poll_mode = {
            {Key::RIGHT, Poll_mode::DIRECTION}, {Key::LEFT, Poll_mode::DIRECTION}, {Key::UP, Poll_mode::DIRECTION}, {Key::DOWN, Poll_mode::DIRECTION},
            {Key::A, Poll_mode::BUTTON}, {Key::B, Poll_mode::BUTTON}, {Key::SELECT, Poll_mode::BUTTON}, {Key::START, Poll_mode::BUTTON}
        };
        void define_keys();
    public:
        Controller(Memory& mem);
        void reset();
        u8 joyp_change(u8 data);
        void key_down(SDL_Scancode key);
        void key_up(SDL_Scancode key);
};