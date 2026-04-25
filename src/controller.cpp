#include "controller.h"
#include "memory.h"
#include "sync.h"
#include "savestates.h"

Controller::Controller(Memory& mem) : mem(mem), event_queue(), event_queue_mutex() {
    this->define_keys();
    this->reset();
}
void Controller::set_sync_controller(Emu_sync* sync_controller){
    this->sync_controller = sync_controller;
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

    this->extra_key_dict[SDL_SCANCODE_SPACE] = Extra_key::TURBO;
    this->extra_sdl_key_set.insert(SDL_SCANCODE_SPACE);
    this->extra_key_dict[SDL_SCANCODE_F1] = Extra_key::SAVESTATE;
    this->extra_sdl_key_set.insert(SDL_SCANCODE_F1);
    this->extra_key_dict[SDL_SCANCODE_F2] = Extra_key::LOADSTATE;
    this->extra_sdl_key_set.insert(SDL_SCANCODE_F2);
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
            return ((data | 0xC0) & 0xF0) | (this->keys_state & 0x0F);
            break;
        case 2:
            this->poll_mode = Poll_mode::BUTTON;
            return ((data | 0xC0) & 0xF0) | ((this->keys_state & 0xF0) >> 4);
            break;
        default:
            this->poll_mode = Poll_mode::NONE;
            return data | 0xCF;
            break;
    }
}

void Controller::enqueue_event(SDL_Scancode key, Controller_event_type type){
    std::scoped_lock lock(this->event_queue_mutex);
    this->event_queue.push_back({type, key});
}

void Controller::process_events(){
    
    std::deque<Input_event> events_to_process;
    {
        std::scoped_lock lock(this->event_queue_mutex);
        if(this->event_queue.empty()) return;
        events_to_process.swap(this->event_queue);
    }
    for (const auto& event : events_to_process){
        if (event.type == Controller_event_type::KEY_DOWN){
            this->key_down(event.sdl_key);
        }
        else{
            this->key_up(event.sdl_key);
        }
    }
}

void Controller::key_down(SDL_Scancode sdl_key){
    if (this->sdl_key_set.count(sdl_key)){
        Key key = this->key_dict[sdl_key];
        Poll_mode key_pm = this->key_to_poll_mode[key];
        keys_state &= ~(1 << (this->key_to_bit[key] + (key_pm == Poll_mode::BUTTON ? 4 : 0)));
        if (this->poll_mode == key_pm){
            u8 new_value = mem.readX(JOYP_ADDR) & 0xF0;
            if (key_pm == Poll_mode::BUTTON){
                new_value |= ((keys_state & 0xF0) >> 4);
            }
            else{
                new_value |= (keys_state & 0x0F);
            }
            mem.writeX(JOYP_ADDR, new_value);
            mem.writeX(IF_ADDR, (u8)(mem.readX(IF_ADDR) | 0x10)); //Joypad interrupt
        }
    }else if (this->extra_sdl_key_set.count(sdl_key)){
        Extra_key extra_key = this->extra_key_dict[sdl_key];
        switch(extra_key){
            case Extra_key::TURBO:
                sync_controller->set_turbo_mode(true);
                break;
            case Extra_key::SAVESTATE:
                if (save_state_manager){
                    printf("Saving state... ");
                    save_state_manager->save_state();
                    printf("Done.\n");
                }
                break;
            case Extra_key::LOADSTATE:
                if (save_state_manager){
                    printf("Loading state... ");
                    save_state_manager->load_state();
                    printf("Done.\n");
                }
                break;
        }
    }
}

void Controller::key_up(SDL_Scancode sdl_key){
    if (this->sdl_key_set.count(sdl_key)){
        Key key = this->key_dict[sdl_key];
        Poll_mode key_pm = this->key_to_poll_mode[key];
        keys_state |= 1 << (this->key_to_bit[key] + (key_pm == Poll_mode::BUTTON ? 4 : 0));
        if (this->poll_mode == key_pm){
            u8 new_value = mem.readX(JOYP_ADDR) & 0xF0;
            if (key_pm == Poll_mode::BUTTON){
                new_value |= ((keys_state & 0xF0) >> 4);
            }
            else{
                new_value |= (keys_state & 0x0F);
            }
            mem.writeX(JOYP_ADDR, new_value);
        }
    }else if (this->extra_sdl_key_set.count(sdl_key)){
        Extra_key extra_key = this->extra_key_dict[sdl_key];
        switch(extra_key){
            case Extra_key::TURBO:
                sync_controller->set_turbo_mode(false);
                break;
            default:
                break;
        }
    }
}

void Controller::set_save_state_manager(SaveStateManager* manager){
    this->save_state_manager = manager;
}