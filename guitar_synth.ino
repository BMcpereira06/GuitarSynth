
#include "config.h"
#include "frequencyToNote.h"
#include "pitchToNote.h"
#include <SoftwareSerial.h>


int notesArray[DEPTH];                        // Array to store detected notes and find the "correct" note which occurred the most often
int occurrences[DEPTH];                       // Array in which the number of occurrences for each note are stored
bool marked[DEPTH];                           // Array to indicate which of the notes have been checked
int frequencyIndex = 0;                       // Used to navigate to where the current note must be stored

int previousNote;
unsigned int startTime;                       // Used to determine when the note must stop
unsigned int humanTime;


byte newData = 0;
byte prevData = 0;

//freq variables
unsigned int timer = 0;//counts period of wave
unsigned int period;
int frequency;

SoftwareSerial mySerial(2, 3); // RX, TX

void setup()
{
  mySerial.begin(31250);
  Serial.begin(9600);
  freq_setup();

  // Hw configuration
  for (int i = 0; i < 7; i++)
  {
    pinMode(pins[i], INPUT);
  }

  midi_setup();

}

void midi_setup()
{
  pinMode(RESET_MIDI, OUTPUT);
  digitalWrite(RESET_MIDI, LOW);
  delay(100);
  digitalWrite(RESET_MIDI, HIGH);
  delay(100);
  talkMIDI(0xB0, 0x07, 120);
  delay(100);
  talkMIDI(0xB0, 0, 0x00); //Default bank GM1
  delay(100);
}

void freq_setup()
{
  cli();//diable interrupts

  //set up continuous sampling of analog pin 0

  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only

  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements

  sei();//enable interrupts
}

ISR(ADC_vect) 
{
  //when new ADC value ready
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 512 && newData >= 512)
  { //if increasing and crossing midpoint
    period = timer;//get period
    timer = 0;//reset timer
  }
  timer++; //increment timer at rate of 38.5kHz
}

float get_frequency()
{
  return frequency = 38462 / period;
}

void loop()
{
  test_shield_playing();
  float frequency = get_frequency();
  Serial.print("Frequency: ");
  Serial.println(frequency, 2);
  int instrument = get_instrument();
  set_instrument(instrument);
  Serial.print("Intrument: ");
  Serial.print(instrument);
  //process_freq(frequency, instrument);
  play_midi(frequency);
}

void set_instrument(int instrument)
{
  delay(250);
  Serial.println("Changing instrument");
  talkMIDI(0xC0, instrument, 0);
  delay(250);
}

int get_instrument()
{
  byte selector = 0;
  byte pinState;
  Serial.println("Pins");
  for (int i = 0; i < 7; i++)
  {
    pinState = digitalRead(pins[i]);

    Serial.println(pinState);
    if (pinState == HIGH)
    {
      selector |= 1 << i;
    }
  }
  return ( selector );
}

void play_midi(float frequency)
{
  if (frequency > 0)
  {
    int note_to_play = get_note(frequency);
    if (note_to_play != previousNote)
    {
       noteOff(0, previousNote, INTENSITY);
       noteOn(0, note_to_play, INTENSITY);
       previousNote = note_to_play;
    }
       
  }
  else
  {
    noteOff(0, previousNote, INTENSITY);
  }
}
int get_note(float frequency)
{
    int noteIndex = searchForNote(frequency); // Find the index of the corresponding frequency
    return notePitch[noteIndex];
}

void test_shield_playing()
{
  Serial.println("Basic Instruments");
  talkMIDI(0xB0, 0, 0x00); //Default bank GM1
   int instrument;
   int note;
  //Change to different instrument
  for (instrument = 0 ; instrument < 127 ; instrument++) {

    Serial.print(" Instrument: ");
    Serial.println(instrument, DEC);

    talkMIDI(0xC0, instrument, 0); //Set instrument number. 0xC0 is a 1 data byte command

    //Play notes from F#-0 (30) to F#-5 (90):
    for (note = 30 ; note < 40 ; note++) {
      Serial.print("N:");
      Serial.println(note, DEC);

      //Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
      noteOn(0, note, 60);
      delay(50);

      //Turn off the note with a given off/release velocity
      noteOff(0, note, 60);
      delay(50);
    }

    delay(100); //Delay between instruments
  }
}

void process_freq(float frequency, int instrument)
{
  if (frequency > 0)
  {
    int noteIndex = searchForNote(frequency); // Find the index of the corresponding frequency
    int note = notePitch[noteIndex];          // Use that index to find the corresponding note in the LUT
    notesArray[frequencyIndex++] = note;      // Store the note and continue to next value in array

    if (frequencyIndex > DEPTH)               // If all the notes have been stored
    {
      frequencyIndex = 0;                     // Reset the index
      int i, j;

      /*Reset all the occurences and marked positions*/
      for (i = 0; i < DEPTH; i++)
      {
        occurrences[i] = 0;
        marked[i] = 0;
      }

      /*Count the number of occurrences*/
      for (i = 0; i < DEPTH; i++)
      {
        for (j = 0; j < DEPTH; j++)
        {
          // If notes are the same and the note has not been marked yet
          if ((!marked[j]) && (notesArray[j] == notesArray[i]))
          {
            occurrences[i]++;                 // Increment the number of occurrences
            marked[j] = true;                 // Signal the note as marked
          }
        }
      }

      int numberOfdifferentFrequencies = 0;   // Used to determine how many different Frequencies have been detected

      for (i = 0; i < DEPTH; i++)
      {
        // If the counter does not equal zero
        if (occurrences[i] != 0)
        {
          // Store the the various detected Frequencies
          notesArray[numberOfdifferentFrequencies] = notesArray[i];
          // And the number of occurrences for each note
          occurrences[numberOfdifferentFrequencies] = occurrences[i];
          numberOfdifferentFrequencies++;      // Increment the number of detected Frequencies
        }
      }

      /*Search for the maximum number of occurrences to discriminate the played note*/
      int maxNumberOfFrequencies = occurrences[0];
      int rightIndex = 0;

      for (i = 0; i < numberOfdifferentFrequencies; i++);
      {
        // If a new maximum exist
        if (occurrences[i] > maxNumberOfFrequencies)
        {
          // Update the value
          maxNumberOfFrequencies = occurrences[i];
          // Update the index
          rightIndex = i;
        }
      }
      note = notesArray[rightIndex];          // Note to be played is that with the most occurrences
      // If the specified time has elapsed before the next note
      if (millis() - humanTime > HUMAN_RATE)
      {
        humanTime = millis();                 // Update the timer
        startTime = millis();                 // Update the note duration
        noteOff(0, previousNote, INTENSITY);  // Stop playing the previous note
        previousNote = note;                  // Update previous note with the new one
        Serial.println(note);                 // Print the note to be played
        noteOn(0, note, INTENSITY);           // Play the note!
      }
    }
  }

  if (millis() - startTime > MAX_DURATION)    // If maximum time elapsed
    noteOff(0, previousNote, INTENSITY);      // Turn the note off
}



int searchForNote(float frequency)
{
  float minimum = abs(frequency - noteFrequency[0]);
  float newMinimum;
  int index = 0;

  /*Search for the nearest frequency that is in the vector*/
  for (int i = 0; i < NUMBER_OF_NOTES - 1; i++)
  {
    newMinimum = abs(frequency - noteFrequency[i]);
    if (newMinimum < minimum)
    {
      minimum = newMinimum;
      index = i;
    }
  }
}



void talkMIDI(byte cmd, byte data1, byte data2) {
  mySerial.write(cmd);
  mySerial.write(data1);

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if ( (cmd & 0xF0) <= 0xB0)
    mySerial.write(data2);
}

void noteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void noteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}
