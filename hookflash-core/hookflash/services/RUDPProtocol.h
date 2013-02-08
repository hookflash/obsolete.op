/*

 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.

 */

#pragma once

/*

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Channel Number        |          Data Length          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Flags     |      Lower 24bits of Sequence Number          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Reserved    |             Lower 24bits of GSNR              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/                      Options and Padding                      /
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
/                       Application Data                        /
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Channel Number - in the same range as allowed by TURN (0x4000 -> 0x7FFF)
Data Length - how much application data is included in this packet after the
              header (0 is a valid length)
Flags - See definition below
Lower 24bits of Sequence Number - lower 24 bits of the 64bit sequence number
Reserved - must be set to "0" on send and ignored upon receipt
Lower 24bits of GSNR - Lower 24bits of the 64bit GSNR (Greatest Sequence
                       Number Received). If no packets have been received this
                       should be set to the NEXT-SEQUENCE-NUMBER as received
                       from the remote party.
Options and padding - If the EQ bit is set to zero in the flags then the
                      vector/GSNFR is included as part of the header.
Application data - Application data at the length of the data length


Flags are defined as follows
 0
 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|P|P|X|D|E|E|A|R|
|S|G|P|P|C|Q|R| |
+-+-+-+-+-+-+-+-+
PS = Parity bit of sending packet (this packet)
PG = Parity of the Greatest Sequence Number Received (if no packets have been
     received yet then this value is "0")
XP = XOR'd parity of all packets received up-to-and-including the GSNFR (if
     no packets have been received then this value is "0")
DP = Duplicate packets have been received since the last ACK packet was sent.
EC = ECN (Explicit Congestions Notification) received on incoming packet
     since last packet in sequence sent. If no packets have been received then
     this value is set to "0".
EQ = GSNR == GSNFR (Greatest Sequence Number Received equals Greatest
     Sequence Number Fully Received). If no packets have been received then
     this value is set to "1".
AR = ACK required (must send a STUN "RELIABLE-CHANNEL-ACK"
     indication/request or another packet with ACK information (i.e.
     header only packet without data is okay).
R  = RFFU (Reserved For Future Use). Must be set to "0" on sending and ignored
     upon receipt


-------------------------------------------------------------------------------
This header is present in packet after the header if EQ flag is "0" (and
therefor cannot be present if no packets were received from the remote party
as the EQ value in this case must be "1").

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|P|Vector Length|             Lower 24bits of GSNFR             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/   [0...vector length] vector RLE information                  .
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
.            [padded RLE to next DWORD alignment (if required)] /
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
P - XOR'ed parity of all packets marked as received in the vector
    (starting at the calculated XORed to-date-up-to-and-including the GSNFR)
Vector Length - Total vector size included after header as expressed in
                DWORDs (if the only packet missing is the GSNFR+1 then
                vector can be zero)
Lower 24 bits of GSNFR - The lower 24 bits of the 64bit GSNFR (Greatest
                         Sequence Number Fully Received).

The entire packet including header cannot be over the PMTU or 512 bytes
if not known.

Sender will estimate the receiver's packet window. The sender will only
send packets that are in the sequence number range from the last reported
GSNFR up to the end of the receiver's
estimated packet window, i.e.:
((sequence_number > GSNFR) &&
 (sequence_number < (GSNFR + estimated_receivers_window)).

The sender will use a fairness algorithm to estimate the receiver's packet
window and adjust the window up and down according to its own policy on how
much data can be outstanding in the path in at half the RTT. The sender
can verify that the receiver has in fact received packets by way of the XOR'd
bit validation in a way that the receiver can't cheat and lie that it has
received packets when it has not. This prevents the receiver from maliciously
pretending it has received packets where it has not and causing the sender
to over-estimate the capacity of the path or miscalculate RTT by the
receiver acknowledging packets faster than it has actually received them.

The sender will cryptographically randomly choose a parity bit for every
packet is sends over the wire.

The sender will include the parity bit of the packet representing the GSNR
packet. The sender will include the XORed parity-to-date up-to-and-including
the GSNFR packet.

If the GSNFR equals the GSNR then the sender will set the EQ bit on the
packet to 1 otherwise it will set it to 0 and include the vector/GSNFR
additional header.

If the network in which the sender receives packets is ECN aware and
marks packets with ECN, the sender will set the EC flag on a packet to
1 if the packet

The sender will mark the last packet in a series when no more data is
available for sending at the moment with an AR flag.

The sender will automatically resend unacknowledged packets that are beyond
haven't been acknowledged in the 2 times the total estimated
RTT (Round Trip Time). The estimated RTT must never be lower than the
negotiated MINIMUM-RTT.

A receiver can ACK packets received in two ways:
1) Any channel data packet sent in a series acts as an ACK for the channel
2) Send a STUN RELIABLE-CHANNEL-ACK indication or request

The receiver will ignore incoming packets with a sequence number that is
less than the receiver's start window (the last fully ACKed packet) plus
the receiver's window size. The receiver will ignore packets that have the
GSNR parity incorrect for their sent packet. The receiver will close the
connection if it receives any packets with the incorrect GP parity or the
XP parity wrong from the same source:port:connection bound to the connection as
this is an attempt either by a spoofer to inject data into the stream or by a
client attempting to fake acknowledgements on packets it never received.

An IP spoofer could attempt to inject data into a stream by randomly flooding
a receiver in attempt to hit within the sequence number window but they would
have to fake the source IP:port and channel number in order for the attack to
succeed. Thus it is recommended that the channel number is randomly chosen
to make a spoof flood attack less likely to succeed.

Be aware that an IP spoofer may use the XP flag as an attack to attempt to
close a connection to which they don't own by broadcasting packets but they
are unlikely to know the correct sequence number window and channel number
and thus would have to attempt to broadcast many packets in order to obtain
a packet within the window. Obviously if they were sniffing and interfering
with the network directly they could launch an attack but they already could
interfere on a much deeper level in such situations which no protocol can stop
but only detect. Adding security, such as TLS on top of RUDP is recommended to
prevent faked data from being injected into a stream.

The receiver will ignore packets that are outside it's own receiving window
(i.e. from the last fully ACKed packet to the last valid received packet
plus the receiver's window). The receiver will ignore packets beyond its
own receiving buffer capacity (i.e. total packets beyond a missing packet
is greater than the receiver is willing to buffer).

If the AR flag was set on an accepted incoming packet for a packet
with a sequence number greater than the last acknowledged, the sender
will send an ACK packet immediately. The receiver can use a data packet
for the ACK as long as it doesn't violate its own sending rules.

The receiver must send an ACK packet for packets that it didn't acknowledge
yet within the window of the oldest unacknowledged packet plus one
calculated RTT time frame. The calculated RTT must never be lower than the
negotiated MINIMUM-RTT.

The receiver will acknowledge all packets it can every single data packet it
sends out.

The receiver will calculate the latest RTT based on the acknowledgement
of its last packet flagged with the AR bit. The calculated RTT must never be
set lower than the negotiated MINIMUM-RTT.

With packets the receiver accepts, the receiver will look for
acknowledgements that it can verify as accurate with the parity bits.
Older packets containing acknowledgements where the data is still available
to validate the parities can be used to acknowledge packets but never be
used to mark already acknowledged packets as having not been received.

Vector format is as follows:
+--------+--------+--------+--------
|SSLLLLLL|SSLLLLLL|SSLLLLLL|  ...
+--------+--------+--------+--------

 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|Sta| Run Length|
+-+-+-+-+-+-+-+-+

Sta[te] occupies the most significant two bits of each byte and can
have one of four values, as follows:

State  Meaning
-----  -------
0    Received
1    Received ECN Marked
2    Reserved
3    Not Yet Received

A "0" vector byte is used at the end of a RLE series for padding to the next
DWORD alignment (and if interpreted would be seen as "0" packets received).

NOTE: There is no guarantee that a "0" byte will be contained at the end
      of any vector RLE series as it is only used for padding.


-----------------------------------------------------------------------------
STUN REQUEST: RELIABLE-CHANNEL-OPEN

This STUN request is used to open a channel to a remote party. In an ICE
environment, the requester is always the ICE controlling party.

Will contain following attributes:

If sent over non-ICE to open an anonymous channel:
First send request without any attributes with get 401 back with
NONCE/REALM back.

USERNAME - is set to userRandomFragRemote:userRandomFromLocal. The random frag
           should be globally unique to not cause conflict and unguessable.
PASSWORD - is set to the userRandomFragRemote. The password is not included
           directly but instead used in the MESSAGE-INTEGRITY calculation.
           When the server issues a request it will reverse the fragments
           and use the userRandomFromLocal as the password.

NONCE - as indicated by server
REALM - as indicated by server

If sent over an established ICE channel:
USERNAME - is set to the userFragRemote:userFragLocal of the nominated ICE
           pairing (just like ICE BINDING requests).
PASSWORD - is set to the ICE password of the remote party of the nominated
           ICE pairing (just like ICE BINDING request). The password is not
           included directly but instead used in the MESSAGE-INTEGRITY
           calculation. Short-term credential calculation is used.
NONCE/REALM - not used.

The request will always contain:
LIFETIME - set to how long the channel should remain open before it is
           automatically closed (in seconds). Setting to zero will
           cause the channel to close immediately and there is no need to
           contain NEXT-SEQUENCE-NUMBER, MINUMIM-RTT or CONGESTION-CONTROL.
           Any data received on the channel or RELIABLE-CHANNEL-ACK will
           cause the LIFETIME attribute timeout countdown to be reset to the
           default.

           If not specified, a LIFETIME of 10 minutes is assumed.
CHANNEL-NUMBER - set to the channel number the local party wishes the remote
                 party to use in all packets the remote party will send to
                 itself.
NEXT-SEQUENCE-NUMBER - set to the first sequence number-1 that
                       will be sent from this location (the first sequence
                       number must be at least 1 but less than 2^48-1)
MINUMIM-RTT - set to the number of milliseconds for the minimum RTT (Round Trip
              Time). The request may contain the MINUMIM-RTT attribute to
              indicate a minimum RTT it wishes to negotiate with the remote
              party.
CONGESTION-CONTROL - A list of congestion control algorithms available to use
                     by the sender with the preferred listed first. There
                     must be two of these attributes, one representing the
                     local (requester) congestion to use and one representing
                     the remote (responder) congestion algorithm.
CONNECTION-INFO - A string representing whatever additional information is
                  required to exchange upon connection.

Response will contain (signed with message integrity):
LIFETIME - The responder can always choose a value lower than the requested
           LIFETIME of the requester but never can respond with "0" unless
           the requester sent "0". This is a negotiated value between requester
           and responder. The channel is kept alive by any channel data being
           sent or by RELIABLE-CHANNEL-ACK requests/indications. If the
           responder wishes to close the channel at a later date the responder
           can chose to issue its own CHANNEL open in the reverse direction
           with a LIFETIME of "0" with the CHANNEL-NUMBER being set to the
           CHANNEL-NUMBER the responder is currently expecting to receive
           from the remote party.

           If not specified, a LIFETIME of what the requester asked is assumed.
NEXT-SEQUENCE-NUMBER - set to the first sequence number-1 that
                       will be sent from this responder (the first sequence
                       number must be at least 1 but less than 2^48-1).
CHANNEL-NUMBER - set to the channel number the responder party wishes the
                 requester to use in all packets it will send to the
                 responder.
MINUMIM-RTT - set to the number of milliseconds for the minimum RTT (Round Trip
              Time) that the response party will accept. If the response
              agrees with the minimum value by the requester it does not
              need to include this attribute. The response may contain this
              attribute value containing a larger than the requester if it
              wishes to negotiate a larger minimum RTT between the two parties
              but can never choose a shorter minimum RTT than the requester.
CONGESTION-CONTROL - A list of congestion control algorithms available to use
                     by the receiver with the selected algorithm listed first.
                     The selected algorithm must be within the list offered
                     by the requester. If the attribute is missing then the
                     responder is assumed to use the algorithm preferred by
                     the requester. Typically, two of these attributes are
                     present in the response, one for the local (i.e.
                     responder) and one for the remote (requester).
CONNECTION-INFO - A string representing whatever additional information is
                  required to exchange upon connection.


When a channel is open for the first time, the responder does not start
sending data until the requester first sends data or sends an ACK. This is
required to ensure the response actually arrived to the requester and thus
proving the negotiation completed.

If either party responds to a renegotiation attempt (i.e. a new
RELIABLE-CHANNEL-OPEN with changed attributes on the same channel, it must
cease sending channel data until a data packet or ACK is received with a
remote sequence number equal or than the sequence number in the request.

If both parties attempt a simutanious renegotiation attempt a
"487 Role Conflict" should result unless the negotiated request from the
remote party is completely compatible with the outstanding negotiated
request issued from the local party.

If either party was attempting to close the channel but an error was
received as a response, the channel should therefor be considered closed
(except in the case where the NONCE is reported as stale).


-----------------------------------------------------------------------------
STUN REQUEST/INDICATION: RELIABLE-CHANNEL-ACK

Either party can send this as a request or indication. The NONCE/REALM are
only needed on a non-ICE situations. All other attributes must be present in
request, except the ACK-VECTOR if it is not needed (i.e. when the only
packet sequence number missing is the GSNR-1). If not send as an indication
then a response is required and the response must contain the same
attributes as listed for the request except the USERNAME, NONCE and REALM.

USERNAME - same logic as RELIABLE-CHANNEL-OPEN
PASSWORD - same logic as RELIABLE-CHANNEL-OPEN
REALM/NONCE - same logic as RELIABLE-CHANNEL-OPEN
CHANNEL-NUMBER - set to the channel number the local party wished the remote
                 party to use in all packets the remote party sent to
                 itself.
NEXT-SEQUENCE-NUMBER - set to the next sequence number the requester will
                       send over the wire (but has not sent yet).
GSNR - set to the greatest sequence number seen from the remote party.
GSNFR - set to greatest sequence number up to which all packets have been
        received.
RELIABLE-FLAGS - Flags indicating the parity or other useful information
ACK-VECTOR - Vector RLE in the same fashion as in the data packet.

A successful response (MESSAGE-INTEGRITY is not required) will indicate
closure is complete. A failure response indicates the request/channel was
not understood properly and the client should consider it closed, except
a 483 where a packet must be re-issued to satisfy the NONCE being stale.


-----------------------------------------------------------------------------
STUN ATTRIBUTE: NEXT-SEQUENCE-NUMBER

This is a 64bit unsigned integer attribute indicating the next sequence
number the requester or responder expects to send (but has not sent).

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      Next Sequence Number                     .
.                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


-----------------------------------------------------------------------------
STUN ATTRIBUTE: GSNR

This is a 64bit unsigned integer attribute indicating the Greatest Sequence
Number Received by the requester/responder encoded in the same method as
the NEXT-SEQUENCE-NUMBER attribute.


-----------------------------------------------------------------------------
STUN ATTRIBUTE: GSNFR

This is a 64bit unsigned integer attribute indicating the Greatest Sequence
Number Fully Received by the requester/responder encoded in the same method
as the NEXT-SEQUENCE-NUMBER attribute. In other words, all the packets
that have been received to date up to a certain sequence number. If the GSNR
is the same value as the GSNFR then this attribute is optional. If this
attribute was not received on a RELIABLE-ACK then the GSNFR is assumed to
be the same value as the GSNR.


-----------------------------------------------------------------------------
STUN ATTRIBUTE: MINIMUM-RTT

This is a 32bit unsigned integer representing the minimum RTT in milliseconds
negotiated by the two parties.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Minimum-RTT                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


-----------------------------------------------------------------------------
STUN ATTRIBUTE: CONNECTION-INFO

An encoded string used at channel open to add additional information about
the connection. The interpretation is entirely dependant on the context.


-----------------------------------------------------------------------------
STUN ATTRIBUTE: RELIABLE-FLAGS

The reliable flags are flags needed to indicate the parity bits and other
acknowledgement flags encoded in 4 bytes. The first byte is the only byte used
at this time.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V|P|X|D|E|  R  | RFFU (Reserved For Future Use)                |
|P|G|P|P|C|     | (must be set to "0" on send and ignored)      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

VP - XOR'ed parity of all packets marked as received in the ACK-VECTOR
     attribute (starting with the calculated XORed to-date-up-to-and
     including the GSNFR) - Same meaning as the "P" flag on the vector/GSNFR
     header in the data packet.
PG = Parity of the Greatest Sequence Number Received
XP = XOR'd parity of all packets received up-to-and-including the GSNFR
DP = Duplicate packets have been received since the last ACK packet was sent.
EC = ECN (Explicit Congestions Notification) received on incoming packet
     since last packet in sequence sent
R  = RFFU (Reserved For Future Use). Must be set to "0" on sending and ignored
     upon receipt


-----------------------------------------------------------------------------
STUN ATTRIBUTE: ACK-VECTOR

Has the same meaning and encoding as the optional vector encoded after the
vector/GSNFR header. The "P" flag from the vector header is contained in the
VP flag of the RELIABLE-FLAGS attribute.


-----------------------------------------------------------------------------
STUN ATTRIBUTE: CONGESTION-CONTROL

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|D|     RFFU    |     RFFU      | Profile preferred or selected |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
/ Profile preferred or selected | Profile preferred or selected /
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
/ Profile preferred or selected | [Profile/padding as required] /
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

The first byte is reserved for flags with only one flag available at this time.
D = Direction. If "0" the congestion control list applies to the "local"
               party. If "1" the congestion control list applies to the
               "remote" party.
               When receiving a request, the remote will apply to the responder
               and the local will apply the requester. When receiving a reply
               the remote will apply to the requester and the local will apply
               to the responder.
RFFU - All bits should be set to "0" and ignored upon receipt.

The second byte is RFFU.

This is a list of unsigned 16bit integers representing the congestions
profile algorithms offered or accepted. The preferred or selected algorithm
must be listed first. The order of the algorithms is assumed to be the
preferred order of the requester or responder. The responder must select an
algorithm within the list offered by the remote party.

*/
