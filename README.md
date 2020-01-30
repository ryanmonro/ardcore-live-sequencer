# Ardcore Live Sequencer

## by Ryan Monro (2016)

This is an Arduino patch for the [SnazzyFX Ardcore](http://snazzyfx.com/products/ardcore/) eurorack synthesizer module, based on the original [Ardcore project](http://20objects.com/ardcore/) by Darwin Grosse.

### Who is this for?

Hands up if you like writing code. Keep them up if you've messed around with an Arduino. Now keep them up if you're into synthesizers. Keep them up if you know what Eurorack is. 

### Huh?

Imagine a Venn diagram with two circles, one of modular synth dudes/dudettes and one of coder dudes/dudettes. The people in the intersecting bit in the middle are who this is for. As you can see, we're getting into some niche territory. If you're not one of those people and are still reading, I'd better define some things first:

- modular synthesizer: the mad scientist end of the synth world, where separate modules can be plugged together in combinations not allowed by those synths from the '80s with keyboards attached, rewarding those who survive the steep learning curve ascent with more musical/unmusical freedom. See [this incredible photo](https://www.kvraudio.com/forum/viewtopic.php?t=114829) of Steve Porcaro from Toto.
- Eurorack: a standard (with a particular module size, voltage, power supply, etc) of modular synths developed by [Doepfer](http://www.doepfer.de/home.htm) which really took off with (mostly bearded) enthusiasts in the early 21st century, as charted by the sadly defunct blog [Eurorack Dudes With Beards](https://www.factmag.com/2015/07/02/eurorack-dudes-with-beards-tumblr-modular-synths/). For many, a lot of money went in and very little music came out. Garnered the nickname 'eurocrack' due to its wallet-emptyingly addictive nature.
- Arduino: a tiny computer that can be programmed by the user to do things with electronics. The software person's gateway drug to building hardware. Write a program that makes a light blink on and off. Then, one day, write a program that inflates a balloon of Kanye West's face whenever he is mentioned on Twitter.
- Ardcore: a Eurorack module, containing an Arduino. A blank canvas with a few jacks and knobs, allowing you to define its behaviour.
- Live Sequencer: the code I wrote for the Ardcore.

### What does it do?

It started as a clone of the sequencer of the Roland SH-101 – one of my favourites. The 101's sequencer lets you enter a list of up to 100 notes, and it will go on playing them in sequence. It's simple and surprisingly useful. God knows why they chose 100 (it's not really divisible by anything musical) but that's what we've got. The Ardcore Live Sequencer lets you record four sequences of up to 64 notes. Each note can be between 1 and 8 steps long, a feature inspired by the [Intellijel Metropolis](https://intellijel.com/shop/eurorack/metropolis/). Notes are played back in time with pulses received on the Clock input. It really works, and I've actually made music with it.

### What you'll need

1. SnazzyFX Ardcore
2. the SnazzyFX Ardcore Expander
3. a CV/Gate source for entering notes. Most likely a keyboard but it could be a lot of other things.
4. a CV/Gate compatible synthesizer

### How to use

1. Install the [Arduino](https://www.arduino.cc/en/main/software) software and upload the Live Sequencer sketch via USB
2. Plug your clock source into the CLK input
3. Turn the A0 through A3 knobs all the way to the left; A4 and A5 all the way to the right
3. Plug the gate from your keyboard into A4, the CV into A5
4. Plug D0 into the gate in on your synth, and the DAC out into the CV in on your synth
5. optional: the clock in is duplicated on D1 out, if you need that for something else
6. A2 is the Play button: turn that past midday to start. Optional: a high voltage on the A2 jack will also start Play mode if knob A2 is open. Could be handy.
7. Select your sequence with the A3 knob: there are four to choose from.
8. Start your clock source and you should hear something.
9. To Record: turn A0 past midday and we're in record mode: choose the sequence to overwrite with A3. Turn A1 to choose the first note length (which will be displayed on the Expander LEDs), then play a note on the keyboard/joystick/stressball/other-weird-CV-gate-source and it will advance to the next note. Keep following that pattern: choose length, play note. When you're finished, turn A0 back to zero and you'll hear your sequence.

### Acknowledgements

- the EEPROM reading/writing functions are from Darwin Grosse's 101 Sequencer sketch, part of the [20 Objects](https://github.com/darwingrosse/ArdCore-Code) repo.