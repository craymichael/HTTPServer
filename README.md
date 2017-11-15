# Simple HTTP (GET) Server
Compile using `gcc main.c -lpthread -o server`.
Add the `DDEBUG` flag in the compilation string for debugging output.
This works on Linux systems only.

The "html" directory contains a sample index page and a testing page for
permissions (git doesn't preserve ownership so that will need to be changed
manually). The directory is where the server expects to find all files to
serve.

