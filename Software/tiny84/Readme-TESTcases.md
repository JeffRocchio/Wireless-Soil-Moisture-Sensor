Notes on Testing
========================================

I haven't been able to work out a 'clean' way to use test cases in the Arduino environment.
So I've set it up this way:

   * Each test case gets it's own subdirectory. In that directory is the test case
sketch and any files it depends on that are different from the working program's files.
These subdirectories are named 'TC##' where the ## is the ID number of the test case.

   * There is another subdirectory, named: "00_StashWhileTesting" which is used to
stash the current working program (and potentially files unique to it and not the
test case) while we are working on a test case.

So the procedure to work on, and run, a test case is as follows:

   1. MOVE the main, working, program (e.g., tiny84_SensorAsSlave.ino) into the
00_StashWhileTesting directory. Also move other files that would interfere with
the particular test case, if any.

   2. COPY the contents of the relevant Test Case subdirectory into the parent
directory || OR create the files and code needed for your new test case.

   3. PERFORM TESTS. Make changes and bug fixes as needed until you are done with
the test case.

   4. MOVE test case-specific files back into it's designated subdirectory || IF this
was a new test case, create a new directory for it and move files into that.

   5. MOVE the, working, program back into the parent folder. Including any other
files that were stashed there.

