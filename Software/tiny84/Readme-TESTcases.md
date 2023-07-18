Notes on Testing
========================================

I haven't been able to work out a 'clean' way to use test cases in the Arduino environment due to the way that the IDE and the environment is dependent upon the root sketch file name being the exact same as the directory name; and that you cannot specify file-paths in the #include statements. So for myself I've defined the following procedure for test-cases:

   * Each test case gets it's own subdirectory, inside the main sketch directory. In that directory is the test case sketch and any files it depends on that are not library files in the normal Arduino library directory.

   * These subdirectories are named 'TC##_DescriptiveName' where the ## is the ID number of the test case and 'DescriptiveName' is a brief title for the test case.

The procedure to work on, and run, a test case is as follows:

   1. Do a git commit just before starting in on the below procedure. This way you have a way to roll back to this point should things get out of hand as you muck around with the code in various files trying to get the test case to work out. Use command: [`git commit -m "TC##_DescriptiveName. Commit just prior to initiating work on test case."`]

   1. IF starting a new test case, create the directory for it, using the above noted naming convention.

   1. COPY all files the test case depends on into the test case's subdirectory. Do this as a COPY and not a move just so you have a handy copy in the parent directory to revert back to if need be.

   1. IF starting a new test case, create a .ino file that will be the main sketch that will perform the test(s) of your test case. Typically the easiest way to do this is to copy the main sketch from a prior test case that will serve as a good starting basis for your new test case. Copy such a sketch from it's TC## subdirectory into the subdirectory of the test case you are using.

   1. Open your TC##_DescriptiveName.ino sketch in the Arduino IDE.

   1. Write your test code.

   1. PERFORM TESTS, COMMIT AS APPROPRIATE. Make changes and bug fixes as needed until you are done with the test case. Consider making git commits as you go, based on the extent and nature of code modifications you are making to satisfy the test case; esp changes to the dependent files that will get applied back up to the primary sketch once the test case work is completed. Think of these as snapshots you can use to recover back to in case you end up in dead-ends and need to restart. Use: [`git commit -m "TC##_DescriptiveName. WIP. [optional brief description.]"`]

   1. PRE-CLOSE COMMIT. Once you have completed the test case, capture a snapshot for a reliable rollback point. Do: [`git commit -m "TC##_DescriptiveName. Work completed, pre-close commit."`]

   1. COPY all main-sketch dependent files back up to the parent directory. Do not copy the test case's main sketch nor files that are only used by the test case's main sketch. Obviously some of those files will have been modified as the test was run and fixes made. So you are bringing those changes back up to the ATTiny84 sensor program you are building or modifying. Therefore, as you make this file copy you will be asked if you wish to overwrite the pre-existing files. Just respond 'yes' to all. This will, of course, also overwrite files that you didn't need to change as part of your test work, but that's fine. I feel it's more reliable to just overwrite all the files rather than working out which files changed as you tested and which did not.

   1. CLOSE TEST CASE: Do: [`git commit -m "TC##_DescriptiveName: Tests Completed. Dependent files [have | have not] been modified."`] In conjunction with the above mentioned commits this gives you several good rollback points just in case something bad or mysterious has happened. You can get back to the state of the code at several key points to work out issues or mysteries.

