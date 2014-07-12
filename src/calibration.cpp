#include "calibration.h"
#include "limit_switch.h"

#include "settings.h"
#include "axis.h"

#include "utils.h"

/**
 * Attempt to resolve the direction and a axis of a motor.
 *
 * Assumptions:
 *
 * Only one switch is affected by the movement, and the axes are long enough such
 * that the supplied movement distance won't release and trigger the limits of
 * that axis.
 *
 * The (released | triggered) logic simplification relies on this.
 *
 * @param motor The motor to attempt to resolve
 * @param steps The number of steps to move that motor
 * @param axis_mask A mask defining the axis that this motor should correspond
 to.

 * @param axis_correct A pointer to a bool, which will indicate whether the
 axis_mask corresponds to any of the limit switches that were triggered or
 released.

 * @param direction_mask A mask defining the direction that this motor should
 move in for the given step, typically positive.

 * @param direction_correct A pointer to a bool...
 */
bool resolve(Motor *motor,
             long steps,
             uint8_t axis_mask,
             bool *axis_correct,
             uint8_t direction_mask,
             bool *direction_correct) {

    uint8_t before = limit_switches();
    motor->move(steps);
    uint8_t after = limit_switches();

    uint8_t released = (before & ~after);
    uint8_t triggered = (after & ~before);

    // If we activated a switch, release it. Should this happen?
    if(triggered) {
        motor->move(-steps);
    }

    if(released || triggered) {
        *axis_correct = ((released | triggered) & axis_mask);

        *direction_correct = ((released | triggered) & direction_mask);

        if(released) {
            *direction_correct = !(*direction_correct);
        }

        return true;
    }

    return false;
}

bool freedom(bool *x_direction_resolved, bool *y_direction_resolved) {
    *x_direction_resolved = false;
    *y_direction_resolved = false;

    bool a_resolved = false;
    bool b_resolved = false;

    if(any_limit()) {
        bool axis_correct = false;
        bool direction_correct = false;

        // A -> X+
        a_resolved = resolve(&aMotor, a_escape_steps, X_MASK, &axis_correct, POS_MASK, &direction_correct);

        if(!a_resolved) {
            a_resolved = resolve(&aMotor, -a_escape_steps, X_MASK, &axis_correct, NEG_MASK, &direction_correct);
        }

        if(a_resolved) {
            if(axis_correct) {
            //    infoln("Found A = X");

                xMotor = &aMotor;
                yMotor = &bMotor;

                *x_direction_resolved = true;
            } else {
            //    infoln("Found A = Y");

                xMotor = &bMotor;
                yMotor = &aMotor;

                *y_direction_resolved = true;
            }

            if(direction_correct) {
            //    infoln("Found + = +");
            } else {
            //    infoln("Found + = -");
                aMotor.set_inverted(true);
            }
        }

        // B -> Y+
        b_resolved = resolve(&bMotor, b_escape_steps, Y_MASK, &axis_correct, POS_MASK, &direction_correct);

        if(!b_resolved) {
            b_resolved = resolve(&bMotor, -b_escape_steps, Y_MASK, &axis_correct, NEG_MASK, &direction_correct);
        }

        if(b_resolved) {
            if(axis_correct) {
            //    infoln("Found B = Y");
                xMotor = &aMotor;
                yMotor = &bMotor;

                *y_direction_resolved = true;
            } else {
            //    infoln("Found B = X");
                xMotor = &bMotor;
                yMotor = &aMotor;

                *x_direction_resolved = true;
            }

            if(direction_correct) {
            //    infoln("Found + = +");
            } else {
                //infoln("Found + = -");
                bMotor.set_inverted(true);
            }
        }
    } else {
        //infoln("No switches are initially triggered.");
    }

    return (a_resolved | b_resolved);
}

void calibrate(CalibrationData *calibration) {
    //infoln("Calibration beginning.");

    xMotor->set_direction(Motor::Forward);
    yMotor->set_direction(Motor::Forward);

    xMotor->set_speed(250);
    yMotor->set_speed(250);

    long x_distance = 0L;
    long y_distance = 0L;

    bool axes_resolved = false;
    bool x_direction_resolved = false;
    bool y_direction_resolved = false;

    axes_resolved = freedom(&x_direction_resolved, &y_direction_resolved);

    xMotor->set_speed(2500);
    yMotor->set_speed(2500);

    if(!axes_resolved) {
        //infoln("Resolved nothing");
    //    infoln("Finding X");

        while(!any_limit()) {
            xMotor->move(-1);
        }

        if(y_limit()) {
            // Incorrect
            Motor *temp = xMotor;

            xMotor = yMotor;
            yMotor = temp;
        }
    }

    while(!(x_direction_resolved && y_direction_resolved)) {
        if(!x_direction_resolved) {
            if(!x_limit()) {
                xMotor->move(-1);
            } else {
                x_direction_resolved = true;

                if(x_pos_limit()) {
                    // Inverted
                    xMotor->set_inverted(true);
                }
            }
        }

        if(!y_direction_resolved) {
            if(!y_limit()) {
                yMotor->move(-1);
            } else {
                y_direction_resolved = true;

                if(y_pos_limit()) {
                    // Inverted
                    yMotor->set_inverted(true);
                }
            }
        }
    }

    log_info("Homing :");

    log_info_np("1");

    int expected_x = 13791;
    int expected_y = 10764;
    int tolerance = 50;

    while(!pos_limit() && x_distance < (expected_x + tolerance) && y_distance < (expected_y + tolerance)) {
        xMotor->move(1);
        yMotor->move(1);

        x_distance++;
        y_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    log_info_np("2");

    while(!x_pos_limit() && x_distance < (expected_x + tolerance)) {
        xMotor->move(1);

        x_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    log_info_np("3");

    while(!y_pos_limit() && y_distance < (expected_y + tolerance)) {
        yMotor->move(1);

        y_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    log_info_np("4");

    x_distance = 0;
    y_distance = 0;

    while(!neg_limit() && x_distance < (expected_x + tolerance) && y_distance < (expected_y + tolerance)) {
        xMotor->move(-1);
        yMotor->move(-1);

        x_distance++;
        y_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    log_info_np("5");

    while(!x_neg_limit() && x_distance < (expected_x + tolerance)) {
        xMotor->move(-1);
        x_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    //infonp("6");

    while(!y_neg_limit() && y_distance < (expected_y + tolerance)) {
        yMotor->move(-1);
        y_distance++;
    }

    if(!(x_distance < (expected_x + tolerance) || y_distance < (expected_y + tolerance))) {
        log_info("JAMMED");
        while(1);
    }

    xMotor->reset_position();
    yMotor->reset_position();

    log_info("\r\n");

    if(calibration) {
        calibration->x_axis.motor = (xMotor == &aMotor) ? Motor::A : Motor::B;
        calibration->x_axis.flipped = xMotor->is_inverted();
        calibration->x_axis.length = x_distance;

        calibration->y_axis.motor = (yMotor == &aMotor) ? Motor::A : Motor::B;
        calibration->y_axis.flipped = yMotor->is_inverted();
        calibration->y_axis.length = y_distance;
    }
}
