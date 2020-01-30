#include <EEPROM.h>

// TODO: separate play and record knobs!
// Safe: completely works before adding separate play and record modes.


// Ryan Live Sequencer Sketch
/*
  Allows storage and playback of four sequences of 64 notes. Each
  note can be between 1 and 8 steps long. Recording while not playing is
  much easier, but it is possible to record one sequence while playing
  another. Playback is via the A3 knob, or as long as a HIGH is received
  on the jack. Sequences can be selected via knob or CV.

*/
//
// Knob A0: (record mode ) currently play and record
// Knob A1: note length
// Analog In A2: (play)
// Analog In A3: sequence selector
// Digital Out 1: Gate out
// Digital Out 2: Clock out 
// Clock In: Clock In
// Analog Out: CV out

// Expander:
// A4 in: Record Gate in
// A5: Record CV In (leave attenuator open, accurate but halves note range)
// 13 and 11: 
// 8 bits: displays step length in record mode. (todo: show seq number)

/*
Todo: 

- flash sequence number on LEDs in record mode.
- memory: can we do 8 sequences now? They're easier to select now.
- it pauses while saving, any way around that?
  - fixed sequence length, write each note as it happens
  - is that better?
  - OR only write notes to EEPROM if not playing.
- play and record are separate controls TICK
  - echo notes in record-only mode
  - allow recording of other seqs while playing
  - 
  
- clean up code: it's probably a mess after all the edits.

- implement global tranpose: changing sequence doesn't reset tranpose.
- implement full range: CV in needs to be attenuated but full kb range is possible
- gate length: worth thinking about? Maybe? Half length of note?
- avoid using delay() - not that important, it's only in rec mode

*/

// OPTIONS:
#define GLOBAL_TRANSPOSE 0
#define FULL_RANGE 0


 #define PLAYBACK 0
 #define RECORD 1
#define PITCH 0
#define LENGTH 1



// Template for useful naming of ports:
const int a0 = 0;
const int a1 = 1;
const int a2 = 2;
const int a3 = 3;
const int a4 = 4; // 4 and 5 are bipolar
const int a5 = 5;
const int d0 = 3;
const int d1 = 4;
const int clockInput = 2;
const int bits[8] = {5, 6, 7, 8, 9, 10, 11, 12};

//  constant for actual 0-5V quantization (vs. >> 4)
const int qArray[61] = {
  0,   9,   26,  43,  60,  77,  94,  111, 128, 145, 162, 180, 
  197, 214, 231, 248, 265, 282, 299, 316, 333, 350, 367, 384, 
  401, 418, 435, 452, 469, 486, 503, 521, 538, 555, 572, 589, 
  606, 623, 640, 657, 674, 691, 708, 725, 742, 759, 776, 793, 
  810, 827, 844, 862, 879, 896, 913, 930, 947, 964, 981, 998, 
  1015};
  
const int sequenceMaxLength = 64;
const int sequenceCount = 4;

// Clock input  
volatile int clockIn = LOW;
long lastClockInTime = 0;

// Gate Input
long lastGateInTime = 0;
int gateInState = LOW;

// Gate Output
long lastGateOutTime = 0;
int gateOutState = LOW;

// Clock Output
int clockOutState = LOW;


int trigTime = 25; // 25ms trigger
int activeMode = PLAYBACK;

// Sequences, playback status
// sequences array: 4 sequences, 64 notes, pitch and length
byte sequences[sequenceCount][sequenceMaxLength][2];
int seqLength[sequenceCount];
int seqPosition = 0;
int currentSequence = 0;
int nextSequence = 0;
bool playNextNote = true;
unsigned int noteLengthRemaining = 0;
int noteLength = 1;

// Tranposition
int transpose = 0;
boolean transposing = false;
int transposeCenter = 0;
boolean stopTransposeAtEndOfSequence = false;

//int clockCount = 0; // clock Div

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(clockInput, INPUT);
  pinMode(d0, OUTPUT);
  digitalWrite(d0, LOW);
  pinMode(d1, OUTPUT);
  digitalWrite(d1, LOW);
  for(int i = 0; i < 8; i++)
  {
    pinMode(bits[i], OUTPUT);
    digitalWrite(bits[i], LOW);
  }
  //Serial.println(sizeof(sequences));
  readEEPROM();
  attachInterrupt(0, clockInRising, RISING);
}

void loop() {  
  // read mode knob a0, check if it has moved
  int modeKnob = analogRead(a0) >> 9; // 0 to 1
  if (modeKnob != activeMode)
  {
    activeMode = modeKnob;
    // if we've switched to play, write sequences to eeprom
    if (activeMode == PLAYBACK)
    {
      writeEEPROM();
    }
    // reset sequence
    transposing = false;
    transpose = 0;
    seqPosition = 0;
    currentSequence = nextSequence;
  }
  // read sequence knob, if moved set to new sequence and reset playback
  int sequenceKnob = analogRead(a3) >> 8; // 0 to 3
  if (sequenceKnob != currentSequence)
  {
    stopTransposeAtEndOfSequence = true;
    nextSequence = sequenceKnob;
    if (activeMode == RECORD)
    {
      seqPosition = 0;
      currentSequence = nextSequence;
    }
  }  
  
  // If recording, read noteLength knob. If moved, flash current 
  // note length to LEDs.
  if (activeMode == RECORD)
  {
    int noteKnob = (analogRead(a1) >> 7) + 1; // 1 to 8
    if (noteLength != noteKnob)
    {
      noteLength = noteKnob;
      // clear LEDs
      dacOutput(0);
      // flash leds on
      digitalWrite(bits[noteLength - 1], HIGH);
      // wait
      delay(20);
      // turn em off
      digitalWrite(bits[noteLength - 1], LOW);
    }
  }
    
  
  // if in playback mode and clock is high, play a note
  if (activeMode == PLAYBACK && clockIn == HIGH)
  {
    
    clockIn = LOW;
    lastClockInTime = millis();
    // send a clock out D1 on every clock in
    digitalWrite(d1, HIGH);
    clockOutState = HIGH;

    // if it's time for a new note, start gate out
    if (playNextNote)
    {
      digitalWrite(d0, HIGH);
      gateOutState = HIGH;
      lastGateOutTime = millis();
      int pitch = sequences[currentSequence][seqPosition][PITCH];
      if (transposing) pitch = pitch + transpose;
      dacOutput(pitch << 2);
      noteLengthRemaining = sequences[currentSequence][seqPosition][LENGTH];
      playNextNote = false;
    }
    noteLengthRemaining--;
    if (noteLengthRemaining == 0)
    {
      seqPosition++;
      if (seqPosition == seqLength[currentSequence]) 
      {
        seqPosition = 0;
        currentSequence = nextSequence;
        if (stopTransposeAtEndOfSequence)
        {
          transposing = false;
          transpose = 0;
          stopTransposeAtEndOfSequence = false;
        }
      }
      playNextNote = true;
    }
  }
  // Gate In: if in record, add note to sequence. If playing, transpose seq.
  // wait 10ms since last gate check, check if gate input is high
  if (millis() > lastGateInTime + 10)
  {
    boolean gateInIsHigh = analogRead(a4) > 1000;
    // if it's high and it was low this is a note starting
    if (gateInState == LOW && gateInIsHigh)
    {
      gateInState = HIGH;
      lastGateInTime = millis();
      // wait 20ms (portamento), get current CV In note value
      delay(20); // Maybe avoid using delay() for this. TODO
      //int pitchRead = analogRead(a3);
      //int pitch = vQuant(pitchRead);
      int pitch2Read = (analogRead(a5) - 510);
      int pitch = vQuant(pitch2Read);
      // if we're recording, add note to current sequence
      if (activeMode == RECORD)
      {
        // get desired note length in steps (Hey there Intellijel Metropolis)
        //int noteLength = (analogRead(a1) >> 7) + 1; // 1 to 8
        sequences[currentSequence][seqPosition][PITCH] = pitch;
        sequences[currentSequence][seqPosition][LENGTH] = noteLength;
        // increment seq pos, increase seq length to include this one
        seqPosition++;
        seqLength[currentSequence] = seqPosition;
      }
      // if playing (but not recording (TODO)), transpose sequence
      else if (activeMode == PLAYBACK)
      {
        // if we haven't been transposing, we are now. Get center note.
        if (!transposing)
        {
          transposing = true;
          transposeCenter = pitch;
        }
        // if we have a center note, transpose sequence
        else 
        {
          transpose = pitch - transposeCenter;
        }
      }

    }
    // if it's low and it was high, the note stopped
    if (gateInState == HIGH && !gateInIsHigh)
    {
      gateInState = LOW;
    }
  }
  
  // turn off pins if it's time
  if (gateOutState == HIGH && millis() - lastGateOutTime > trigTime)
  {
    digitalWrite(d0, LOW);
    // TODO: don't turn off gate out until end of note?
    gateOutState == LOW;
    
  }
  if (clockOutState == HIGH && millis() - lastClockInTime > trigTime)
  {
    digitalWrite(d1, LOW);
    clockOutState = LOW;
  }

}

void clockInRising(){
  clockIn = HIGH;
}

//  dacOutput(byte) - deal with the DAC output
//  -----------------------------------------
void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

//  vQuant(int) - properly convert an ADC reading to a value
//  ---------------------------------------------------------
int vQuant(int v)
{
  int tmp = 0;
    
  for (int i=0; i<61; i++) {
    if (v >= qArray[i]) {
      tmp = i;
    }
  }
  
  return tmp;
}

//  writeEEPROM() - write the data to a tagged EEPROM set
//  -----------------------------------------------------
void writeEEPROM()
{
  const int seqLengthsOffset = 4;
  int notesOffset = 8;
  
  // write out the tag
  EEPROM.write(0, 'R');
  EEPROM.write(1, 'M');
  EEPROM.write(2, '1');
  EEPROM.write(3, '1');
  
  // write sequence lengths
  for (int i=0; i < sequenceCount; i++)
  {
    EEPROM.write(seqLengthsOffset + i, seqLength[i]);
  }
  Serial.println("WRITING");

  
  // write notes, pitch followed by length
  for (int s=0; s < sequenceCount; s++)
  {
    Serial.println(s);
    for (int n=0; n < sequenceMaxLength; n++) 
    {
      EEPROM.write(notesOffset, sequences[s][n][PITCH]);
      Serial.print("Pitch: ");
      Serial.println(sequences[s][n][PITCH]);
      notesOffset++;
      EEPROM.write(notesOffset, sequences[s][n][LENGTH]);
      Serial.print("Length: ");
      Serial.println(sequences[s][n][LENGTH]);   
      notesOffset++;
      // if at the end of sequence, break to next sequence
      if (n == seqLength[s] - 1) break;
    }
  }
  Serial.print("Bytes written to EEPROM: ");
  Serial.println(notesOffset);
}

//  readEEPROM() - read the data from a tagged EEPROM set
//  -----------------------------------------------------
void readEEPROM()
{  
  const int seqLengthsOffset = 4;
  int notesOffset = 8;
  
  if (EEPROM.read(0) != 'R')  goto clearit;
  if (EEPROM.read(1) != 'M')  goto clearit;
  if (EEPROM.read(2) != '1')  goto clearit;
  if (EEPROM.read(3) != '1')  goto clearit;
  
  // read sequence lengths
  for (int i = 0; i < sequenceCount; i++)
  {
    seqLength[i] = EEPROM.read(seqLengthsOffset + i);
  }
  Serial.println("READING");
  // read notes, pitch followed by length
  for (int s = 0; s < sequenceCount; s++)
  {
    Serial.println(s);
    for (int n = 0; n < seqLength[s]; n++)
    {
      sequences[s][n][PITCH] = EEPROM.read(notesOffset);
      notesOffset++;
      sequences[s][n][LENGTH] = EEPROM.read(notesOffset);
      notesOffset++;
    }
  }   
  return;
  
clearit:
  // initialise sequence array with chromatic scale
  for (int s = 0; s < 4; s++)
  {
    seqLength[s] = sequenceMaxLength;
    for (int i = 0; i < sequenceMaxLength; i++)
    {
      sequences[s][i][PITCH] = i;
      sequences[s][i][LENGTH] = (i % 5) + 1;
    }
  }  
} 
