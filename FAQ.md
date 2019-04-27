### FAQ:


(Q):
Do i need autorcm?

(A):
No! Loading a payload will do just that, reboot the switch and load the payload. If you're familiar with reboot_to_payload, it works the exact same way (even uses the code!).


(Q):
Will this work with "name of cfw".

(A):
Short answer: Yes!

Long answer: So far, this works on all cfw such as Atmosphere(kosmos), ReiNX and sxos.


(Q):
Does this mean I can "dual boot" cfw?

(A):
Yes, you can even triple boot! ;)

But yes, you can swap between cfw without the need of a jig or injecting a payload.

 - Be aware that atmosphere and reinx use differnt versions of "sept". If you're on firmware <= 7, then you will need to change the sept files before trying to dual boot atmosphere and reinx.
 - Support can be added that swaps the sept files, just unsure which is the best method for it. Feel free to open a suggestion!


(Q):
What if i select the wrong payload, what will happen?

(A):
Depends on the payload. If you send sxos payload, and you dont have sxos, you will have a "boot.dat?" message. Selecting lockpick will simpily load lockpick.

In all, it will just load the payload thats selected.


(Q):
Will this brick my switch?

(A):
No.


(Q):
How can i delete hotkeys?

(A):
Press either LS(left stick) or RS(right stick). You can also override the hotkey with a new one.


(Q):
I've encountered an error || this does not work for me, what do i do?

(A):
Please open an issue stating what doesn't work (the error as well if possible) and i will get back to you shortly.


(Q):
What's 9 + 10?

(A):
21.