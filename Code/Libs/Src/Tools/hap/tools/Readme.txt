HAP Python Remote Debugger Readme.txt

History:
	31 August 2003 - new version released after a long hiatus
	04 December 2001 - readme created for initial release
	
Overview:
The HAP (Humongous Addition to Python) Python Remote Debugger was created 
as a few people at Humongous Entertainment began using Python and it became 
clear that none of the available debuggers would fit our needs.  We were going to be 
using Python in a few ways that made the existing debuggers less than ideal:
	Our applications would run in full screen mode
	They would make use of multi threading
	They would be developed by people without extensive software development experience
	They would be developed for Windows, Macintosh and other platforms.
	
After initial experiments revealed that it would be possible to make our own debugger, we
started really liking how it was turning out.  Ultimately, we decided that it would be a good
chance to give something back to the Python community so we released the project as
an open source project.

The debugger is made up of two applications: HapDebugger is the editor and IDE, 
HapClient is the remote debugging host - it runs the python script and 
communicates to the IDE via a network socket. The idea was that the 
ConsoleEmbed application would be ported to any platforms that we needed
to support while the DbgRemote app would be maintained only on the windows
platform.  This way we could use a single debugging interface to debug our python
applications regardless of what platform we were running them on.  This two piece
approach also allowed us to break into running python code in a more elegant way
than possible in the standard python debuggers.  

We hope that you find this project useful.  If you have comments or suggestions, please
write in - the developers mailing list on SourceForge is the best place to start.

The current release was built with Python 2.3 - it should be possible to build it for earlier
versions however.

09 September 2003
fixed a problem with syntax highlighting
fixed a problem that prevented the project files from saving their file lists.

31 August 2003
So many new features and its been so long since any have been released that I don't really
remember what all is there.
-Incorporated changes made by Bruce
-Added a right click menu to the main view with lots of goodness in it
-Added an improved right click menu to the output window - you can now find in it.
-Profiling feature - when stepping through python code, put HeDbg.GetLineCount() in the watch window (import HeDbg first) and it will show the number of lines of python code executed with each step - this is very useful for profiling python code.
-When youre debugging, be sure to change your workspace around to suit - it will remember the debugging and editing setups seperately and switch between them when you start/finish debugging.
-I'm also releasing the source code to our perforce and alienbrain version control integrations, they are pretty simple but hooked up, they make using python with source control much easier - if you use a different system, I'll provide whatever help you need to get something working.
