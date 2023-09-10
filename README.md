# Laurie
An **improvisational oscillator** for the **Korg Nu:Tekt NTS-1** inspired by Laurie Spiegel's **Music Mouse** - replacing mouse input with the encoders on the NTS-1, triggering melody and chord relative to their position and scale selected. In total there are 4 voices distributed between the encoders, configured either in a 1-3 or 2-2 setup. A 5th voice can also be triggered from the keyboard to add an additional layer.

## Usage
The oscillator in intended to be used with **EG** set to **Open**.

When changed, the **A + B** encoders trigger notes for melody and chord. In **default configuration**, notes are triggered with a random delay (after turning an encoder) to add a strumming feel. Pressing a key will **trigger a note relative to the A encoder**.

Keys below C (lowest visible on the NTS-1 keybed) **toggles between 1-3 or 2-2 voice allocation** (A-B encoders).

The **B encoder can also be configured to trigger from a keypress** - this is accomplished by keeping the arpeggiator running and changing the **mode parameter** to a value above 1 (under oscillator parameters). The random delay between notes will then indirectly be determined by the (arpeggiator) tempo.   

> The oscillator is quite CPU intensive, preventing usage of all effect default stages (mod + delay + reverb) at the same time - but e.g. delay + reverb is possible

## Oscillator parameters

### Scale
| Value |  Scale
| -----:| -----:|
| 1 | Pentatonic
| 2 | Octatonic
| 3 | Diatonic
| 4 | Mid-Eastern
| 5 | Chromatic

### Note offset
1-12

### Mode
| Value        |  A encoder | B encoder | 5th voice pitch | 5th voice trigger
| -----:| -----:| -----:| -----:| -----:|
| 1 | Immediate | Immediate | A encoder | Keypress
| 2 | Immediate | Keypress  | A encoder | Keypress
| 3 | Immediate | Keypress  | A encoder + ARP octave shift | Keypress
| 4 | Immediate | Keypress  | A encoder | A encoder

### Patch
1-9 (**1 = Random**)

Upload [laurie.ntkdigunit](laurie.ntkdigunit) using the NTS-1 digital Librarian application.
