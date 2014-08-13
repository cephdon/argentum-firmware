#include "axis.h"
#include "logging.h"

#include "../argentum/argentum.h"

Axis::Axis(const char axis,
           Stepper *motor,
           bool (*positive_limit)(void),
           bool (*negative_limit)(void)) {
    this->axis = axis;
    this->motor = motor;
    this->positive_limit = positive_limit;
    this->negative_limit = negative_limit;

    length = 0;
    current_position = 0;
    desired_position = 0;

    motor_mapping = Axis::CW_Positive;

    direction = Axis::Positive;

    motor->set_speed(1000);

    //logger.info() << "Axis created for: " << axis << Comms::endl;
}

Axis::~Axis() {
}

bool Axis::run(void) {
    if(current_position == desired_position) {
        return false;
    } else {
        logger.info() << current_position << " -> " << desired_position
                << Comms::endl;

        if(((current_position < desired_position) && positive_limit())
                || ((current_position > desired_position)
                && negative_limit())) {
            /*logger.warn() << axis
                    << " tried to step in a limited direction, holding."
                    << " current_position: " << current_position
                    << " desired_position: " << desired_position
                    << Comms::endl;*/
            logger.warn("LIM");

            hold();

            return false;
        }

        return step();
    }
}

bool Axis::step(void) {
    bool did_step = motor->step();

    if(did_step) {
        if(direction == Axis::Positive) {
            current_position++;
        } else {
            // 'Push' the limit only if we can still go backwards, but aren't at
            // position 0
            if(current_position > 0) {
                current_position--;
            }
        }
    }

    return did_step;
}

void Axis::set_direction(uint8_t direction) {
    if(direction == this->direction) {
        return;
    }

    this->direction = direction;
    //motor->swap_direction();

    set_motor_direction();

    logger.info() << axis << " axis setting direction to " << direction
            << Comms::endl;
}

void Axis::move_absolute(double position) {
    if(position < 0) {
        logger.error() << axis << " absolute movement with negative position ("
                << position << ")";
    }

    uint32_t pos = position * steps_per_mm;

    move_absolute(pos);
}

void Axis::move_absolute(uint32_t position) {
    if(position == desired_position || position == current_position) {
        return;
    }

    //logger.info() << axis << " axis absolute movement from " << current_position
    //    << " to " << position << "" << Comms::endl;

    // Constrain the possible positions
    desired_position = max(position, 0);

    // This could really be ~14000
    desired_position = min(desired_position, 16000);

    //logger.info() << axis << " axis setting new desired position to "
    //    << desired_position << Comms::endl;

    if(desired_position > current_position) {
        set_direction(Axis::Positive);
    } else {
        set_direction(Axis::Negative);
    }
}

void Axis::move_incremental(double increment) {
    int32_t steps = increment * steps_per_mm;

    move_incremental(steps);
}

void Axis::move_incremental(int32_t increment) {
    uint32_t new_desired_position = desired_position + increment;

    //logger.info() << axis << " axis given increment of (" << increment
    //        << ")" << Comms::endl;

    if(((int32_t)desired_position + increment) < 0) {
        logger.error() << axis << " axis given incremental move below 0.000 ("
                << increment << ")" << Comms::endl;

        new_desired_position = 0;
    }

    move_absolute(new_desired_position);
}

void Axis::move_to_positive(void) {
    set_direction(Axis::Positive);

    while(!positive_limit()) {
        while(!step());
    }

    hold();
}

void Axis::move_to_negative(void) {
    set_direction(Axis::Negative);

    while(!negative_limit()) {
        while(!step());
    }

    hold();
}

uint32_t Axis::get_current_position(void) {
    //return ((double)current_position) / steps_per_mm;
    return current_position;
}

uint32_t Axis::get_desired_position(void) {
    //return ((double)desired_position) / steps_per_mm;
    return desired_position;
}

void Axis::zero(void) {
    current_position = 0;
    desired_position = 0;
}

void Axis::hold(void) {
    desired_position = current_position;
}

bool Axis::moving(void) {
    return (current_position != desired_position);
}

void Axis::wait_for_move(void) {
    while(moving()) {
        run();
    }
}

void Axis::set_speed(uint32_t mm_per_minute) {
    motor->set_speed(mm_per_minute);
}

uint8_t Axis::get_motor_mapping(void) {
    return motor_mapping;
}

void Axis::set_motor_mapping(uint8_t motor_mapping) {
    this->motor_mapping = motor_mapping;

    logger.info() << axis << " axis motor_mapping = " << motor_mapping
            << Comms::endl;

    set_motor_direction();
}

void Axis::set_motor(Stepper *motor) {
    this->motor = motor;

    set_motor_direction();
}

void Axis::set_motor_direction(void) {
    if(direction == Axis::Positive) {
        if(motor_mapping == Axis::CW_Positive) {
            motor->set_direction(Stepper::CW);
        } else {
            motor->set_direction(Stepper::CCW);
        }
    } else if(direction == Axis::Negative) {
        if(motor_mapping == Axis::CW_Negative) {
            motor->set_direction(Stepper::CW);
        } else {
            motor->set_direction(Stepper::CCW);
        }
    }
}

Stepper * Axis::get_motor(void) {
    return motor;
}

void Axis::debug_info(void) {
    LoggerWrapper &info = logger.info() << axis << " axis, ";

    if(motor == &a_motor) {
        info << "a";
    } else {
        info << "b";
    }

    info << " motor, ";

    if(direction == Axis::Positive) {
        info << "+";
    } else {
        info << "-";
    }

    info << " direction, ";

    if(motor->get_direction() == Stepper::CW) {
        info << "CW";
    } else {
        info << "CCW";
    }

    info << " motor";

    if(motor_mapping == CW_Positive) {
        info << " (+CW STD)";
    } else {
        info << " (-CW INV)";
    }

    info << Comms::endl;
}
