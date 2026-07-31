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
    this->key_dict[Host_key::UP] = Key::UP;
    this->key_set.insert(Host_key::UP);
    this->key_dict[Host_key::DOWN] = Key::DOWN;
    this->key_set.insert(Host_key::DOWN);
    this->key_dict[Host_key::LEFT] = Key::LEFT;
    this->key_set.insert(Host_key::LEFT);
    this->key_dict[Host_key::RIGHT] = Key::RIGHT;
    this->key_set.insert(Host_key::RIGHT);
    this->key_dict[Host_key::Z] = Key::A;
    this->key_set.insert(Host_key::Z);
    this->key_dict[Host_key::X] = Key::B;
    this->key_set.insert(Host_key::X);
    this->key_dict[Host_key::RETURN] = Key::START;
    this->key_set.insert(Host_key::RETURN);
    this->key_dict[Host_key::BACKSPACE] = Key::SELECT;
    this->key_set.insert(Host_key::BACKSPACE);

    this->extra_key_dict[Host_key::SPACE] = Extra_key::TURBO;
    this->extra_key_set.insert(Host_key::SPACE);
    this->extra_key_dict[Host_key::F1] = Extra_key::SAVESTATE;
    this->extra_key_set.insert(Host_key::F1);
    this->extra_key_dict[Host_key::F2] = Extra_key::LOADSTATE;
    this->extra_key_set.insert(Host_key::F2);
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

void Controller::enqueue_event(Host_key key, Controller_event_type type){
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
            this->key_down(event.key);
        }
        else{
            this->key_up(event.key);
        }
    }
}

void Controller::key_down(Host_key host_key){
    if (this->key_set.count(host_key)){
        Key key = this->key_dict[host_key];
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
    }else if (this->extra_key_set.count(host_key)){
        Extra_key extra_key = this->extra_key_dict[host_key];
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

void Controller::key_up(Host_key host_key){
    if (this->key_set.count(host_key)){
        Key key = this->key_dict[host_key];
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
    }else if (this->extra_key_set.count(host_key)){
        Extra_key extra_key = this->extra_key_dict[host_key];
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