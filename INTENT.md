# Intent

This document tries to capture the intent behind some of the design decisions I made in the code.

# Known defects

There is at least one defect lurking in the Dexter firmware that I never found.  For example, sometimes when changing modes into PR-8210, this mode won't work correctly.  But pressing RESET (after the EEPROM has updated) always fixes it so I never bothered to track this down.

# All of these #ifdef's for REV1/REV2/REV3 or V1/V2/V3

Dexter has gone through three major revisions (rev1, rev2, and rev3).

Throughout the code you may find #ifdef's for these different revisions.
REV3 is the only revision that you should care about.  Anything marked with REV1 or REV2 is for obsolete hardware and can be safely deleted.
I just didn't bother deleting that stuff because it was extra work that didn't provide much benefit.

But I can see how these old code artifacts might look important to a newcomer so I want to document specifically that they are safe to delete.

# The ideal laserdisc player mode

ld-700* is the last one I wrote and therefore is probably the best example of how these should be written.
ld-v1000* is the first one I wrote and is therefore probably the worst example :)

# The test suite

When I embarked on this project, I didn't have a good way to write unit tests (due to the source code being in C instead of C++), so I relied instead on hardware manual testing (wince).
At some point, I figured out a slick way to unit test C code so most of the existing unit tests are for ld700* and any other code that I touched when working on that mode.
So just because unit tests are missing for a lot of the code doesn't mean I don't think it is important.  I just didn't have the tools to do it.
If you want to backfill code with unit tests (which I would recommend!) follow the pattern for the ld700* tests.  You could probably even get AI to do most of it for you since it has a solid pattern to follow.

# Capturing the LM1881 vsync signal

In the different laserdisc player modes, syncing the call to 'ldpc_OnVBlankChanged' up with the LM1881 hardware vsync may be inconsistently handled (sometimes using FIELD_PIN, sometimes using the inverse of FIELD_PIN, sometimes just toggling an internal flag).

If I were to pick one way of handling it that is probably the 'best', it would be the way it is handled inside ld700-main.c .  But be aware that I never performed an exhaustive study of what the correct behavior should be, so do not assume that just because the code is handling it a certain way, that that way is the best!  If you do some tests, you may find a better way!

Be careful about changing the way it's handled in timing critical modes like vp931.  It's okay to experiment with changing the behavior, just make sure you can do a hardware test to ensure Firefox's attract mode still looks correct (for example).

The point is that I never really did thorough testing of whether ldpc_OnVBlankChanged should pass FIELD_PIN or the inverse of FIELD_PIN.  I just took a theoretical guess and noticed that things seemed to work.  It may be possible that either one would work.

## Super Don Quixote

There is a Dexter bug that causes 1 wrong frame to often be displayed when Super Don performs a skip.  This may be related to how ldpc_OnVBlankChanged is handled in ldv1000 mode.

If you want to clean up ld-v1000's behavior, I'd would target Super Don Quixote and try to clean up the behavior on that game.  Once you get that dialed in, that would be a big clue for how ldpc_OnVBlankChanged should be handled, perhaps for all modes.

# The VBI injector

The aux AVR (the ATMega328p) mostly handles VBI picture number injection for PR-8210/A games only such as Cliff Hanger, Goal to Go, Mach 3, us vs Them, cobra command and Star Rider.

Due to the way this is designed, the picture number that the game sees will always be 1 field/frame behind what a real laserdisc player would have showed.  It's important to realize this limitation so that you don't wrongly assume that Dexter is more accurate than it actually is.

In practice, this picture number lag doesn't seem to adversely impact game play significantly.

# Assembly language callbacks ( for example, g_pVsyncCallback )

You may notice that all of the vsync callback methods are written in assembly language instead of C.

The main reason for this is the AVR only allows one hard-coded ISR per interrupt which I decided to implement in assembly language for performance.  This means that the vsync ISR had to be implemented in assembly language.  Since I was using an assembly routine to handle the callback (vsync_asm.s), if I tried to point vsync_asm.s to a C function, the C compiler would push literally every register onto the stack and pop every register off since it had no visibility into which registers were safe to clobber.  So rather than incurring this pointless waste, I decided to just (re)write all of the vsync callbacks into assembly language so I could avoid this.  I followed the pattern of writing the callbacks first in C, then looking at the compiler's disassembly to get hints on how to rewrite it in assembly language.

Other callbacks that follow this same pattern include g_pTimer1Callback ( ex. ldv1000_timer1_callback, ld700_on_ext_ctrl_timeout ), and g_pPCINT0Callback ( ex. ld700_on_ext_ctrl_changed ).

Using C functions instead of assembly language functions should be possible if someone can figure out how to tell the compiler which registers are safe to clobber so that it doesn't push every single register onto the stack.  This is what I would do if I could figure it out.

I historically would give every laserdisc player mode its own custom vsync callback.  This was because LD-V1000 (which I wrote first) strictly documents the timing of the command/status strobes relative to vsync.  However, for some modes (such as ld-700* which I wrote last), the vsync timing doesn't matter that much so I didn't write a callback at all but instead polled to see when vsync had triggered.  Some of the modes that currently have their own vsync callbacks could probably get away with just polling, especially if the callback does nothing other than setting a flag that says "hey, we got vsync!"

# Assembly language routines that need to really be assembly language

The VP931 assembly language routines are assembly language for performance specifically and should not be attempted to be converted to C.  Performance is critical for those due to the strict timing requirements of the VP931 interface.

The assembly language routines for the aux AVR (VBI injector) also need to be assembly language because they literally count instruction cycles to make sure that the generated VBI is the correct size.

# Generated VBI has some jitter

If you've ever watched the VBI data that Dexter generates (using an Ati 750 for example), you may notice that the VBI is not stable and has some jitter.  This is because the aux AVR's csync ISR doesn't trigger until the currently running instruction finishes (some instructions may be 1 cycle, some 2 cycles, etc).  So the ISR's starting time relative to csync will not be consistent.  You may be able to improve the consistency by putting the AVR to sleep ('halt' I believe is the term) so that it wakes up when csync is generated.  This should remove the jitter.  But you'd want to make sure the Star Rider still works if you make any changes, specifically the disabling of the pr-8210a vsync (star rider's seek test would test this for you).

In practice, this jitter doesn't seem to matter as far as gameplay, so I'd just leave it as-is unless it really bugs you.
