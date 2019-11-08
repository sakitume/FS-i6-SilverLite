#ifndef __BUTTONS_H__
#define __BUTTONS_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

//------------------------------------------------------------------------------
// Buttons are configured as a 4x3 matrix. L1-L4 are rows, R1-R3 are columns
//
//      R1      R2          R3
// L1   Roll-R  Throttle-U  Down
// L1   Roll-L  Throttle-D  Up
// L3   Pitch-U Yaw-R       Ok
// L4   Pitch-D Yaw-L       Cancel
//

typedef enum e_BtnIndex
{
    // The following buttons are in a 4x3 matrix, normally low
    kBtn_RollR,     kBtn_ThrottleU,     kBtn_Down,
    kBtn_RollL,     kBtn_ThrottleD,     kBtn_Up,
    kBtn_PitchU,    kBtn_YawR,          kBtn_Ok,
    kBtn_PitchD,    kBtn_YawL,          kBtn_Cancel,

    // These 3 are discrete button pins, normally low
    kBtn_Bind,
    kBtn_SwA,
    kBtn_SwD
} e_BtnIndex;


void buttons_init(void);
void buttons_update(void);
void buttons_test(void);

int button_toggled(e_BtnIndex btnIndex);
int button_toggledActive(e_BtnIndex btnIndex);
int button_active(e_BtnIndex btnIndex);
int button_raw_state(e_BtnIndex btnIndex);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif  // #ifndef __BUTTONS_H__
