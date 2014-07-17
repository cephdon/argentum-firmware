#include "axis.h"
#include "logging.h"

Axis::Axis(const char axis,
           ProtoMotor *motor,
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

    logger.info() << "Axis created for: " << axis << Comms::endl;
}

Axis::~Axis() {
}

bool Axis::run(void) {
    if(current_position == desired_position) {
        return false;
    } else {
        //logger.info() << current_position << " -> " << desired_position
        //        << Logger::endl;
        if(((current_position < desired_position) && positive_limit())
                || ((current_position > desired_position)
                    && negative_limit())) {
            logger.warn() << axis
                    << " tried to step in a limited direction, holding."
                    << Comms::endl;

            hold();

            return false;
        }

        return step();
    }
}

bool Axis::step(void) {
    bool did_step = motor->step();

    if(did_step) {
        //logger.info() << axis << " step" << Logger::endl;
        if(direction == Axis::Positive) {
            current_position++;
        } else {
            current_position--;
        }

        if(current_position == desired_position) {
            logger.info() << "Axis " << axis << " reached goal position: "
                    << desired_position << Comms::endl;
        }
        return true;
    }

    return false;
}

void Axis::set_direction(uint8_t direction) {
    if(direction == this->direction) {
        return;
    }

    this->direction = direction;
    //motor->swap_direction();

    if(direction == Axis::Positive) {
        if(motor_mapping == Axis::CW_Positive) {
            motor->set_direction(ProtoMotor::CW);
        } else {
            motor->set_direction(ProtoMotor::CCW);
        }
    } else if(direction == Axis::Negative) {
        if(motor_mapping == Axis::CW_Negative) {
            motor->set_direction(ProtoMotor::CW);
        } else {
            motor->set_direction(ProtoMotor::CCW);
        }
    }

    logger.info() << "Setting direction to " << direction << Comms::endl;
}

void Axis::move_absolute(double position) {
    uint32_t pos = position * steps_per_mm;

    logger.info() << "move_to(" << position << ") -> move_to(" << pos << ")"
            << Comms::endl;

    move_absolute(pos);
}

void Axis::move_absolute(uint32_t position) {
    if(position == desired_position || position == current_position) {
        return;
    }

    // Constrain the possible positions
    desired_position = max(position, 0);

    // This could really be ~14000
    desired_position = min(desired_position, 20000);

    logger.info() << "Setting new desired position to " << desired_position
            << Comms::endl;

    if(desired_position > current_position) {
        set_direction(Axis::Positive);
    } else {
        set_direction(Axis::Negative);
    }
}

void Axis::move_incremental(double increment) {
    uint32_t steps = increment * steps_per_mm;

    move_incremental(steps);
}

void Axis::move_incremental(uint32_t increment) {
    move_absolute(desired_position + increment);
}

double Axis::get_current_position(void) {
    return ((double)current_position) / steps_per_mm;
}

double Axis::get_desired_position(void) {
    return ((double)desired_position) / steps_per_mm;
}

void Axis::zero(void) {
    current_position = 0;
    desired_position = 0;
}

void Axis::hold(void) {
    desired_position = current_position;
}

bool Axis::moving(void) {
    return (current_position == desired_position);
}