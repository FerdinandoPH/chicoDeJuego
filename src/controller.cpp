#include <controller.h>
#include <memory.h>

Controller::Controller(Memory& mem) : mem(mem) {
    this->define_keys();
    this->reset();
}

void Controller::define_keys(){
    //It will be configurable in the future, for now it's hardcoded
    this->key_dict[SDL_SCANCODE_UP] = Key::UP;
    this->sdl_key_set.insert(SDL_SCANCODE_UP);
    this->key_dict[SDL_SCANCODE_DOWN] = Key::DOWN;
    this->sdl_key_set.insert(SDL_SCANCODE_DOWN);
    this->key_dict[SDL_SCANCODE_LEFT] = Key::LEFT;
    this->sdl_key_set.insert(SDL_SCANCODE_LEFT);
    this->key_dict[SDL_SCANCODE_RIGHT] = Key::RIGHT;
    this->sdl_key_set.insert(SDL_SCANCODE_RIGHT);
    this->key_dict[SDL_SCANCODE_Z] = Key::A;
    this->sdl_key_set.insert(SDL_SCANCODE_Z);
    this->key_dict[SDL_SCANCODE_X] = Key::B;
    this->sdl_key_set.insert(SDL_SCANCODE_X);
    this->key_dict[SDL_SCANCODE_RETURN] = Key::START;
    this->sdl_key_set.insert(SDL_SCANCODE_RETURN);
    this->key_dict[SDL_SCANCODE_BACKSPACE] = Key::SELECT;
    this->sdl_key_set.insert(SDL_SCANCODE_BACKSPACE);
}
void Controller::reset(){
    this->poll_mode = Poll_mode::NONE;
    this->keys_state = 0xFF;
}

u8 Controller::joyp_change(u8 data){
    u8 new_poll_mode = ((~data) >> 4) & 0x3;
    switch(new_poll_mode){
        case 1:
            this->poll_mode = Poll_mode::DIRECTION;
            return (data & 0xF0) | (this->keys_state & 0x0F);
            break;
        case 2:
            this->poll_mode = Poll_mode::BUTTON;
            return (data & 0xF0) | ((this->keys_state & 0xF0) >> 4);
            break;
        default:
            this->poll_mode = Poll_mode::NONE;
            return data | 0x0F;
            break;
    }
}

void Controller::key_down(SDL_Scancode sdl_key){
    if (this->sdl_key_set.count(sdl_key)){
        Key key = this->key_dict[sdl_key];
        Poll_mode key_pm = this->key_to_poll_mode[key];
        keys_state &= ~(1 << (this->key_to_bit[key] + (key_pm == Poll_mode::BUTTON ? 4 : 0)));
        if (this->poll_mode == key_pm){
            u8 new_value = (key_pm == Poll_mode::BUTTON ? 0b11010000 : 0b11100000);
            if (key_pm == Poll_mode::BUTTON){
                new_value |= ((keys_state & 0xF0) >> 4);
            }
            else{
                new_value |= (keys_state & 0x0F);
            }
            mem.writeX(JOYP_ADDR, new_value);
            mem.writeX(IF_ADDR, (u8)(mem.readX(IF_ADDR) | 0x10)); //Joypad interrupt
        }
    }
}

void Controller::key_up(SDL_Scancode sdl_key){
    if (this->sdl_key_set.count(sdl_key)){
        Key key = this->key_dict[sdl_key];
        Poll_mode key_pm = this->key_to_poll_mode[key];
        keys_state |= 1 << (this->key_to_bit[key] + (key_pm == Poll_mode::BUTTON ? 4 : 0));
        if (this->poll_mode == key_pm){
            u8 new_value = (key_pm == Poll_mode::BUTTON ? 0b11010000 : 0b11100000);
            if (key_pm == Poll_mode::BUTTON){
                new_value |= ((keys_state & 0xF0) >> 4);
            }
            else{
                new_value |= (keys_state & 0x0F);
            }
            mem.writeX(JOYP_ADDR, new_value);
        }
    }
}