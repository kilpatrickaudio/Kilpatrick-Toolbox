# vMIDI&trade;

## Patchable MIDI Implementation Guide for Developers

Kilpatrick Audio has developed the **vMIDI&trade;** protocol of patchable
MIDI inside VCV Rack. This uses the existing CV cable infrastructure but encodes MIDI messages in place of voltages.
Other developers are encouraged to use **vMIDI&trade;** in their own plugins for maximum intercompatibility.

## Protocol Specifications

Messages are sent over the CV signal interface within VCV Rack by encoding a 3-byte MIDI message as a '''float''',
sending it over the virtual cable, and then converting it back to an '''int''' representation on the receiving side.
To ensure proper encoding of the message we assume that a normal IEEE 32 bit '''float''' can represent whole numbers
within the range of -16777215 to 0 which is true for 32 bit floats. By default only
monophonic cables are used to send a single MIDI port over a cable. A single three byte message is sent in each sample
period. The format for the int message word for each message is as follows:

<pre>
- bits 23:16 - data[0] (status byte) - normally a status byte
- bits 15:8 - data[1] (data1 byte) - normally the first data byte of a 2-3 byte MIDI message
- bits 7:0 - data[2] (data2 byte) - normally the second data byte of a 3 byte MIDI message
</pre>

### Converting to/from a CV Signal</h2>

The best way to use **vMIDI&trade;** is to make a small helper library to encode and decode messages. The
internal VCV Rack MIDI structure '''midi::Message''' can be used for MIDI messages going in and out of the library.
To send a message, encode an '''int''' with the 24 bit format shown above and cast it to a '''float''' while
also changing the sign so that the number is negative:

<pre>
    int msgWord;
    msgWord = (msg.bytes[0] & 0xff) << 16;
    msgWord |= (msg.bytes[1] & 0xff) << 8;
    msgWord |= msg.bytes[2] & 0xff;
    port->setVoltage(-(float)msgWord);
</pre>

If no message is to be sent, set the output value to 0.0f to indicate that there is no valid message. A received value of
<0.0f indicates a valid message and enables any value to be sent for each of the three data bytes. It is very important to set
the value to 0.0f when no message is present to prevent the parser code from running on every sample. This will also prevent
messages from being received without a cable connected without having to check the connection status.

When receiving the data, check to see that the value is <0.0f which indicates that a valid message is available. Convert the
float to an int by switching the sign and using '''roundf()''' and then inspect / unpack the bits in the returned
'''int'''. All three bytes can then be parsed.

<pre>
    int msgWord;
    if(port->getVoltage() < 0.0f) {
        msgWord = roundf(-port->getVoltage());
        msg.bytes[0] = (msgWord >> 16) & 0xff;
        msg.bytes[1] = (msgWord >> 8) & 0xff;
        msg.bytes[2] = msgWord & 0xff;
        // parser code goes here
    }
</pre>

### Parsing Data to Determine the Message Size</h2>

When parsing the data it is necessary to correctly encode the size variable within the midi::Message struct. Inspect
the data and determine the number of bytes based on the status byte. The MIDI specification has a list of status bytes
and the expected length for each type of message. All values (even undefined ones) should be implemented. If the first byte
is not a valid status byte (MSB not set) then the message is probably part of a SYSEX message being sent across multiple
3-byte message.

The following explains a simple procedure for parsing messages to quickly determine the length. This is believed to cover
all valid messages that could occur:

- If the first byte is a valid status byte choose the correct length - 0xf0 almost certainly is at least 3 bytes long so set it as 3

- If the first byte is not a valid status byte:
  - If the second byte is 0xf7 then this is the end of a SYSEX message - set the length to 2

  - If the second byte is not 0xf7 then this is probably part of a SYSEX message - set the length to 3

## Usage Requirements

If you use **vMIDI&trade;** within your own module designs you must label them as supporting **vMIDI&trade;**
in the documentation and website for the plugin. Also you must link to this page from your documentation or product site
so we can encourage more developers to use it and get credit for creating it. **vMIDI&trade;** is a trademark of
Kilpatrick Audio and may be used without limitation as long as the protocol is not changed and credit is suitably given.
