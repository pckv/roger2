#include <ZumoBuzzer.h>

const int MELODY_LENGTH = 28;

unsigned char melodyNotes[MELODY_LENGTH] = {NOTE_C(5), NOTE_D(5), NOTE_E(5), NOTE_F(5), NOTE_G(5), SILENT_NOTE,
                                            NOTE_G(5), SILENT_NOTE, NOTE_A(5), NOTE_A(5), NOTE_A(5), NOTE_A(5),
                                            NOTE_G(5), SILENT_NOTE, NOTE_F(5), NOTE_F(5), NOTE_F(5), NOTE_F(5),
                                            NOTE_E(5), SILENT_NOTE, NOTE_E(5), SILENT_NOTE, NOTE_D(5), NOTE_D(5),
                                            NOTE_D(5), NOTE_D(5), NOTE_C(5), SILENT_NOTE};

unsigned int noteDuration[MELODY_LENGTH] = {50, 50, 50, 50, 190, 10, 190, 10, 25, 25, 25, 25, 700, 100, 25, 25, 25, 25,
                                            190, 10, 190, 10, 25, 25, 25, 25, 700, 100};