#define DEPTH                60                      // Defines depth of the array for averaged frequencies
#define HUMAN_RATE           50                      // Defines 50ms corresponding to 20 notes/s
#define MAX_DURATION         1000                    // Defines the max play duration of the note
#define GUITAR_LOW_FREQ      80.0                  // Bandwidth = [GUITAR_LOW_FREQ,GUITAR_HIGH_FREQ ]
#define GUITAR_HIGH_FREQ     1200.0

#define INTENSITY            64

#define ANALOGUE_PORT        A0
#define THUMBWHEEL_INPUT0    22
#define THUMBWHEEL_INPUT1    24
#define THUMBWHEEL_INPUT2    26
#define THUMBWHEEL_INPUT3    28
#define THUMBWHEEL_INPUT4    30
#define THUMBWHEEL_INPUT5    32
#define THUMBWHEEL_INPUT6    34


const int pins[7] = {THUMBWHEEL_INPUT0, THUMBWHEEL_INPUT1, THUMBWHEEL_INPUT2, THUMBWHEEL_INPUT3, THUMBWHEEL_INPUT4, THUMBWHEEL_INPUT5, THUMBWHEEL_INPUT6};
void talkMIDI(byte cmd, byte data1, byte data2);
int searchForNote(float frequency);
void noteOn(byte channel, byte note, byte attack_velocity);
void noteOff(byte channel, byte note, byte release_velocity);
int get_instrument();
void set_instrument(int instrument);
