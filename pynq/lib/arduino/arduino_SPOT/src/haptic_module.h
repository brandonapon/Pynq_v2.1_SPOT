#ifndef __HAPTIC_MODULE_H__
#define __HAPTIC_MODULE_H__

// haptic motor driver address
#define DRV2605_ADDRESS (0x5A)

void write_hapt(u8 reg, u8 data);
u8 read_hapt(u8 reg);
void play_hapt(u8 *waveforms);
void stop_hapt();
u32 is_playing_hapt();
void auto_calibrate_hapt();

#endif
