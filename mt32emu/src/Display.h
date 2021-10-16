/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2021 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_DISPLAY_H
#define MT32EMU_DISPLAY_H

#include "globals.h"
#include "Types.h"

namespace MT32Emu {

class Synth;

/** Facilitates emulation of internal state of the MIDI MESSAGE LED and the MT-32 LCD. */
class Display {
public:
	static const unsigned int LCD_TEXT_SIZE = 20;

	enum Mode {
		Mode_MAIN, // a.k.a. Master Volume
		Mode_STARTUP_MESSAGE,
		Mode_PROGRAM_CHANGE,
		Mode_CUSTOM_MESSAGE,
		Mode_ERROR_MESSAGE
	};

	Display(Synth &synth);
	/** Returns whether the MIDI MESSAGE LED is ON and fills the targetBuffer parameter. */
	bool updateDisplayState(char *targetBuffer);

	void midiMessagePlayed();
	void rhythmNotePlayed();
	void programChanged(Bit8u partIndex);
	void checksumErrorOccurred();
	bool customDisplayMessageReceived(const Bit8u *message, Bit32u startIndex, Bit32u length);

private:
	typedef Bit8u DisplayBuffer[LCD_TEXT_SIZE];

	Synth &synth;

	Mode mode;
	Bit32u displayResetTimestamp;
	bool displayResetScheduled;
	Bit32u midiMessageLEDResetTimestamp;
	bool midiMessagePlayedSinceLastReset;
	Bit32u rhythmStateResetTimestamp;
	bool rhythmNotePlayedSinceLastReset;

	DisplayBuffer displayBuffer;
	DisplayBuffer customMessageBuffer;

	void scheduleDisplayReset();
	bool shouldResetTimer(Bit32u scheduledResetTimestamp);
	void maybeResetTimer(bool &timerState, Bit32u scheduledResetTimestamp);
};

} // namespace MT32Emu

#endif // #ifndef MT32EMU_DISPLAY_H
